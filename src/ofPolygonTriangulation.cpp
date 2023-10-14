#include "ofPolygonTriangulation.h"

bool isTriangle(ofDoublyConnectedEdgeList::Face face) {
	const auto firstEdge = face.getOuterComponent();
	auto edge = firstEdge;

	// Jump along the 2 next edges.
	edge = edge.getNext().getNext();

	// If the current edge point to the first, we have 3 edges and a triangle.
	if (edge.getNext() == firstEdge) {
		return true;
	}

	return false;
}

void ofPolygonTriangulation::execute(ofDoublyConnectedEdgeList & dcel) {
	auto innerFace = dcel.getInnerFace();
	m_SplitToMonotone.execute(dcel, innerFace);

	auto facesIterator = ofDoublyConnectedEdgeList::FacesIterator(dcel);

	do {
		auto face = facesIterator.getCurrent();
		// Bypass already triangulated faces.
		if (isTriangle(face)) {
			continue;
		}

		m_FacesPendingTriangulation.push(face);
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
