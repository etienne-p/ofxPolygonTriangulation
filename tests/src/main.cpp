#include "ofAppNoWindow.h"
#include "ofDoublyConnectedEdgeList.h"
#include "ofMain.h"
#include "ofPolygonTriangulation.h"
#include "ofPolygonUtility.h"
#include "ofxUnitTests.h"

class ofApp : public ofxUnitTestsApp {
private:
	ofDoublyConnectedEdgeList m_Dcel;

	void title(const string & str) {
		ofLogNotice() << "====(( "
					  << str
					  << " ))====";
	}

public:
	void TestDcelConstruction() {
		title("Testing Dcel Construction");

		vector<glm::vec2> vertices = {
			{ 0, 0 },
			{ 1, 0 },
			{ 1, 1 },
			{ 0, 1 }
		};

		auto dcel = ofDoublyConnectedEdgeList();
		dcel.initializeFromCCWVertices(vertices);

		auto innerFace = dcel.getInnerFace();

		auto edgeIt = ofDoublyConnectedEdgeList::HalfEdgesIterator(innerFace);
		auto count = 0;
		do {
			ofxTest(edgeIt.getCurrent().getIncidentFace() == dcel.getInnerFace(), "Inner half edges are bound properly.");
			ofxTest(edgeIt.getCurrent().getTwin().getIncidentFace().getIndex() == ofDoublyConnectedEdgeList::getOuterFaceIndex(), "Outer half edges are bound properly.");
			++count;
		} while (edgeIt.moveNext());

		ofxTest(vertices.size() == count, "Half edges on inner face match vertices count.");

		count = 0;
		auto vertex = innerFace.getOuterComponent().getOrigin();
		auto faceOnVertit = ofDoublyConnectedEdgeList::FacesOnVertexIterator(vertex);
		do {
			++count;
		} while (faceOnVertit.moveNext());

		ofxTest(count == 1, "Vertex is on one non-outer face.");
	}

	void TestDcelSplitFaceAdjacentFails() {
		title("Testing Dcel Split Face Adjacent Fails");

		vector<glm::vec2> vertices = {
			{ 0, 0 },
			{ 1, 0 },
			{ 1, 1 },
			{ 0, 1 }
		};

		auto dcel = ofDoublyConnectedEdgeList();
		dcel.initializeFromCCWVertices(vertices);

		auto edge = dcel.getInnerFace().getOuterComponent();
		auto vertexA = edge.getOrigin();
		auto vertexB = edge.getDestination();

		bool trew = false;
		try {
			dcel.splitFace(vertexA, vertexB);
		} catch (std::runtime_error error) {
			trew = true;
		}

		ofxTest(trew, "Split adjacent vertices throws.");
	}

