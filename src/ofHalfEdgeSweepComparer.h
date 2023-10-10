#pragma once

#include "ofDoublyConnectedEdgeList.h"

struct ofHalfEdgeSweepComparer {
public:
	static glm::vec2 sweepIntersection(const ofDoublyConnectedEdgeList::HalfEdge & edge, float sweepY, bool & found) {
		return sweepIntersection(edge.getOrigin().getPosition(), edge.getDestination().getPosition(), sweepY, found);
	}

	ofHalfEdgeSweepComparer(float sweepLineY)
		: m_SweepLineY(sweepLineY) { }

	inline bool operator()(
		const ofDoublyConnectedEdgeList::HalfEdge & first,
		const ofDoublyConnectedEdgeList::HalfEdge & last) {
		bool _;
		auto leftX = sweepIntersection(first, m_SweepLineY, _).x;
		auto rightX = sweepIntersection(last, m_SweepLineY, _).x;
		return leftX < rightX;
	}

private:
	const float m_SweepLineY;

	static glm::vec2 sweepIntersection(glm::vec2 origin, glm::vec2 destination, float sweepY, bool & found) {
		auto r = (origin.y - sweepY) / (origin.y - destination.y);

		// add a bit of tolerance as some vertices may have the same Y coordinate
		if (r > 1 || r < 0) {
			found = false;
			return glm::vec2(0.0f, 0.0f);
		}

		found = true;
		return glm::mix(origin, destination, r);
	}
};
