#include "ofApp.h"
#include "ofPolygonUtility.h"
#include <cassert>
#include <random>

void ofApp::setup() {
	ofSetVerticalSync(true);

	// Always at least one line available.
	m_Lines.push_back(ofPolyline());
	m_LineColors.push_back(ofColor::aquamarine);

	m_NumPointsSlider.addListener(this, &ofApp::numPointsChanged);
	m_IsMonotoneToggle.addListener(this, &ofApp::isMonotoneChanged);
	m_IsWireFrameToggle.addListener(this, &ofApp::isWireFrameChanged);
	m_SplitToMonotoneButton.addListener(this, &ofApp::splitToMonotoneButtonPressed);
	m_TriangulateButton.addListener(this, &ofApp::triangulateButtonPressed);
	m_ResetButton.addListener(this, &ofApp::resetButtonPressed);

	m_Gui.setup();
	m_Gui.add(m_NumPointsSlider.setup("Num Points", m_NumPoints, 2, 64));
	m_Gui.add(m_IsMonotoneToggle.setup("Is Monotone", m_IsMonotone));
	m_Gui.add(m_IsWireFrameToggle.setup("Is WireFrame", m_IsMonotone));
	m_Gui.add(m_SplitToMonotoneButton.setup("Split To Monotone"));
	m_Gui.add(m_TriangulateButton.setup("Triangulate"));
	m_Gui.add(m_ResetButton.setup("Reset"));
}

void ofApp::exit() {
	m_NumPointsSlider.removeListener(this, &ofApp::numPointsChanged);
	m_IsMonotoneToggle.removeListener(this, &ofApp::isMonotoneChanged);
	m_IsWireFrameToggle.removeListener(this, &ofApp::isWireFrameChanged);
	m_SplitToMonotoneButton.removeListener(this, &ofApp::splitToMonotoneButtonPressed);
	m_TriangulateButton.removeListener(this, &ofApp::triangulateButtonPressed);
	m_ResetButton.removeListener(this, &ofApp::resetButtonPressed);
}

void ofApp::update() { }

void ofApp::draw() {
	auto width = ofGetWidth();
	auto height = ofGetHeight();

	ofPushMatrix();
	ofTranslate(glm::vec3(width / 2.0f, height / 2.0f, 0));
	ofScale(glm::vec3(width / 2.0f, height / 2.0f, 1.0f));

	if (m_IsWireFrame) {
		m_Mesh.drawWireframe();
	} else {
		m_Mesh.draw();
	}

	auto index = 0;
	for (const auto & line : m_Lines) {
		ofSetColor(m_LineColors[index]);
		line.draw();
		++index;
	}

	ofPopMatrix();

	m_Gui.draw();
}

void ofApp::numPointsChanged(int & numPoints) {
	m_NumPoints = numPoints;
	updatePolygon(m_Lines[0]);
}

void ofApp::isMonotoneChanged(bool & isMonotone) {
	m_IsMonotone = isMonotone;
	updatePolygon(m_Lines[0]);
}

void ofApp::isWireFrameChanged(bool & isWireFrame) {
	m_IsWireFrame = isWireFrame;
}

void ofApp::splitToMonotoneButtonPressed() {
	m_Dcel.initializeFromCCWVertices(m_Vertices);
	auto innerFace = m_Dcel.getInnerFace();
	m_SplitToMonotone.execute(m_Dcel, innerFace);

	// TODO Expose number of faces to alloc ahead.
	m_Lines.clear();
	m_LineColors.clear();

	auto facesIterator = ofDoublyConnectedEdgeList::FacesIterator(m_Dcel);
	do {
		auto face = facesIterator.getCurrent();
		auto halfEdgesIterator = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);
		auto line = ofPolyline();

		do {
			auto vertex = halfEdgesIterator.getCurrent().getOrigin().getPosition();
			line.addVertex(glm::vec3(vertex.x, vertex.y, 0));
		} while (halfEdgesIterator.moveNext());

		// Close the line.
		auto vertex = halfEdgesIterator.getCurrent().getOrigin().getPosition();
		line.addVertex(glm::vec3(vertex.x, vertex.y, 0));

		m_Lines.push_back(line);
	} while (facesIterator.moveNext());

	m_LineColors.resize(m_Lines.size());

	for (auto i = 0; i != m_LineColors.size(); ++i) {
		m_LineColors[i].setHsb(ofRandom(255), ofRandom(150, 255), ofRandom(150, 255));
	}
}

void ofApp::triangulateButtonPressed() {
	m_Dcel.initializeFromCCWVertices(m_Vertices);
	m_Triangulation.execute(m_Dcel);
	m_Dcel.extractTriangles(m_Vertices, m_Indices);

	m_Mesh.clear();
	m_Mesh.addVertices(m_Vertices);
	m_Mesh.addIndices(m_Indices);
}

void ofApp::resetButtonPressed() {
	// TODO clumsy.
	// Leave 1 line for base polygon.
	m_Lines.resize(1);
	m_LineColors.resize(1);
	m_Mesh.clear();
}

void ofApp::updatePolygon(ofPolyline & line) {
	m_Vertices.resize(m_NumPoints);

	if (m_IsMonotone) {
		ofPolygonUtility::createPolygonRandomMonotone(m_Vertices);
	} else {
		ofPolygonUtility::createPolygonRandom(m_Vertices);
	}

	line.clear();
	line.addVertices(m_Vertices);
	line.setClosed(true);
}
