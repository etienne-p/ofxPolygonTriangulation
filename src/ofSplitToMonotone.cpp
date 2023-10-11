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
	auto helper = m_SweepStatus.getHelper(vertex.getIncidentEdge().getPrev());

	if (m_VerticesClassification[helper.getIndex()] == ofSplitToMonotone::VertexType::Merge) {
		dcel.splitFace(vertex.getIncidentEdge(), helper, ofDoublyConnectedEdgeList::EdgeAssign::None);
	}

	m_SweepStatus.remove(vertex.getIncidentEdge().getPrev());
}

void ofSplitToMonotone::diagonalToLeftEdgeHelper(
	ofDoublyConnectedEdgeList & dcel,
	ofDoublyConnectedEdgeList::Vertex & vertex) {
	auto leftEdge = m_SweepStatus.findLeft(vertex);
	auto leftHelper = m_SweepStatus.getHelper(leftEdge);
	if (m_VerticesClassification[leftHelper.getIndex()] == ofSplitToMonotone::VertexType::Merge) {
		dcel.splitFace(vertex.getIncidentEdge(), leftHelper, ofDoublyConnectedEdgeList::EdgeAssign::None);
	}

	m_SweepStatus.updateHelper(leftEdge, vertex);
}

void ofSplitToMonotone::handleStartVertex(
	ofDoublyConnectedEdgeList::Vertex & vertex) {
	m_SweepStatus.emplace(vertex.getIncidentEdge(), vertex);
}

void ofSplitToMonotone::handleStopVertex(
	ofDoublyConnectedEdgeList & dcel,
	ofDoublyConnectedEdgeList::Vertex & vertex) {
	diagonalToPreviousEdgeHelper(dcel, vertex);
}

void ofSplitToMonotone::handleSplitVertex(
	ofDoublyConnectedEdgeList & dcel,
	ofDoublyConnectedEdgeList::Vertex & vertex) {
	auto leftEdge = m_SweepStatus.findLeft(vertex);
	auto leftHelper = m_SweepStatus.getHelper(leftEdge);
	dcel.splitFace(vertex.getIncidentEdge(), leftHelper, ofDoublyConnectedEdgeList::EdgeAssign::None);
	m_SweepStatus.updateHelper(leftEdge, vertex);
	m_SweepStatus.emplace(vertex.getIncidentEdge(), vertex);
}

void ofSplitToMonotone::handleMergeVertex(
	ofDoublyConnectedEdgeList & dcel,
	ofDoublyConnectedEdgeList::Vertex & vertex) {
	diagonalToPreviousEdgeHelper(dcel, vertex);
	diagonalToLeftEdgeHelper(dcel, vertex);
}

void ofSplitToMonotone::handleRegularVertex(
	ofDoublyConnectedEdgeList & dcel,
	ofDoublyConnectedEdgeList::Vertex & vertex) {
	// if the interior of the polygon lies to the right of vertex
	if (glm::orientedAngle(glm::vec2(1, 0), glm::normalize(vertex.getIncidentEdge().getDirection())) <= 0) {
		diagonalToPreviousEdgeHelper(dcel, vertex);
		m_SweepStatus.emplace(vertex.getIncidentEdge(), vertex);
	} else {
		diagonalToLeftEdgeHelper(dcel, vertex);
	}
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
		m_SweepStatus.setSweepLineY(it->getY());

		switch (m_VerticesClassification[it->getIndex()]) {
		case ofSplitToMonotone::VertexType::Start:
			handleStartVertex(*it);
			break;

		case ofSplitToMonotone::VertexType::Stop:
			handleStopVertex(dcel, *it);
			break;

		case ofSplitToMonotone::VertexType::Split:
			handleSplitVertex(dcel, *it);
			break;

		case ofSplitToMonotone::VertexType::Merge:
			handleMergeVertex(dcel, *it);
			break;

		case ofSplitToMonotone::VertexType::Regular:
			handleRegularVertex(dcel, *it);
			break;
		}
	}

	m_SweepStatus.clear();
	m_VerticesClassification.clear();
	m_Vertices.clear();
}
