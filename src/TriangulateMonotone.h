#pragma once

#include "DoublyConnectedEdgeList.h"
#include <queue>
#include <stack>
#include <utility>
#include <vector>

class TriangulateMonotone {
public:
	void execute(DoublyConnectedEdgeList & dcel, DoublyConnectedEdgeList::Face & face);

private:
	std::stack<DoublyConnectedEdgeList::Vertex> m_PendingDiagonalVertices;
	std::vector<DoublyConnectedEdgeList::Vertex> m_Vertices;
	std::stack<std::pair<DoublyConnectedEdgeList::Vertex, std::size_t>> m_VertexStack;

	std::stack<DoublyConnectedEdgeList::Vertex> m_SweepStack;
	std::queue<DoublyConnectedEdgeList::Vertex> m_SweepQueue;

	void sortSweepMonotone(
		std::vector<DoublyConnectedEdgeList::Vertex> & vertices,
		DoublyConnectedEdgeList::HalfEdge & top,
		DoublyConnectedEdgeList::HalfEdge & bottom);
};
