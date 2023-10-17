/// \file ofDoublyConnectedEdgeList.h
#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

/// \brief Winding order of a polygon vertices.
enum class ofPolygonWindingOrder {
	/// @brief Vertices are in undetermined order.
	Undefined,
	/// @brief Vertices are in clockwise order.
	ClockWise,
	/// @brief Vertices are in counter clockWise order.
	CounterClockWise,
};

/// @brief A class implementing a doubly connected edge list data structure.
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
	/// @brief Identifies vertex chains on monotone polygons.
	enum class Chain {
		/// @brief The chain is undetermined.
		None,
		/// @brief Right chain.
		Right,
		/// @brief Left chain.
		Left,
		/// @brief Top vertex.
		Top,
		/// @brief Bottom vertex.
		Bottom
	};

	// Forward declaration as Vertex, HalfEdge and Face depend on one another.
	struct HalfEdge;
	struct Face;

	/// @brief A handle to a vertex.
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

		HalfEdge getIncidentEdge() const;
		void setIncidentEdge(const HalfEdge & halfEdge);

	private:
		std::size_t m_Index;
		ofDoublyConnectedEdgeList * m_Dcel;
	};

	/// @brief A handle to an half edge.
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

		Vertex getOrigin() const;
		void setOrigin(const Vertex & vertex);

		inline Vertex getDestination() const { return getTwin().getOrigin(); }

		Face getIncidentFace() const;
		void setIncidentFace(const Face & face);

		glm::vec2 getDirection() const;

	private:
		std::size_t m_Index;
		ofDoublyConnectedEdgeList * m_Dcel;
	};

	/// @brief A handle to a face.
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

	bool tryFindSharedFace(
		const Vertex & vertexA, const Vertex & vertexB,
		HalfEdge & halfEdgeA, HalfEdge & halfEdgeB) const;

	// Private template, DRY but safe API.
	template <class vecN>
	void initializeFromCCWVertices(const std::vector<vecN> & vertices);
	template <class vecN>
	static ofPolygonWindingOrder getWindingOrder(const std::vector<vecN> & vertices);

	HalfEdge createEdge();
	Face createFace();
	// As we initialize a DCEL, we start with 2 faces.
	// Inside the polygon and outside of it, respectively.
	// The indices we choose are a convention on our part.
	static constexpr int k_OuterFaceIndex = 0;
	static constexpr int k_InnerFaceIndex = 1;

	std::vector<VertexData> m_Vertices;
	std::vector<HalfEdgeData> m_Edges;
	std::vector<FaceData> m_Faces;

