#pragma once

#include "ofSplitToMonotone.h"
#include "ofTriangulateMonotone.h"

/// @brief A class implementing polygon triangulation.
class ofPolygonTriangulation {
public:
	/// @brief Triangulates a doubly connected edge list.
	/// @param dcel The doubly connected edge list.
	///
	/// Triangulation will occur in two steps.
	/// First the doubly connected edge list is split into monotone polygons.
	/// Then the monotone polygons are triangulated.
	void execute(ofDoublyConnectedEdgeList & dcel);

private:
	ofSplitToMonotone m_SplitToMonotone;
	ofTriangulateMonotone m_TriangulateMonotone;
	std::stack<ofDoublyConnectedEdgeList::Face> m_FacesPendingTriangulation;
};
