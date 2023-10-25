/// \file ofSplitToMonotone.h

#pragma once

#include "ofHalfEdgeSweepComparer.h"
#include <algorithm>
#include <limits>
#include <unordered_map>
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

	/// @brief Classify a vertex for splitting to monotone.
	/// @param vertex The vertex.
	/// @return The classification.
	///
	/// This is mainly exposed for unit tests.
	static VertexType classifyVertex(const ofDoublyConnectedEdgeList::Vertex & vertex);

	/// @brief Split a face of a doubly connected edge list into monotone polygons.
	/// @param dcel The doubly connected edge list.
	/// @param face The face.
	void execute(ofDoublyConnectedEdgeList & dcel, ofDoublyConnectedEdgeList::Face & face);

private:
	class SweepLineStatus {
	public:
		float getCoordinate() const { return m_Coordinate; }
		void setCoordinate(float coordinate) { m_Coordinate = coordinate; }

		void clear() {
			m_EdgeToHelperMap.clear();
			m_HalfEdges.clear();
		}

		ofDoublyConnectedEdgeList::Vertex getHelper(const ofDoublyConnectedEdgeList::HalfEdge & edge) {
			auto it = m_EdgeToHelperMap.find(edge);

			if (it == m_EdgeToHelperMap.end()) {
				throw std::runtime_error("Could not find helper.");
			}

			return it->second;
		}

		// Find the edge directly to the left.
		ofDoublyConnectedEdgeList::HalfEdge findLeft(ofDoublyConnectedEdgeList::Vertex vertex) {
			auto oneOrMoreIntersectionFound = false;
			auto lastIntersectX = -std::numeric_limits<float>::max();
			ofDoublyConnectedEdgeList::HalfEdge leftEdge;

			for (auto it = m_HalfEdges.begin(); it != m_HalfEdges.end(); ++it) {
				auto edge = *it;
				if (edge.getDestination() == vertex) {
					continue;
				}

				auto intersectionFound = false;
				auto intersect = ofHalfEdgeSweepComparer::sweepIntersection(edge, m_Coordinate, intersectionFound);
				if (intersectionFound && vertex.getX() > intersect.x && intersect.x > lastIntersectX) {
					oneOrMoreIntersectionFound = true;
					lastIntersectX = intersect.x;
					leftEdge = edge;
				}
			}

			if (!oneOrMoreIntersectionFound) {
				throw std::runtime_error("Could not find left edge.");
			}

			return leftEdge;
		}

		void emplace(ofDoublyConnectedEdgeList::HalfEdge edge, ofDoublyConnectedEdgeList::Vertex helper) {
			m_EdgeToHelperMap.emplace(edge, helper);
			// Keep the collection sorted.
			auto insertion = std::upper_bound(m_HalfEdges.begin(), m_HalfEdges.end(), edge, ofHalfEdgeSweepComparer(m_Coordinate));
			m_HalfEdges.insert(insertion, edge);
		}

		void remove(ofDoublyConnectedEdgeList::HalfEdge edge) {
			m_EdgeToHelperMap.erase(edge);
			auto removal = std::remove(m_HalfEdges.begin(), m_HalfEdges.end(), edge);
			m_HalfEdges.erase(removal, m_HalfEdges.end());
		}

		void updateHelper(ofDoublyConnectedEdgeList::HalfEdge edge, ofDoublyConnectedEdgeList::Vertex helper) {
			auto it = m_EdgeToHelperMap.find(edge);

			if (it == m_EdgeToHelperMap.end()) {
				throw std::runtime_error("Could not find helper edge for update.");
			}

			m_EdgeToHelperMap[edge] = helper;
		}

	private:
		// We know that the index is enough to compute the key's hash.
		struct ConstantHalfEdgeHash {
			std::size_t operator()(ofDoublyConnectedEdgeList::HalfEdge const & s) const {
				return s.getIndex();
			}
		};

		std::unordered_map<ofDoublyConnectedEdgeList::HalfEdge, ofDoublyConnectedEdgeList::Vertex, ConstantHalfEdgeHash> m_EdgeToHelperMap;
		std::vector<ofDoublyConnectedEdgeList::HalfEdge> m_HalfEdges;
		float m_Coordinate;
	};

	SweepLineStatus m_SweepLineStatus;
	std::vector<VertexType> m_VerticesClassification;
	std::vector<ofDoublyConnectedEdgeList::Vertex> m_Vertices;

	void diagonalToPreviousEdgeHelper(
		ofDoublyConnectedEdgeList & dcel,
		ofDoublyConnectedEdgeList::Vertex & vertex);

	void diagonalToLeftEdgeHelper(
		ofDoublyConnectedEdgeList & dcel,
		ofDoublyConnectedEdgeList::Vertex & vertex);
};
