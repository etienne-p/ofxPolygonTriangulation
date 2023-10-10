#pragma once

#include "ofDoublyConnectedEdgeList.h"

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
