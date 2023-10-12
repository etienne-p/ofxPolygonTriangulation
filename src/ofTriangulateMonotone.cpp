#include "ofTriangulateMonotone.h"
#include "ofVertexSweepComparer.h"
#include <cassert>
#include <glm/gtx/vector_angle.hpp>

using VertexAndIndex = std::pair<ofDoublyConnectedEdgeList::Vertex, std::size_t>;

bool isInside(
	const ofDoublyConnectedEdgeList::Vertex & vertex,
	const ofDoublyConnectedEdgeList::Chain vertexChain,
	const ofDoublyConnectedEdgeList::Vertex & popped,
	const ofDoublyConnectedEdgeList::Vertex & prevPopped) {
	auto currentEdge = popped.getPosition() - vertex.getPosition();
	auto prevEdge = prevPopped.getPosition() - vertex.getPosition();
	auto alpha = glm::orientedAngle(glm::normalize(prevEdge), glm::normalize(currentEdge));

	if (vertexChain == ofDoublyConnectedEdgeList::Chain::Left) {
		return alpha <= 0;
	}

	return alpha >= 0;
}

ofDoublyConnectedEdgeList::EdgeAssign getEdgeAssign(
	const ofDoublyConnectedEdgeList::Vertex & origin,
	const ofDoublyConnectedEdgeList::Vertex & destination) {
	if (origin.getChain() == destination.getChain()) {
		// We reassign so that the edge whose normal points inside is used
		if (origin.getChain() == ofDoublyConnectedEdgeList::Chain::Left) {
			// Rebind the edge that points down.
			return origin.getY() > destination.getY() ? ofDoublyConnectedEdgeList::EdgeAssign::Origin : ofDoublyConnectedEdgeList::EdgeAssign::Destination;
		}

		return origin.getY() < destination.getY() ? ofDoublyConnectedEdgeList::EdgeAssign::Origin : ofDoublyConnectedEdgeList::EdgeAssign::Destination;
	}

	// Otherwise, we reassign so that the deg whose normal points downwards is used.
	return origin.getChain() == ofDoublyConnectedEdgeList::Chain::Left ? ofDoublyConnectedEdgeList::EdgeAssign::Destination : ofDoublyConnectedEdgeList::EdgeAssign::Origin;
}

void getTopAndBottomVertices(
	const ofDoublyConnectedEdgeList::Face & face,
	ofDoublyConnectedEdgeList::HalfEdge & top,
	ofDoublyConnectedEdgeList::HalfEdge & bottom) {
	top = face.getOuterComponent();
	bottom = face.getOuterComponent();

	auto halfEdgesIterator = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);

	do {
		auto edge = halfEdgesIterator.getCurrent();

		if (ofVertexSweepComparer()(edge.getOrigin(), top.getOrigin())) {
			top = edge;
		} else if (ofVertexSweepComparer()(bottom.getOrigin(), edge.getOrigin())) {
			bottom = edge;
		}

	} while (halfEdgesIterator.moveNext());
}

void labelChains(const ofDoublyConnectedEdgeList::HalfEdge & top, const ofDoublyConnectedEdgeList::HalfEdge & bottom) {
	auto edge = top;

	// Label left chain.
	do {
		edge.getOrigin().setChain(ofDoublyConnectedEdgeList::Chain::Left);
		edge = edge.getNext();
	} while (edge != bottom);

	// Label right chain.
	do {
		edge.getOrigin().setChain(ofDoublyConnectedEdgeList::Chain::Right);
		edge = edge.getNext();
	} while (edge != top);
}

void ofTriangulateMonotone::sortSweepMonotone(
	std::vector<ofDoublyConnectedEdgeList::Vertex> & vertices,
	ofDoublyConnectedEdgeList::HalfEdge & top,
	ofDoublyConnectedEdgeList::HalfEdge & bottom) {
	// We use a queue for the left chain as we'll receive top vertices first.
	// We use a stack for the right chain as we'll receive bottom vertices first.
	// To merge we'll start from the top.
	assert(m_SweepQueue.empty());
	assert(m_SweepStack.empty());

	auto edge = top;

	// Left chain goes in the queue.
	do {
		m_SweepQueue.push(edge.getOrigin());
		edge = edge.getNext();
	} while (edge != bottom);

	// Right chain goes in the stack.
	do {
		m_SweepStack.push(edge.getOrigin());
		edge = edge.getNext();
	} while (edge != top);

	// Merge chains.
	while (!m_SweepQueue.empty() && !m_SweepStack.empty()) {
		if (ofVertexSweepComparer()(m_SweepQueue.front(), m_SweepStack.top())) {
			vertices.push_back(m_SweepQueue.front());
			m_SweepQueue.pop();
		} else {
			vertices.push_back(m_SweepStack.top());
			m_SweepStack.pop();
		}
	}

	// Add remaining vertices if any.
	while (!m_SweepQueue.empty()) {
		vertices.push_back(m_SweepQueue.front());
		m_SweepQueue.pop();
	}

	while (!m_SweepStack.empty()) {
		vertices.push_back(m_SweepStack.top());
		m_SweepStack.pop();
	}
}

