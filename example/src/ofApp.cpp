#include <cassert>
#include <random>
#include "Triangulatemonotone.h"
#include "ofApp.h"

void ofApp::setup()
{
	ofSetVerticalSync(true);

	// Always at least one line available.
	m_Lines.push_back(ofPolyline());
	m_LineColors.push_back(ofColor::aquamarine);

	m_NumPointsSlider.addListener(this, &ofApp::numPointsChanged);
	m_IsMonotoneToggle.addListener(this, &ofApp::isMonotoneChanged);
	m_SplitToMonotoneButton.addListener(this, &ofApp::splitToMonotoneButtonPressed);
	m_TriangulateButton.addListener(this, &ofApp::triangulateButtonPressed);
	m_ResetButton.addListener(this, &ofApp::resetButtonPressed);

	m_Gui.setup();
	m_Gui.add(m_NumPointsSlider.setup("Num Points", m_NumPoints, 2, 64));
	m_Gui.add(m_IsMonotoneToggle.setup("Is Monotone", m_IsMonotone));
	m_Gui.add(m_SplitToMonotoneButton.setup("Split To Monotone"));
	m_Gui.add(m_TriangulateButton.setup("Triangulate"));
	m_Gui.add(m_ResetButton.setup("Reset"));
}

void ofApp::exit()
{
	m_NumPointsSlider.removeListener(this, &ofApp::numPointsChanged);
	m_IsMonotoneToggle.removeListener(this, &ofApp::isMonotoneChanged);
	m_SplitToMonotoneButton.removeListener(this, &ofApp::splitToMonotoneButtonPressed);
	m_TriangulateButton.removeListener(this, &ofApp::triangulateButtonPressed);
	m_ResetButton.removeListener(this, &ofApp::resetButtonPressed);
}

void ofApp::update() {}

void ofApp::draw()
{
	auto width = ofGetWidth();
	auto height = ofGetHeight();

	// TODO Replace by some scope mapping a rect coords to screen.
	ofPushMatrix();
	ofTranslate(glm::vec3(width / 2.0f, height / 2.0f, 0));
	ofScale(glm::vec3(width / 2.0f, height / 2.0f, 1.0f));

	// TODO only when relevant.
	m_Mesh.drawWireframe();

	// TODO Minor ick
	auto index = 0;
	for (const auto& line : m_Lines)
	{
		ofSetColor(m_LineColors[index]);
		line.draw();
		++index;
	}

	ofPopMatrix();

	m_Gui.draw();
}

void ofApp::numPointsChanged(int& numPoints)
{
	m_NumPoints = numPoints;
	updatePolygon(m_Lines[0]);
}

void ofApp::isMonotoneChanged(bool& isMonotone)
{
	m_IsMonotone = isMonotone;
	updatePolygon(m_Lines[0]);
}

void ofApp::splitToMonotoneButtonPressed()
{
	m_Dcel.initializeFromCCWVertices(m_Vertices);
	auto innerFace = m_Dcel.getInnerFace();
	m_SplitToMonotone.execute(m_Dcel, innerFace);

	// TODO Expose number of faces to alloc ahead.
	m_Lines.clear();
	m_LineColors.clear();

	auto facesIterator = DoublyConnectedEdgeList::FacesIterator(m_Dcel);
	do
	{
		auto face = facesIterator.getCurrent();

		if (face.getIndex() == DoublyConnectedEdgeList::getOuterFaceIndex())
		{
			continue;
		}

		auto halfEdgesIterator = DoublyConnectedEdgeList::HalfEdgesIterator(face);

		auto line = ofPolyline();

		do
		{
			auto vertex = halfEdgesIterator.getCurrent().getOrigin().getPosition();
			line.addVertex(glm::vec3(vertex.x, vertex.y, 0));
		} while (halfEdgesIterator.moveNext());

		m_Lines.push_back(line);
	} while (facesIterator.moveNext());

	m_LineColors.resize(m_Lines.size());

	for (auto i = 0; i != m_LineColors.size(); ++i)
	{
		m_LineColors[i].setHsb(ofRandom(255), ofRandom(150, 255), ofRandom(150, 255));
	}
}

// TODO Belongs in separate triangulation type.
enum class FaceType
{
	Other,
	Triangle,
	Quad
};

FaceType getFaceType(DoublyConnectedEdgeList::Face face)
{
	// The limited walk along the edge cycle allows us to unroll the loop.
	// Note that NO face ever has less than 3 edges.
	const auto firstEdge = face.getOuterComponent();
	auto edge = firstEdge;

	// Jump along the 2 next edges.
	edge = edge.getNext().getNext();

	// If the current edge point to the first, we have 3 edges and a triangle.
	if (edge.getNext() == firstEdge)
	{
		return FaceType::Triangle;
	}

	// One more jump. If the current edge point to the first, we have 4 edges and a quad.
	edge = edge.getNext();
	if (edge.getNext() == firstEdge)
	{
		return FaceType::Quad;
	}

	// Otherwise we have more than 4 edges.
	return FaceType::Other;
}

