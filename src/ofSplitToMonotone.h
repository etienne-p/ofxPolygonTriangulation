/// \file ofSplitToMonotone.h

#pragma once

#include "ofDoublyConnectedEdgeList.h"
#include "ofHalfEdgeSweepComparer.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>

/// @brief A class implementing the split of a polygon into monotone polygons.
class ofSplitToMonotone {
public:
	/// @brief Vertex classification when splitting a polygon to monotone polygons.
	enum class VertexType {
		/// @brief A vertex whose neighbors lie below and the interior angle is less than pi.
		Start,
		/// @brief A vertex whose neighbors lie above and the interior angle is less than pi.
		Stop,
		/// @brief A vertex whose neighbors lie below and the interior angle is greater than pi.
		Split,
		/// @brief A vertex whose neighbors lie above and the interior angle is greater than pi.
		Merge,
		/// @brief A vertex whose neighbors lie above and below.
		Regular
	};

	/// @brief Split a face of a doubly connected edge list into monotone polygons.
	/// @param dcel The doubly connected edge list.
	/// @param face The face.
	void execute(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face & face);

private:
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

	SweepStatus m_SweepStatus;
	std::map<std::size_t, VertexType> m_VerticesClassification;
	std::vector<ofDoublyConnectedEdgeList::Vertex> m_Vertices;

	void diagonalToPreviousEdgeHelper(
		ofDoublyConnectedEdgeList & dcel,
		ofDoublyConnectedEdgeList::Vertex & vertex);

	void diagonalToLeftEdgeHelper(
		ofDoublyConnectedEdgeList & dcel,
		ofDoublyConnectedEdgeList::Vertex & vertex);
};
