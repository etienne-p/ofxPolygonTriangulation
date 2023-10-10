#pragma once

#include "DoublyConnectedEdgeList.h"

struct VertexSweepComparer {
	inline bool operator()(
		const DoublyConnectedEdgeList::Vertex & first,
		const DoublyConnectedEdgeList::Vertex & last) {
		// Strict comparison is deliberate.
		if (first.getY() == last.getY()) {
			return first.getX() < last.getX();
		}

		// On purpose, we go in descending Y order.
		return last.getY() < first.getY();
	}
};
