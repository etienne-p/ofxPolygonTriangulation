#pragma once

#include "ofDoublyConnectedEdgeList.h"
#include <queue>
#include <stack>
#include <utility>
#include <vector>

/// @brief A class implementing the triangulation of monotone polygons.
class ofTriangulateMonotone {
public:
	/// @brief Triangulates a face of a doubly connected edge list.
	/// @param dcel The doubly connected edge list.
	/// @param face The face.
	///
	/// The face is assumed to be monotone.
	void execute(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face & face);

private:
	std::vector<ofDoublyConnectedEdgeList::Vertex> m_Vertices;
	std::stack<ofDoublyConnectedEdgeList::Vertex> m_VertexStack;

	std::stack<ofDoublyConnectedEdgeList::Vertex> m_SweepStack;
	std::queue<ofDoublyConnectedEdgeList::Vertex> m_SweepQueue;

	void sortSweepMonotone(
		std::vector<ofDoublyConnectedEdgeList::Vertex> & vertices,
		ofDoublyConnectedEdgeList::HalfEdge & top,
		ofDoublyConnectedEdgeList::HalfEdge & bottom);
};