	void TestDcelSplit() {
		title("Testing Dcel Split Face");

		vector<glm::vec2> vertices = {
			{ 0, 0 },
			{ 1, 0 },
			{ 1, 1 },
			{ 0, 1 }
		};

		auto dcel = ofDoublyConnectedEdgeList();
		dcel.initializeFromCCWVertices(vertices);

		auto edge = dcel.getInnerFace().getOuterComponent();
		auto vertexA = edge.getOrigin();
		auto vertexB = edge.getNext().getDestination();
		dcel.splitFace(vertexA, vertexB);

		auto faceIt = ofDoublyConnectedEdgeList::FacesIterator(dcel);

		do {
			auto count = 0;
			auto face = faceIt.getCurrent();
			ofxTest(face.getIndex() != ofDoublyConnectedEdgeList::getOuterFaceIndex(), "Face iterator skips outer face.");
			auto edgeIt = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);
			do {
				ofxTest(edgeIt.getCurrent().getIncidentFace() == face, "Split face is bound properly.");
				++count;
			} while (edgeIt.moveNext());
			ofxTest(count == 3, "Split face vertices count.");
		} while (faceIt.moveNext());
	}

	void TestFacesOnVertexIterator() {
		title("Testing Test Faces On Vertex Iterator");

		vector<glm::vec2> vertices = {
			{ 0, 0 },
			{ 1, 0 },
			{ 2, 1 },
			{ 1, 1 },
			{ 0, 1 }
		};

		auto dcel = ofDoublyConnectedEdgeList();
		dcel.initializeFromCCWVertices(vertices);

		const auto edge = dcel.getInnerFace().getOuterComponent();
		auto vertexA = edge.getOrigin();
		auto vertexB = edge.getNext().getDestination();
		auto vertexC = edge.getNext().getNext().getDestination();
		dcel.splitFace(vertexA, vertexB);
		//dcel.splitFace(vertexA, vertexC);

		// Note: full triangulation by now.
		auto it = ofDoublyConnectedEdgeList::FacesOnVertexIterator(vertexA);
		auto count = 0;
		do {
			ofxTest(it.getCurrent().getIncidentFace().getIndex() != ofDoublyConnectedEdgeList::getOuterFaceIndex(), "Faces on vertex iterator iterator skips outer face.");
			++count;
		} while (it.moveNext());
		ofxTest(count == 2, "Faces on vertex iterator traverses proper faces.");
	}

	void TestSplitToMonotone() {
		title("Split To Monotone");

		auto dcel = ofDoublyConnectedEdgeList();
		auto splitToMonotone = ofSplitToMonotone();
		vector<glm::vec3> vertices;

		for (auto i = 12; i != 64; ++i) {
			vertices.resize(i);
			ofPolygonUtility::createPolygonRandom(vertices);
			dcel.initializeFromCCWVertices(vertices);
			splitToMonotone.execute(dcel, dcel.getInnerFace());

			// Check that all the faces are monotone.
			auto facesIt = ofDoublyConnectedEdgeList::FacesIterator(dcel);
			do {
				const auto face = facesIt.getCurrent();
				if (ofDoublyConnectedEdgeList::getWindingOrder(face) != ofPolygonWindingOrder::CounterClockWise) {
					ofxTest(false, "Face is clockwise.");
				}
				auto edgesIt = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);
				do {
					// A polygon is monotone if it has no split nor merge vertices.
					auto vertex = edgesIt.getCurrent().getOrigin();
					auto classification = ofSplitToMonotone::classifyVertex(vertex);
					if (classification == ofSplitToMonotone::VertexType::Merge || classification == ofSplitToMonotone::VertexType::Split) {
						ofxTest(false, "Split to monotone failed.");
						return;
					}
				} while (edgesIt.moveNext());
			} while (facesIt.moveNext());
		}
		ofxTest(true, "Split to monotone succeeded.");
	}

	void TestTriangulateMonotone() {
		title("Triangulate Monotone");

		auto dcel = ofDoublyConnectedEdgeList();
		auto triangulateMonotone = ofTriangulateMonotone();
		vector<glm::vec3> vertices;

		for (auto i = 12; i != 64; ++i) {
			vertices.resize(i);
			ofPolygonUtility::createPolygonRandomMonotone(vertices);
			dcel.initializeFromCCWVertices(vertices);
			triangulateMonotone.execute(dcel, dcel.getInnerFace());

			// Check that all the faces are triangles.
			auto facesIt = ofDoublyConnectedEdgeList::FacesIterator(dcel);
			do {
				const auto face = facesIt.getCurrent();
				if (ofDoublyConnectedEdgeList::getWindingOrder(face) != ofPolygonWindingOrder::CounterClockWise) {
					ofxTest(false, "Face is clockwise.");
				}
				auto edgesIt = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);
				auto count = 0;
				do {
					++count;
				} while (edgesIt.moveNext());
				if (count != 3) {
					ofxTest(false, "Triangulate monotone failed.");
				}
			} while (facesIt.moveNext());
		}
		ofxTest(true, "Triangulate monotone succeeded.");
	}

	void TestTriangulate() {
		title("Triangulate");

		auto dcel = ofDoublyConnectedEdgeList();
		auto polygonTriangulation = ofPolygonTriangulation();
		vector<glm::vec3> vertices;

		for (auto i = 12; i != 64; ++i) {
			vertices.resize(i);
			ofPolygonUtility::createPolygonRandom(vertices);
			dcel.initializeFromCCWVertices(vertices);
			polygonTriangulation.execute(dcel);

			// Check that all the faces are triangles.
			auto facesIt = ofDoublyConnectedEdgeList::FacesIterator(dcel);
			do {
				const auto face = facesIt.getCurrent();
				if (ofDoublyConnectedEdgeList::getWindingOrder(face) != ofPolygonWindingOrder::CounterClockWise) {
					ofxTest(false, "Face is clockwise.");
				}
				auto edgesIt = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);
				auto count = 0;
				do {
					++count;
				} while (edgesIt.moveNext());
				if (count != 3) {
					ofxTest(false, "Triangulate failed.");
				}
			} while (facesIt.moveNext());
		}
		ofxTest(true, "Triangulate succeeded.");
	}

	void run() {
		TestDcelConstruction();
		TestDcelSplitFaceAdjacentFails();
		TestDcelSplit();
		TestFacesOnVertexIterator();
		TestSplitToMonotone();
		TestTriangulateMonotone();
		TestTriangulate();
	}
};

int main() {
	ofInit();
	auto window = std::make_shared<ofAppNoWindow>();
	auto app = std::make_shared<ofApp>();
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(window, app);
	return ofRunMainLoop();
}