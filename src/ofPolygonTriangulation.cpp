#include "ofPolygonTriangulation.h"
#include <glm/gtx/vector_angle.hpp>

// Used to identify trivial polygons.
enum class FaceType {
	Triangle,
	Quad,
	Polygon
};

FaceType getFaceType(ofDoublyConnectedEdgeList::Face face) {
	const auto firstEdge = face.getOuterComponent();
	auto edge = firstEdge;

	// Jump along the 2 next edges.
	edge = edge.getNext().getNext();

	// If the current edge point to the first, we have 3 edges and a triangle.
	if (edge.getNext() == firstEdge) {
		return FaceType::Triangle;
	}

	edge = edge.getNext();

	// If the current edge point to the first, we have 4 edges and a quad.
	if (edge.getNext() == firstEdge) {
		return FaceType::Quad;
	}

	return FaceType::Polygon;
}

void triangulateQuad(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face face) {
	// Find maximal inner angle.
	auto maxInnerAngleEdge = ofDoublyConnectedEdgeList::HalfEdge();
	ofDoublyConnectedEdgeList::findMaxInnerAngle(dcel, face, maxInnerAngleEdge);

	// Add a diagonal starting at the max inner angle vertex.
	dcel.addHalfEdge(maxInnerAngleEdge, maxInnerAngleEdge.getNext().getNext());
}

void ofPolygonTriangulation::execute(ofDoublyConnectedEdgeList & dcel) {
	auto innerFace = dcel.getInnerFace();
	m_SplitToMonotone.execute(dcel, innerFace);

	auto facesIterator = ofDoublyConnectedEdgeList::FacesIterator(dcel);

	do {
		auto face = facesIterator.getCurrent();
		
		switch (getFaceType(face)) {
		case FaceType::Triangle:
			// Bypass already triangulated faces.
			continue;
		case FaceType::Quad:
			triangulateQuad(dcel, face);
			break;
		default:
			m_FacesPendingTriangulation.push(face);
		}

	} while (facesIterator.moveNext());

	while (!m_FacesPendingTriangulation.empty()) {
		auto face = m_FacesPendingTriangulation.top();
		m_FacesPendingTriangulation.pop();

		// We must ensure that all the vertices we are about to process have an incident edge on the current face.
		auto halfEdgeIterator = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);
		do {
			auto edge = halfEdgeIterator.getCurrent();
			edge.getOrigin().setIncidentEdge(edge);
		} while (halfEdgeIterator.moveNext());

		m_TriangulateMonotone.execute(dcel, face);
	}
}
