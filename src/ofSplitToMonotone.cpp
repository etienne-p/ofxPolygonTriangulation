#include "ofSplitToMonotone.h"
#include "ofVertexSweepComparer.h"
#include <glm/gtx/vector_angle.hpp>
#include <stdexcept>

ofSplitToMonotone::VertexType classifyVertex(ofDoublyConnectedEdgeList::Vertex vertex) {
	const auto angle = glm::orientedAngle(
		glm::normalize(vertex.getIncidentEdge().getPrev().getDirection()),
		glm::normalize(vertex.getIncidentEdge().getDirection()));

	auto comparer = ofVertexSweepComparer();
	auto prevVertex = vertex.getIncidentEdge().getPrev().getOrigin();
	auto nextVertex = vertex.getIncidentEdge().getDestination();
	auto compPrev = comparer(prevVertex, vertex);
	auto compNext = comparer(vertex, nextVertex);

	// If the 2 neighbors lie above.
	if (compPrev && !compNext) {
		return angle > 0 ? ofSplitToMonotone::VertexType::Stop : ofSplitToMonotone::VertexType::Merge;
	}

	// If the 2 neighbors lie below.
	if (!compPrev && compNext) {
		return angle > 0 ? ofSplitToMonotone::VertexType::Start : ofSplitToMonotone::VertexType::Split;
	}

	return ofSplitToMonotone::VertexType::Regular;
}

void ofSplitToMonotone::diagonalToPreviousEdgeHelper(
	ofDoublyConnectedEdgeList & dcel,
	ofDoublyConnectedEdgeList::Vertex & vertex) {
	auto prevEdge = vertex.getIncidentEdge().getPrev();
	auto helper = m_SweepLineStatus.getHelper(prevEdge);

	if (m_VerticesClassification[helper.getIndex()] == ofSplitToMonotone::VertexType::Merge) {
		dcel.splitFace(vertex.getIncidentEdge(), helper, ofDoublyConnectedEdgeList::EdgeAssign::None);
	}

	m_SweepLineStatus.remove(prevEdge);
}

void ofSplitToMonotone::diagonalToLeftEdgeHelper(
	ofDoublyConnectedEdgeList & dcel,
	ofDoublyConnectedEdgeList::Vertex & vertex) {
	auto leftEdge = m_SweepLineStatus.findLeft(vertex);
	auto leftHelper = m_SweepLineStatus.getHelper(leftEdge);
	if (m_VerticesClassification[leftHelper.getIndex()] == ofSplitToMonotone::VertexType::Merge) {
		dcel.splitFace(vertex.getIncidentEdge(), leftHelper, ofDoublyConnectedEdgeList::EdgeAssign::None);
	}

	m_SweepLineStatus.updateHelper(leftEdge, vertex);
}

void ofSplitToMonotone::execute(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face & face) {
	// Collect and label vertices on face.
	auto edge = face.getOuterComponent();
	do {
		auto vertex = edge.getOrigin();
		m_Vertices.push_back(vertex);
		m_VerticesClassification.emplace(vertex.getIndex(), classifyVertex(vertex));
		edge = edge.getNext();
	} while (edge != face.getOuterComponent());

	// Sort vertices according to sweep line.
	std::sort(m_Vertices.begin(), m_Vertices.end(), ofVertexSweepComparer());

	for (auto it = m_Vertices.begin(); it != m_Vertices.end(); ++it) {
		// update comparer with sweep line position
		m_SweepLineStatus.setCoordinate(it->getY());

		switch (m_VerticesClassification[it->getIndex()]) {
		case ofSplitToMonotone::VertexType::Start: {
			m_SweepLineStatus.emplace(it->getIncidentEdge(), *it);
		} break;

		case ofSplitToMonotone::VertexType::Stop: {
			diagonalToPreviousEdgeHelper(dcel, *it);
		} break;

		case ofSplitToMonotone::VertexType::Split: {
			auto vertex = *it;
			auto leftEdge = m_SweepLineStatus.findLeft(vertex);
			auto leftHelper = m_SweepLineStatus.getHelper(leftEdge);
			dcel.splitFace(vertex.getIncidentEdge(), leftHelper, ofDoublyConnectedEdgeList::EdgeAssign::None);
			m_SweepLineStatus.updateHelper(leftEdge, vertex);
			m_SweepLineStatus.emplace(vertex.getIncidentEdge(), vertex);
		} break;

		case ofSplitToMonotone::VertexType::Merge: {
			diagonalToPreviousEdgeHelper(dcel, *it);
			diagonalToLeftEdgeHelper(dcel, *it);
		} break;

		case ofSplitToMonotone::VertexType::Regular: {
			auto vertex = *it;

			// if (glm::orientedAngle(glm::vec2(1, 0), glm::normalize(vertex.getIncidentEdge().getDirection())) <= 0)
			auto prevY = vertex.getIncidentEdge().getPrev().getOrigin().getY();
			auto nextY = vertex.getIncidentEdge().getDestination().getY();

			// If the interior of the polygon lies to the right of vertex.
			if (prevY > nextY) {
				diagonalToPreviousEdgeHelper(dcel, vertex);
				m_SweepLineStatus.emplace(vertex.getIncidentEdge(), vertex);
			} else {
				diagonalToLeftEdgeHelper(dcel, vertex);
			}
		} break;
		}
	}

	m_SweepLineStatus.clear();
	m_VerticesClassification.clear();
	m_Vertices.clear();
}
