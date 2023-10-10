#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

// Winding order.
enum class WindingOrder {
	None,
	ClockWise,
	CounterClockWise,
};

class ofDoublyConnectedEdgeList {
private:
	template <typename T>
	struct IndexEquality {
		// < operator needed as we'll use the type as key in maps.
		friend inline bool operator<(const T & lhs, const T & rhs) { return lhs.getIndex() < rhs.getIndex(); }
		friend inline bool operator==(const T & lhs, const T & rhs) { return lhs.getIndex() == rhs.getIndex(); }
		friend inline bool operator!=(const T & lhs, const T & rhs) { return !(lhs == rhs); }
	};

public:
	// Identify vertex chains on monotone polygons.
	enum class Chain {
		None,
		Right,
		Left
	};

	// Used for vertex' incident edge reassignment when splitting faces.
	enum class EdgeAssign {
		None,
		Origin,
		Destination
	};

	// Forward declaration as Vertex, HalfEdge and Face depend on one another.
	struct HalfEdge;
	struct Face;

	struct Vertex : IndexEquality<Vertex> {
	public:
		Vertex()
			: m_Dcel(nullptr)
			, m_Index(0) { }
		Vertex(ofDoublyConnectedEdgeList * dcel, std::size_t index)
			: m_Dcel(dcel)
			, m_Index(index) { }

		inline std::size_t getIndex() const { return m_Index; }

		inline glm::vec2 getPosition() const { return m_Dcel->m_Vertices[m_Index].position; }
		inline float getX() const { return getPosition().x; }
		inline float getY() const { return getPosition().y; }

		inline Chain getChain() const { return m_Dcel->m_Vertices[m_Index].chain; }
		inline void setChain(Chain chain) { m_Dcel->m_Vertices[m_Index].chain = chain; }

		inline HalfEdge getIncidentEdge() const;
		inline void setIncidentEdge(const HalfEdge & halfEdge);

	private:
		std::size_t m_Index;
		ofDoublyConnectedEdgeList * m_Dcel;
	};

	struct HalfEdge : IndexEquality<HalfEdge> {
	public:
		HalfEdge()
			: m_Dcel(nullptr)
			, m_Index(0) { }
		HalfEdge(ofDoublyConnectedEdgeList * dcel, std::size_t index)
			: m_Dcel(dcel)
			, m_Index(index) { }
		HalfEdge(const HalfEdge & other)
			: m_Dcel(other.m_Dcel)
			, m_Index(other.m_Index) { }

		inline std::size_t getIndex() const { return m_Index; }

		inline HalfEdge getTwin() const { return HalfEdge(m_Dcel, m_Dcel->m_Edges[m_Index].twin); }
		inline void setTwin(const HalfEdge & halfEdge) { m_Dcel->m_Edges[m_Index].twin = halfEdge.getIndex(); }

		inline HalfEdge getPrev() const { return HalfEdge(m_Dcel, m_Dcel->m_Edges[m_Index].prev); }
		inline void setPrev(const HalfEdge & halfEdge) { m_Dcel->m_Edges[m_Index].prev = halfEdge.getIndex(); }

		inline HalfEdge getNext() const { return HalfEdge(m_Dcel, m_Dcel->m_Edges[m_Index].next); }
		inline void setNext(const HalfEdge & halfEdge) { m_Dcel->m_Edges[m_Index].next = halfEdge.getIndex(); }

		inline Vertex getOrigin() const;
		inline void setOrigin(const Vertex & vertex);

		inline Vertex getDestination() const { return getTwin().getOrigin(); }

		inline Face getIncidentFace() const;
		inline void setIncidentFace(const Face & face);

		glm::vec2 getDirection() const;

	private:
		std::size_t m_Index;
		ofDoublyConnectedEdgeList * m_Dcel;
	};