public:
	/// @brief Exposes the inner face of the doubly connected edge list.
	/// @return The inner face of the doubly connected edge list.
	Face getInnerFace();

	/// @brief Initializes the doubly connected edge list from a list of points representing a polygon.
	/// @param vertices A vector of polygon points in 2 dimensions.
	///
	/// Points are expected to be sorted in count clockwise order.
	void initializeFromCCWVertices(const std::vector<glm::vec2> & vertices);

	/// @brief Initializes the doubly connected edge list from a list of points representing a polygon.
	/// @param vertices A vector of polygon points in 3 dimensions.
	///
	/// Points are expected to be sorted in count clockwise order.
	/// The 3rd dimensions is ignored and accepted as a parameter for compatibility reasons.
	void initializeFromCCWVertices(const std::vector<glm::vec3> & vertices);

	/// @brief Adds an half edge connecting two vertices.
	/// @param vertexA The first vertex.
	/// @param vertexB The second vertex.
	/// @return The newly created half edge.
	HalfEdge addHalfEdge(Vertex & vertexA, Vertex & vertexB);

	/// @brief Adds an half edge connecting two half edges.
	/// @param edgeA The first half edge.
	/// @param edgeB The second half edge.
	/// @return The newly created half edge.
	HalfEdge addHalfEdge(HalfEdge & edgeA, HalfEdge & edgeB);

	/// @brief Write the doubly connected edge list topology in arrays of vertices and indices.
	/// @param vertices The geometry vertices.
	/// @param indices The geometry indices.
	///
	/// Intended for rendering.
	/// The doubly connected edge list must have been triangulated beforehand.
	void extractTriangles(std::vector<glm::vec3> & vertices, std::vector<unsigned int> & indices);

	/// @brief Returns the index of the outer face.
	constexpr static int getOuterFaceIndex() { return k_OuterFaceIndex; }

	/// @brief Returns the index of the inner face.
	constexpr static int getInnerFaceIndex() { return k_InnerFaceIndex; }

	/// @brief Evaluates the winding order of the vertices of a face.
	/// @param face The face.
	/// @return The winding order of the face
	static ofPolygonWindingOrder getWindingOrder(const Face & face);

	/// @brief Evaluates the winding order of the vertices (2d) of a polygon.
	/// @param vertices The vertices of the polygon.
	/// @return The winding order of the polygon
	static ofPolygonWindingOrder getWindingOrder(const std::vector<glm::vec2> & vertices);

	/// @brief Evaluates the winding order of the vertices (3d) of a polygon.
	/// @param vertices The vertices of the polygon.
	/// @return The winding order of the polygon
	///
	/// The 3rd dimensions is ignored and accepted as a parameter for compatibility reasons.
	static ofPolygonWindingOrder getWindingOrder(const std::vector<glm::vec3> & vertices);

	/// @brief A utility to iterate over the half edges of a face.
	///
	/// Note that this is not an iterator as understood by the standard library.
	struct HalfEdgesIterator {
	private:
		const std::size_t m_InitialIndex;
		HalfEdge m_Current;

	public:
		HalfEdgesIterator(const HalfEdge & halfEdge)
			: m_Current(halfEdge)
			, m_InitialIndex(halfEdge.getIndex()) { }
		HalfEdgesIterator(const Face & face)
			: m_Current(face.getOuterComponent())
			, m_InitialIndex(face.getOuterComponent().getIndex()) { }
		inline HalfEdge getCurrent() const { return m_Current; }
		inline bool moveNext() {
			m_Current = m_Current.getNext();
			if (m_Current.getIndex() == m_InitialIndex) {
				return false;
			}
			return true;
		};
	};

	/// @brief A utility to iterate over the faces of the doubly connected edge list.
	///
	/// Note that this is not an iterator as understood by the standard library.
	/// The outer face is bypassed.
	struct FacesIterator {
	private:
		ofDoublyConnectedEdgeList * m_Dcel;
		Face m_Current;
		std::size_t m_Index;

	public:
		// Note that we bypass the outer face by starting at the inner index.
		FacesIterator(ofDoublyConnectedEdgeList & dcel)
			: m_Dcel(&dcel)
			, m_Current(Face(m_Dcel, k_InnerFaceIndex))
			, m_Index(k_InnerFaceIndex) { }
		inline Face getCurrent() const { return m_Current; }
		inline bool moveNext() {
			if (m_Index < m_Dcel->m_Faces.size() - 1) {
				++m_Index;
				m_Current = Face(m_Dcel, m_Index);
				return true;
			}

			return false;
		}
	};

	/// @brief A utility to iterate over the faces a vertex belongs to.
	///
	/// Note that this is not an iterator as understood by the standard library.
	/// The outer face is bypassed.
	struct FacesOnVertexIterator {
	private:
		const std::size_t m_IncidentEdgeIndex;
		HalfEdge m_Current;

	public:
		FacesOnVertexIterator(const Vertex & vertex)
			: m_IncidentEdgeIndex(vertex.getIncidentEdge().getIndex())
			, m_Current(vertex.getIncidentEdge()) {
			assert(m_Current.getIncidentFace().getIndex() != k_OuterFaceIndex);
		}
		inline HalfEdge getCurrent() const { return m_Current; }
		inline bool moveNext() {
			m_Current = m_Current.getPrev().getTwin();
			// Skip outer face.
			if (m_Current.getIncidentFace().getIndex() == k_OuterFaceIndex) {
				m_Current = m_Current.getPrev().getTwin();
			}
			return m_Current.getIndex() != m_IncidentEdgeIndex;
		}
	};
};