void ofApp::triangulateButtonPressed()
{
	auto facesIterator = DoublyConnectedEdgeList::FacesIterator(m_Dcel);

	do
	{
		auto face = facesIterator.getCurrent();

		if (face.getIndex() == m_Dcel.getOuterFaceIndex())
		{
			continue;
		}

		switch (getFaceType(face))
		{
			// No further triangulation needed.
		case FaceType::Triangle:
			continue;

			// Quad is trivially reduced to triangles,
			// no need to run a full monotone triangulation.
		case FaceType::Quad:
		{
			auto edgeA = face.getOuterComponent();
			auto edgeB = edgeA.getNext().getNext();
			m_Dcel.splitFace(edgeA, edgeB, DoublyConnectedEdgeList::EdgeAssign::None);
		}
		continue;

		// Needs monotone triangulation.
		case FaceType::Other:
			m_FacesPendingTriangulation.push(face);
			break;
		}
	} while (facesIterator.moveNext());

	while (!m_FacesPendingTriangulation.empty())
	{
		auto face = m_FacesPendingTriangulation.top();
		m_FacesPendingTriangulation.pop();
#if _DEBUG

		// By this point, we are iterating through the faces of the original DCEL.
		assert(DoublyConnectedEdgeList::getOrder(face) == WindingOrder::CounterClockWise);
#endif

		// We must ensure that all the vertices we are about to process have an incident edge on the current face.
		auto halfEdgeIterator = DoublyConnectedEdgeList::HalfEdgesIterator(face);
		do
		{
			auto edge = halfEdgeIterator.getCurrent();
			edge.getOrigin().setIncidentEdge(edge);
		} while (halfEdgeIterator.moveNext());

		m_TriangulateMonotone.execute(m_Dcel, face);
	}

	m_Dcel.extractTriangles(m_Vertices, m_Indices);

	m_Mesh.clear();
	m_Mesh.addVertices(m_Vertices);
	m_Mesh.addIndices(m_Indices);
}

void ofApp::resetButtonPressed()
{
	// TODO clumsy.
	// Leave 1 line for base polygon. 
	m_Lines.resize(1);
	m_LineColors.resize(1);
	m_Mesh.clear();
}

void ofApp::updatePolygon(ofPolyline& line)
{
	m_Vertices.resize(m_NumPoints);

	if (m_IsMonotone)
	{
		createPolygonRandomMonotone(m_Vertices, m_NumPoints);
	}
	else
	{
		createPolygonRandom(m_Vertices, m_NumPoints);
	}

	line.clear();
	line.addVertices(m_Vertices);
	line.setClosed(true);
}

// No self-intersection by construction.
void ofApp::createPolygonRandom(vector<glm::vec3>& points, size_t numPoints)
{
	constexpr auto angleNoiseY = 0.4f;
	constexpr auto radiusNoiseY = 4.0f;
	auto noiseX = 0.f;
	auto dAngle = glm::pi<float>() * 2.0f / (float)numPoints;

	for (auto i = 0; i != numPoints; ++i)
	{
		auto angle = glm::mix(dAngle * i, dAngle * (i + 1), ofNoise(noiseX, angleNoiseY));
		auto radius = glm::mix(0.2f, 1.0f, ofNoise(noiseX, radiusNoiseY));
		points[i] = glm::vec3(radius * glm::cos(angle), radius * glm::sin(angle), 0);
		noiseX += 0.4f;
	}
}

void ofApp::createPolygonRandomMonotone(vector<glm::vec3>& points, size_t numPoints)
{
	constexpr auto xNoiseY = 0.4f;
	constexpr auto noiseDx = 0.4f;
	constexpr auto yNoiseY = 0.8f;
	constexpr auto epsilon = 1e-6f;

	const auto midPoint = numPoints / 2;

	auto noiseX = 0.f;
	auto dY = 2.0f / (float)(midPoint - 1);
	auto yAcc = -1.0f;

	points[0] = glm::vec3(0, -1, 0);

	// Left chain.
	for (auto i = 1; i != midPoint; ++i)
	{
		auto x = glm::mix(-1.0f, -0.1f, ofNoise(noiseX, xNoiseY));
		auto y = glm::mix(yAcc + epsilon, yAcc + dY - epsilon, ofNoise(noiseX, yNoiseY));
		points[i] = glm::vec3(x, y, 0);
		noiseX += noiseDx;
		yAcc += dY;
	}

	points[midPoint] = glm::vec3(0, 1, 0);

	dY = 2.0f / (float)(numPoints - midPoint - 1);
	yAcc = 1.0f;

	// Right chain.
	for (auto i = midPoint + 1; i != numPoints; ++i)
	{
		auto x = glm::mix(0.1f, 1.0f, ofNoise(noiseX, xNoiseY));
		auto y = glm::mix(yAcc - dY + epsilon, yAcc - epsilon, ofNoise(noiseX, yNoiseY));
		points[i] = glm::vec3(x, y, 0);
		noiseX += noiseDx;
		yAcc -= dY;
	}
}

void ofApp::keyPressed(int key) {}

void ofApp::keyReleased(int key) {}

void ofApp::mouseMoved(int x, int y) {}

void ofApp::mouseDragged(int x, int y, int button) {}

void ofApp::mousePressed(int x, int y, int button) {}

void ofApp::mouseReleased(int x, int y, int button) {}

void ofApp::mouseEntered(int x, int y) {}

void ofApp::mouseExited(int x, int y) {}

void ofApp::windowResized(int w, int h) {}

void ofApp::gotMessage(ofMessage msg) {}

void ofApp::dragEvent(ofDragInfo dragInfo) {}