	struct Face : IndexEquality<Face> {
	public:
		Face()
			: m_Dcel(nullptr)
			, m_Index(0) { }
		Face(ofDoublyConnectedEdgeList * dcel, std::size_t index)
			: m_Dcel(dcel)
			, m_Index(index) { }

		inline std::size_t getIndex() const { return m_Index; }

		inline HalfEdge getOuterComponent() const;
		inline void setOuterComponent(const HalfEdge & halfEdge);

	private:
		std::size_t m_Index;
		ofDoublyConnectedEdgeList * m_Dcel;
	};

	Face getInnerFace();
	void initializeFromCCWVertices(const std::vector<glm::vec2> & vertices);
	void initializeFromCCWVertices(const std::vector<glm::vec3> & vertices);
	HalfEdge splitFace(HalfEdge & edge, Vertex & vertex, EdgeAssign edgeAssign);
	HalfEdge splitFace(HalfEdge & edgeA, HalfEdge & edgeB, EdgeAssign edgeAssign);

private:
	struct VertexData {
		glm::vec2 position;
		Chain chain;
		std::size_t incidentEdge;
	};

	struct FaceData {
		std::size_t outerComponent;
	};

	struct HalfEdgeData {
		std::size_t origin;
		std::size_t incidentFace;
		std::size_t twin;
		std::size_t prev;
		std::size_t next;
	};

	// Private template, DRY but safe API.
	template <class vecN>
	void initializeFromCCWVertices(const std::vector<vecN> & vertices);

	HalfEdge createEdge();
	Face createFace();
	HalfEdge splitFaceInternal(HalfEdge & edgeA, HalfEdge & edgeB, EdgeAssign edgeAssign);

	// As we initialize a DCEL, we start with 2 faces.
	// Inside the polygon and outside of it, respectively.
	// The indices we choose are a convention on our part.
	static constexpr int k_OuterFaceIndex = 0;
	static constexpr int k_InnerFaceIndex = 1;

	std::vector<VertexData> m_Vertices;
	std::vector<HalfEdgeData> m_Edges;
	std::vector<FaceData> m_Faces;

public:
	// TODO template to handle vertex/index types?
	void extractTriangles(std::vector<glm::vec3> & vertices, std::vector<unsigned int> & indices);
	constexpr static int getOuterFaceIndex() { return k_OuterFaceIndex; }
	constexpr static int getInnerFaceIndex() { return k_InnerFaceIndex; }
	static WindingOrder getOrder(const Face & face);

	typedef typename std::vector<VertexData>::const_iterator VertexIterator;
	typedef typename std::vector<FaceData>::const_iterator FaceIterator;

	struct HalfEdgesIterator {
	private:
		bool m_MovedOnce { false };
		const std::size_t m_InitialIndex;
		HalfEdge m_Current;

	public:
		HalfEdgesIterator(const HalfEdge & halfEdge)
			: m_Current(halfEdge)
			, m_InitialIndex(halfEdge.getIndex()) { }
		HalfEdgesIterator(const Face & face)
			: m_Current(face.getOuterComponent())
			, m_InitialIndex(face.getOuterComponent().getIndex()) { }
		HalfEdge getCurrent() const { return m_Current; }
		bool moveNext() {
			if (m_MovedOnce && m_Current.getIndex() == m_InitialIndex) {
				return false;
			}
			m_MovedOnce = true;
			m_Current = m_Current.getNext();
			return true;
		};
	};

	struct FacesIterator {
	private:
		ofDoublyConnectedEdgeList * m_Dcel;
		Face m_Current;
		std::size_t m_Index;

	public:
		FacesIterator(ofDoublyConnectedEdgeList & dcel)
			: m_Dcel(&dcel)
			, m_Current(Face(m_Dcel, 0))
			, m_Index(0) { }
		Face getCurrent() const { return m_Current; }
		bool moveNext() {
			if (m_Index < m_Dcel->m_Faces.size() - 1) {
				++m_Index;
				m_Current = Face(m_Dcel, m_Index);
				return true;
			}

			return false;
		}
	};
};
