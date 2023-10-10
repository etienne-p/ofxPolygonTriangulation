#pragma once

#include "ofDoublyConnectedEdgeList.h"
#include <queue>
#include <stack>
#include <utility>
#include <vector>

class ofTriangulateMonotone {
public:
	void execute(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face & face);

private:
	std::stack<ofDoublyConnectedEdgeList::Vertex> m_PendingDiagonalVertices;
	std::vector<ofDoublyConnectedEdgeList::Vertex> m_Vertices;
	std::stack<std::pair<ofDoublyConnectedEdgeList::Vertex, std::size_t>> m_VertexStack;

	std::stack<ofDoublyConnectedEdgeList::Vertex> m_SweepStack;
	std::queue<ofDoublyConnectedEdgeList::Vertex> m_SweepQueue;

	void sortSweepMonotone(
		std::vector<ofDoublyConnectedEdgeList::Vertex> & vertices,
		ofDoublyConnectedEdgeList::HalfEdge & top,
		ofDoublyConnectedEdgeList::HalfEdge & bottom);
};
