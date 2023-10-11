#pragma once

#include "ofDoublyConnectedEdgeList.h"

/// @brief A functor implementing the comparison of vertices.
struct ofVertexSweepComparer {
	inline bool operator()(
		const ofDoublyConnectedEdgeList::Vertex & first,
		const ofDoublyConnectedEdgeList::Vertex & last) {
		// Strict comparison is deliberate.
		if (first.getY() == last.getY()) {
			return first.getX() < last.getX();
		}

		// On purpose, we go in descending Y order.
		return last.getY() < first.getY();
	}
};