template <class T>
void clearStack(std::stack<T> & stack) {
	while (!stack.empty()) {
		stack.pop();
	}
}

void ofTriangulateMonotone::execute(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face & face) {
	ofDoublyConnectedEdgeList::HalfEdge top;
	ofDoublyConnectedEdgeList::HalfEdge bottom;
	getTopAndBottomVertices(face, top, bottom);

	// Label each vertex with the chain (left or right) it belongs to.
	labelChains(top, bottom);

	sortSweepMonotone(m_Vertices, top, bottom);

	// The stack holds vertices we still (possibly) have edges to connect to.
	m_VertexStack.push(VertexAndIndex(m_Vertices[0], 0));
	m_VertexStack.push(VertexAndIndex(m_Vertices[1], 1));

	for (auto i = 2; i != m_Vertices.size() - 1; ++i) {
		// If current vertex and the vertex on top of stack are on different chains.
		if (m_Vertices[i].getChain() != m_VertexStack.top().first.getChain()) {
			// For all vertices on stack except the last one,
			// for it is connected to the current vertex by an edge.
			while (m_VertexStack.size() > 1) {
				m_PendingDiagonalVertices.push(m_VertexStack.top().first);
				m_VertexStack.pop();
			}

			// We care about the order we want diagonals to be added top to bottom,
			// it allows us to reassign half-edges to vertices properly when splitting.
			while (!m_PendingDiagonalVertices.empty()) {
				auto vertex = m_PendingDiagonalVertices.top();
				m_PendingDiagonalVertices.pop();

				auto edgeAssign = getEdgeAssign(vertex, m_Vertices[i]);
				dcel.splitFace(vertex.getIncidentEdge(), m_Vertices[i].getIncidentEdge(), edgeAssign);
			}

			clearStack(m_VertexStack);

			// Push current vertex and its predecessor on the stack.
			m_VertexStack.push(VertexAndIndex(m_Vertices[i - 1], i - 1));
			m_VertexStack.push(VertexAndIndex(m_Vertices[i], i));
		} else {
			// Pop one vertex from the stack, as it shares an edge with the current vertex.
			auto lastPopped = m_VertexStack.top();
			m_VertexStack.pop();

			// Pop the other vertices while the diagonal from them to the current vertex is inside the polygon.
			// Is the vertex at the top of the stack visible from the current vertex?
			// We can deduce that knowing the previously popped vertex.
			while (!m_VertexStack.empty() && isInside(m_Vertices[i], m_Vertices[i].getChain(), m_VertexStack.top().first, lastPopped.first)) {
				auto topVertex = m_VertexStack.top().first;
				auto edgeAssign = getEdgeAssign(m_Vertices[i], topVertex);
				dcel.splitFace(m_Vertices[i].getIncidentEdge(), topVertex.getIncidentEdge(), edgeAssign);
				lastPopped = m_VertexStack.top();
				m_VertexStack.pop();
			}

			// Push the last vertex that has been popped back onto the stack.
			m_VertexStack.push(lastPopped);

			// Push the current vertex on the stack.
			m_VertexStack.push(VertexAndIndex(m_Vertices[i], i));
		}
	}

	// Add diagonals from the last vertex to all vertices on the stack except the first and the last one.
	m_VertexStack.pop();
	clearStack(m_PendingDiagonalVertices);

	while (m_VertexStack.size() > 1) {
		m_PendingDiagonalVertices.push(m_VertexStack.top().first);
		m_VertexStack.pop();
	}

	// We care about the order we want diagonals to be added top to bottom,
	// it allows us to reassign half-edges to vertices properly when splitting.
	while (!m_PendingDiagonalVertices.empty()) {
		auto vertex = m_PendingDiagonalVertices.top();
		m_PendingDiagonalVertices.pop();

		auto angle = glm::orientedAngle(glm::vec2(0, 1), glm::normalize(vertex.getIncidentEdge().getDirection()));
		auto edgeAssign = angle > 0.0f ? 
			ofDoublyConnectedEdgeList::EdgeAssign::Origin : 
			ofDoublyConnectedEdgeList::EdgeAssign::Destination;
		dcel.splitFace(vertex.getIncidentEdge(), m_Vertices.back().getIncidentEdge(), edgeAssign);
	}

	m_Vertices.clear();
	clearStack(m_VertexStack);
	clearStack(m_PendingDiagonalVertices);
}
