#pragma once

#include <map>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include "DoublyConnectedEdgeList.h"
#include "HalfEdgeSweepComparer.h"

class SplitToMonotone
{
public:
	// TODO Exposed for tests?
	// Used to classify vertices when splitting a polygon to monotone polygons.
	enum class VertexType
	{
		Start,
		Stop,
		Split,
		Merge,
		Regular
	};

	class SweepStatus
	{
	public:
		float getSweepLineY() const { return m_SweepLineY; }
		void setSweepLineY(float sweepLineY) { m_SweepLineY = sweepLineY; }

		void clear()
		{
			m_EdgeToHelperMap.clear();
			m_EdgeToPrevMap.clear();
			m_HalfEdges.clear();
		}

		DoublyConnectedEdgeList::Vertex getHelper(const DoublyConnectedEdgeList::HalfEdge& edge)
		{
			auto it = m_EdgeToHelperMap.find(edge);

			if (it == m_EdgeToHelperMap.end())
			{
				throw std::runtime_error("Could not find helper.");
			}

			return it->second;
		}

		typedef typename std::map<DoublyConnectedEdgeList::HalfEdge, DoublyConnectedEdgeList::Vertex>::const_iterator EdgesAndHelpersIterator;

		// Exposed so that external code may iterate.
		EdgesAndHelpersIterator getEdgesAndHelpersBeginIterator() const { return m_EdgeToHelperMap.begin(); }
		EdgesAndHelpersIterator getEdgesAndHelpersEndIterator() const { return m_EdgeToHelperMap.end(); }

		DoublyConnectedEdgeList::HalfEdge findLeft(DoublyConnectedEdgeList::Vertex vertex)
		{
			// Reverse order as rightmost edges will be at the end of the list.
			for (auto i = m_HalfEdges.size() - 1; i != -1; --i)
			{
				auto edge = m_HalfEdges[i];
				if (edge.getDestination() == vertex)
				{
					continue;
				}

				bool intersectionFound;
				auto intersect = HalfEdgeSweepComparer::sweepIntersection(edge, m_SweepLineY, intersectionFound);
				if (intersectionFound && vertex.getX() > intersect.x)
				{
					return edge;
				}
			}

			throw std::runtime_error("Could not find left edge.");
		}

		void emplace(DoublyConnectedEdgeList::HalfEdge edge, DoublyConnectedEdgeList::Vertex helper)
		{
			m_EdgeToHelperMap.emplace(edge, helper);
			m_HalfEdges.push_back(edge);
			std::sort(m_HalfEdges.begin(), m_HalfEdges.end(), HalfEdgeSweepComparer(m_SweepLineY));
		}

		void remove(DoublyConnectedEdgeList::HalfEdge edge)
		{
			m_EdgeToHelperMap.erase(edge);
			m_HalfEdges.erase(std::remove(m_HalfEdges.begin(), m_HalfEdges.end(), edge), m_HalfEdges.end());
		}

		void updateHelper(DoublyConnectedEdgeList::HalfEdge edge, DoublyConnectedEdgeList::Vertex helper)
		{
			auto it = m_EdgeToHelperMap.find(edge);

			if (it == m_EdgeToHelperMap.end())
			{
				throw std::runtime_error("Could not find helper edge for update.");
			}

			m_EdgeToHelperMap[edge] = helper;
		}

	private:
		std::map<DoublyConnectedEdgeList::HalfEdge, DoublyConnectedEdgeList::Vertex> m_EdgeToHelperMap;
		std::map<DoublyConnectedEdgeList::HalfEdge, DoublyConnectedEdgeList::HalfEdge> m_EdgeToPrevMap;
		std::vector<DoublyConnectedEdgeList::HalfEdge> m_HalfEdges;
		float m_SweepLineY;
	};

	void execute(DoublyConnectedEdgeList& dcel, DoublyConnectedEdgeList::Face& face);

private:
	SweepStatus m_SweepStatus;
	std::map<std::size_t, VertexType> m_VerticesClassification;
	std::vector<DoublyConnectedEdgeList::Vertex> m_Vertices;
};

