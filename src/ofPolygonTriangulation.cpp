#include "ofPolygonTriangulation.h"

enum class FaceType {
	Other,
	Triangle,
	Quad
};

FaceType getFaceType(ofDoublyConnectedEdgeList::Face face) {
	// The limited walk along the edge cycle allows us to unroll the loop.
	// Note that NO face ever has less than 3 edges.
	const auto firstEdge = face.getOuterComponent();
	auto edge = firstEdge;

	// Jump along the 2 next edges.
	edge = edge.getNext().getNext();

	// If the current edge point to the first, we have 3 edges and a triangle.
	if (edge.getNext() == firstEdge) {
		return FaceType::Triangle;
	}

	// One more jump. If the current edge point to the first, we have 4 edges and a quad.
	edge = edge.getNext();
	if (edge.getNext() == firstEdge) {
		return FaceType::Quad;
	}

	// Otherwise we have more than 4 edges.
	return FaceType::Other;
}

void ofPolygonTriangulation::execute(ofDoublyConnectedEdgeList & dcel) {
	auto innerFace = dcel.getInnerFace();
	m_SplitToMonotone.execute(dcel, innerFace);

	auto facesIterator = ofDoublyConnectedEdgeList::FacesIterator(dcel);

	do {
		auto face = facesIterator.getCurrent();

		if (face.getIndex() == dcel.getOuterFaceIndex()) {
			continue;
		}

		switch (getFaceType(face)) {
			// No further triangulation needed.
		case FaceType::Triangle:
			continue;

			// Quad is trivially reduced to triangles,
			// no need to run a full monotone triangulation.
		case FaceType::Quad: {
			//auto edgeA = face.getOuterComponent();
			//auto edgeB = edgeA.getNext().getNext();
			//dcel.splitFace(edgeA, edgeB, ofDoublyConnectedEdgeList::EdgeAssign::None);
			m_FacesPendingTriangulation.push(face);
		}
			continue;

		// Needs monotone triangulation.
		case FaceType::Other:
			m_FacesPendingTriangulation.push(face);
			break;
		}
	} while (facesIterator.moveNext());

	while (!m_FacesPendingTriangulation.empty()) {
		auto face = m_FacesPendingTriangulation.top();
		m_FacesPendingTriangulation.pop();
#if _DEBUG

		// By this point, we are iterating through the faces of the original DCEL.
		assert(ofDoublyConnectedEdgeList::getOrder(face) == ofPolygonWindingOrder::CounterClockWise);
#endif

		// We must ensure that all the vertices we are about to process have an incident edge on the current face.
		auto halfEdgeIterator = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);
		do {
			auto edge = halfEdgeIterator.getCurrent();
			edge.getOrigin().setIncidentEdge(edge);
		} while (halfEdgeIterator.moveNext());

		m_TriangulateMonotone.execute(dcel, face);
	}
}
