#include "SplitToMonotone.h"
#include "VertexSweepComparer.h"
#include <glm/gtx/vector_angle.hpp>
#include <stdexcept>

SplitToMonotone::VertexType classifyVertex(DoublyConnectedEdgeList::Vertex vertex) {
	const auto angle = glm::orientedAngle(
		glm::normalize(vertex.getIncidentEdge().getPrev().getDirection()),
		glm::normalize(vertex.getIncidentEdge().getDirection()));

	auto comparer = VertexSweepComparer();
	auto prevVertex = vertex.getIncidentEdge().getPrev().getOrigin();
	auto nextVertex = vertex.getIncidentEdge().getDestination();
	auto compPrev = comparer(prevVertex, vertex);
	auto compNext = comparer(vertex, nextVertex);

	// If the 2 neighbors lie above.
	if (compPrev && !compNext) {
		return angle > 0 ? SplitToMonotone::VertexType::Stop : SplitToMonotone::VertexType::Merge;
	}

	// If the 2 neighbors lie below.
	if (!compPrev && compNext) {
		return angle > 0 ? SplitToMonotone::VertexType::Start : SplitToMonotone::VertexType::Split;
	}

	return SplitToMonotone::VertexType::Regular;
}

void diagonalToPreviousEdgeHelper(
	DoublyConnectedEdgeList & dcel, SplitToMonotone::SweepStatus & sweepStatus,
	std::map<std::size_t, SplitToMonotone::VertexType> & verticesClassification, DoublyConnectedEdgeList::Vertex & vertex) {
	auto helper = sweepStatus.getHelper(vertex.getIncidentEdge().getPrev());

	if (verticesClassification[helper.getIndex()] == SplitToMonotone::VertexType::Merge) {
		dcel.splitFace(vertex.getIncidentEdge(), helper, DoublyConnectedEdgeList::EdgeAssign::None);
	}

	sweepStatus.remove(vertex.getIncidentEdge().getPrev());
}

void diagonalToLeftEdgeHelper(
	DoublyConnectedEdgeList & dcel, SplitToMonotone::SweepStatus & sweepStatus,
	std::map<std::size_t, SplitToMonotone::VertexType> & verticesClassification, DoublyConnectedEdgeList::Vertex & vertex) {
	auto leftEdge = sweepStatus.findLeft(vertex);
	auto leftHelper = sweepStatus.getHelper(leftEdge);
	if (verticesClassification[leftHelper.getIndex()] == SplitToMonotone::VertexType::Merge) {
		dcel.splitFace(vertex.getIncidentEdge(), leftHelper, DoublyConnectedEdgeList::EdgeAssign::None);
	}

	sweepStatus.updateHelper(leftEdge, vertex);
}

void handleStartVertex(SplitToMonotone::SweepStatus & sweepStatus, DoublyConnectedEdgeList::Vertex & vertex) {
	sweepStatus.emplace(vertex.getIncidentEdge(), vertex);
}

void handleStopVertex(
	DoublyConnectedEdgeList & dcel, SplitToMonotone::SweepStatus & sweepStatus,
	std::map<std::size_t, SplitToMonotone::VertexType> & verticesClassification, DoublyConnectedEdgeList::Vertex & vertex) {
	diagonalToPreviousEdgeHelper(dcel, sweepStatus, verticesClassification, vertex);
}

void handleSplitVertex(
	DoublyConnectedEdgeList & dcel, SplitToMonotone::SweepStatus & sweepStatus,
	DoublyConnectedEdgeList::Vertex & vertex) {
	auto leftEdge = sweepStatus.findLeft(vertex);
	auto leftHelper = sweepStatus.getHelper(leftEdge);
	dcel.splitFace(vertex.getIncidentEdge(), leftHelper, DoublyConnectedEdgeList::EdgeAssign::None);
	sweepStatus.updateHelper(leftEdge, vertex);
	sweepStatus.emplace(vertex.getIncidentEdge(), vertex);
}

void handleMergeVertex(
	DoublyConnectedEdgeList & dcel, SplitToMonotone::SweepStatus & sweepStatus,
	std::map<std::size_t, SplitToMonotone::VertexType> & verticesClassification, DoublyConnectedEdgeList::Vertex & vertex) {
	diagonalToPreviousEdgeHelper(dcel, sweepStatus, verticesClassification, vertex);
	diagonalToLeftEdgeHelper(dcel, sweepStatus, verticesClassification, vertex);
}

void handleRegularVertex(
	DoublyConnectedEdgeList & dcel, SplitToMonotone::SweepStatus & sweepStatus,
	std::map<std::size_t, SplitToMonotone::VertexType> & verticesClassification, DoublyConnectedEdgeList::Vertex & vertex) {
	// if the interior of the polygon lies to the right of vertex
	if (glm::orientedAngle(glm::vec2(1, 0), glm::normalize(vertex.getIncidentEdge().getDirection())) <= 0) {
		diagonalToPreviousEdgeHelper(dcel, sweepStatus, verticesClassification, vertex);
		sweepStatus.emplace(vertex.getIncidentEdge(), vertex);
	} else {
		diagonalToLeftEdgeHelper(dcel, sweepStatus, verticesClassification, vertex);
	}
}

void SplitToMonotone::execute(DoublyConnectedEdgeList & dcel, DoublyConnectedEdgeList::Face & face) {
	m_SweepStatus.clear();
	m_VerticesClassification.clear();
	m_Vertices.clear();

	// Collect and label vertices on face.
	auto edge = face.getOuterComponent();
	do {
		auto vertex = edge.getOrigin();
		m_Vertices.push_back(vertex);
		m_VerticesClassification.emplace(vertex.getIndex(), classifyVertex(vertex));
		edge = edge.getNext();
	} while (edge != face.getOuterComponent());

	// Sort vertices according to sweep line.
	std::sort(m_Vertices.begin(), m_Vertices.end(), VertexSweepComparer());

	for (auto it = m_Vertices.begin(); it != m_Vertices.end(); ++it) {
		// update comparer with sweep line position
		m_SweepStatus.setSweepLineY(it->getY());

		switch (m_VerticesClassification[it->getIndex()]) {
		case SplitToMonotone::VertexType::Start:
			handleStartVertex(m_SweepStatus, *it);
			break;

		case SplitToMonotone::VertexType::Stop:
			handleStopVertex(dcel, m_SweepStatus, m_VerticesClassification, *it);
			break;

		case SplitToMonotone::VertexType::Split:
			handleSplitVertex(dcel, m_SweepStatus, *it);
			break;

		case SplitToMonotone::VertexType::Merge:
			handleMergeVertex(dcel, m_SweepStatus, m_VerticesClassification, *it);
			break;

		case SplitToMonotone::VertexType::Regular:
			handleRegularVertex(dcel, m_SweepStatus, m_VerticesClassification, *it);
			break;
		}
	}
}
