#pragma once

#include "ofDoublyConnectedEdgeList.h"
#include "ofHalfEdgeSweepComparer.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>

class ofSplitToMonotone {
public:
	// TODO Exposed for tests?
	// Used to classify vertices when splitting a polygon to monotone polygons.
	enum class VertexType {
		Start,
		Stop,
		Split,
		Merge,
		Regular
	};

	class SweepStatus {
	public:
		float getSweepLineY() const { return m_SweepLineY; }
		void setSweepLineY(float sweepLineY) { m_SweepLineY = sweepLineY; }

		void clear() {
			m_EdgeToHelperMap.clear();
			m_EdgeToPrevMap.clear();
			m_HalfEdges.clear();
		}

		ofDoublyConnectedEdgeList::Vertex getHelper(const ofDoublyConnectedEdgeList::HalfEdge & edge) {
			auto it = m_EdgeToHelperMap.find(edge);

			if (it == m_EdgeToHelperMap.end()) {
				throw std::runtime_error("Could not find helper.");
			}

			return it->second;
		}

		typedef typename std::map<ofDoublyConnectedEdgeList::HalfEdge, ofDoublyConnectedEdgeList::Vertex>::const_iterator EdgesAndHelpersIterator;

		// Exposed so that external code may iterate.
		EdgesAndHelpersIterator getEdgesAndHelpersBeginIterator() const { return m_EdgeToHelperMap.begin(); }
		EdgesAndHelpersIterator getEdgesAndHelpersEndIterator() const { return m_EdgeToHelperMap.end(); }

		ofDoublyConnectedEdgeList::HalfEdge findLeft(ofDoublyConnectedEdgeList::Vertex vertex) {
			// Reverse order as rightmost edges will be at the end of the list.
			for (auto i = m_HalfEdges.size() - 1; i != -1; --i) {
				auto edge = m_HalfEdges[i];
				if (edge.getDestination() == vertex) {
					continue;
				}

				bool intersectionFound;
				auto intersect = ofHalfEdgeSweepComparer::sweepIntersection(edge, m_SweepLineY, intersectionFound);
				if (intersectionFound && vertex.getX() > intersect.x) {
					return edge;
				}
			}

			throw std::runtime_error("Could not find left edge.");
		}

		void emplace(ofDoublyConnectedEdgeList::HalfEdge edge, ofDoublyConnectedEdgeList::Vertex helper) {
			m_EdgeToHelperMap.emplace(edge, helper);
			m_HalfEdges.push_back(edge);
			std::sort(m_HalfEdges.begin(), m_HalfEdges.end(), ofHalfEdgeSweepComparer(m_SweepLineY));
		}

		void remove(ofDoublyConnectedEdgeList::HalfEdge edge) {
			m_EdgeToHelperMap.erase(edge);
			m_HalfEdges.erase(std::remove(m_HalfEdges.begin(), m_HalfEdges.end(), edge), m_HalfEdges.end());
		}

		void updateHelper(ofDoublyConnectedEdgeList::HalfEdge edge, ofDoublyConnectedEdgeList::Vertex helper) {
			auto it = m_EdgeToHelperMap.find(edge);

			if (it == m_EdgeToHelperMap.end()) {
				throw std::runtime_error("Could not find helper edge for update.");
			}

			m_EdgeToHelperMap[edge] = helper;
		}

	private:
		std::map<ofDoublyConnectedEdgeList::HalfEdge, ofDoublyConnectedEdgeList::Vertex> m_EdgeToHelperMap;
		std::map<ofDoublyConnectedEdgeList::HalfEdge, ofDoublyConnectedEdgeList::HalfEdge> m_EdgeToPrevMap;
		std::vector<ofDoublyConnectedEdgeList::HalfEdge> m_HalfEdges;
		float m_SweepLineY;
	};

	void execute(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face & face);

private:
	SweepStatus m_SweepStatus;
	std::map<std::size_t, VertexType> m_VerticesClassification;
	std::vector<ofDoublyConnectedEdgeList::Vertex> m_Vertices;
};
