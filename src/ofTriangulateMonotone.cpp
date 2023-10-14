#include <cassert>
#include <glm/gtx/vector_angle.hpp>
#include "ofTriangulateMonotone.h"
#include "ofVertexSweepComparer.h"

bool isInside(
	const ofDoublyConnectedEdgeList::Vertex & vertex,
	const ofDoublyConnectedEdgeList::Vertex & stackTop,
	const ofDoublyConnectedEdgeList::Vertex & lastPopped) {
	// Equivalent to calculating the interior angle.
	auto angle = glm::orientedAngle(
		glm::normalize(lastPopped.getPosition() - vertex.getPosition()),
		glm::normalize(stackTop.getPosition() - vertex.getPosition()));

	if (vertex.getChain() == ofDoublyConnectedEdgeList::Chain::Left) {
		return angle < 0.0f;
	}
	return angle > 0.0f;
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
	// Label top vertex;
	auto edge = top;
	edge.getOrigin().setChain(ofDoublyConnectedEdgeList::Chain::Top);
	edge = edge.getNext();

	// Label left chain.
	while (edge != bottom) {
		edge.getOrigin().setChain(ofDoublyConnectedEdgeList::Chain::Left);
		edge = edge.getNext();
	}

	// Label bottom vertex;
	edge.getOrigin().setChain(ofDoublyConnectedEdgeList::Chain::Bottom);
	edge = edge.getNext();

	// Label right chain.
	while (edge != top) {
		edge.getOrigin().setChain(ofDoublyConnectedEdgeList::Chain::Right);
		edge = edge.getNext();
	}
}

void ofTriangulateMonotone::sortSweepMonotone(
	std::vector<ofDoublyConnectedEdgeList::Vertex> & vertices,
	ofDoublyConnectedEdgeList::HalfEdge & top,
	ofDoublyConnectedEdgeList::HalfEdge & bottom) {
	// We use a queue for the left chain as we'll receive top vertices first.
	// We use a stack for the right chain as we'll receive bottom vertices first.
	// To merge we'll start from the top.
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

void ofTriangulateMonotone::execute(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face & face) {
	ofDoublyConnectedEdgeList::HalfEdge top;
	ofDoublyConnectedEdgeList::HalfEdge bottom;
	getTopAndBottomVertices(face, top, bottom);

	// Label each vertex with the chain (left or right) it belongs to.
	labelChains(top, bottom);

	sortSweepMonotone(m_Vertices, top, bottom);

	// The stack holds vertices we still (possibly) have edges to connect to.
	m_VertexStack.push(m_Vertices[0]);
	m_VertexStack.push(m_Vertices[1]);

	for (auto i = 2; i != m_Vertices.size() - 1; ++i) {
		// If current vertex and the vertex on top of stack are on different chains.
		if (m_Vertices[i].getChain() != m_VertexStack.top().getChain()) {
			// Add a diagonal for all vertices on the stack except the last one,
			// for it is connected to the current vertex by an edge.
			while (m_VertexStack.size() > 1) {
				dcel.splitFace(m_Vertices[i], m_VertexStack.top());
				m_VertexStack.pop();
			}
			// Clear vertex stack, last vertex is not connected.
			m_VertexStack.pop();

			// Push current vertex and its predecessor on the stack.
			m_VertexStack.push(m_Vertices[i - 1]);
			m_VertexStack.push(m_Vertices[i]);
		} else {
			// Pop one vertex from the stack, as it shares an edge with the current vertex.
			auto lastPopped = m_VertexStack.top();
			m_VertexStack.pop();

			// Pop the other vertices while the diagonal from them to the current vertex is inside the polygon.
			// Is the vertex at the top of the stack visible from the current vertex?
			// We can deduce that knowing the previously popped vertex.
			while (!m_VertexStack.empty() && isInside(m_Vertices[i], m_VertexStack.top(), lastPopped)) {
				dcel.splitFace(m_Vertices[i], m_VertexStack.top());
				lastPopped = m_VertexStack.top();
				m_VertexStack.pop();
			}

			// Push the last vertex that has been popped back onto the stack.
			m_VertexStack.push(lastPopped);

			// Push the current vertex on the stack.
			m_VertexStack.push(m_Vertices[i]);
		}
	}

	// Add diagonals from the last vertex to all vertices on the stack except the first and the last one.
	m_VertexStack.pop();

	while (m_VertexStack.size() > 1) {
		dcel.splitFace(m_Vertices.back(), m_VertexStack.top());
		m_VertexStack.pop();
	}
	// Clear vertex stack, last vertex is not connected.
	m_VertexStack.pop();

	m_Vertices.clear();
}
