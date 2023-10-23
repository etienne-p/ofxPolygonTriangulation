#include <glm/gtx/vector_angle.hpp>
#include "ofPolygonTriangulation.h"

enum class FaceType {
	Triangle,
	Quad,
	Polygon
};

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

// Calculates the oriented angle, 
// returning the result in the [0, 2PI] range,
// instead of the [-PI, PI] range.
float angle(glm::vec2 a, glm::vec2 b) {
	auto angle = glm::orientedAngle(a, b);
	if (angle < 0) {
		return angle + 2.0f * glm::pi<float>();
	}
	return angle;
}

void triangulateQuad(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face face) {
	// Find maximal inner angle.
	auto it = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);
	auto prevDir = glm::vec2();
	auto currentDir = glm::normalize(it.getCurrent().getPrev().getDirection());
	auto maxInnerAngleEdge = ofDoublyConnectedEdgeList::HalfEdge();
	auto maxInnerAngle = 0.0f;

	do {
		prevDir = currentDir;
		currentDir = glm::normalize(it.getCurrent().getDirection());
		auto innerAngle = angle(currentDir, prevDir);
		if (maxInnerAngle < innerAngle)
		{
			maxInnerAngle = innerAngle;
			maxInnerAngleEdge = it.getCurrent();
		}

	} while (it.moveNext());

	// Add a diagonal starting at the max inner angle vertex.
	dcel.addHalfEdge(maxInnerAngleEdge, maxInnerAngleEdge.getNext().getNext());
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
