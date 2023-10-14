#include "ofDoublyConnectedEdgeList.h"
#include <cassert>
#include <stdexcept>

// Lighten below code.
using dcel = ofDoublyConnectedEdgeList;

dcel::HalfEdge dcel::Vertex::getIncidentEdge() const {
	return dcel::HalfEdge(m_Dcel, m_Dcel->m_Vertices[m_Index].incidentEdge);
}

void dcel::Vertex::setIncidentEdge(const dcel::HalfEdge & halfEdge) {
	m_Dcel->m_Vertices[m_Index].incidentEdge = halfEdge.getIndex();
}

dcel::Vertex dcel::HalfEdge::getOrigin() const {
	return dcel::Vertex(m_Dcel, m_Dcel->m_Edges[m_Index].origin);
}

void dcel::HalfEdge::setOrigin(const dcel::Vertex & vertex) {
	m_Dcel->m_Edges[m_Index].origin = vertex.getIndex();
}

dcel::Face dcel::HalfEdge::getIncidentFace() const {
	return dcel::Face(m_Dcel, m_Dcel->m_Edges[m_Index].incidentFace);
}

void dcel::HalfEdge::setIncidentFace(const dcel::Face & face) {
	// TODO Temp.
	assert(face.getIndex() != k_OuterFaceIndex);
	m_Dcel->m_Edges[m_Index].incidentFace = face.getIndex();
}

glm::vec2 dcel::HalfEdge::getDirection() const {
	return getDestination().getPosition() - getOrigin().getPosition();
}

dcel::HalfEdge dcel::Face::getOuterComponent() const {
	return dcel::HalfEdge(m_Dcel, m_Dcel->m_Faces[m_Index].outerComponent);
}

inline void dcel::Face::setOuterComponent(const dcel::HalfEdge & halfEdge) {
	m_Dcel->m_Faces[m_Index].outerComponent = halfEdge.getIndex();
}

float cross2d(const glm::vec2 v1, const glm::vec2 v2) {
	return v1.x * v2.y - v2.x * v1.y;
}

ofPolygonWindingOrder signedAreaToWindingOrder(float area) {
	// Handled out of due diligence but unlikely.
	if (area == 0.0f) {
		return ofPolygonWindingOrder::Undefined;
	}

	return area > 0 ? ofPolygonWindingOrder::CounterClockWise : ofPolygonWindingOrder::ClockWise;
}

// https : //demonstrations.wolfram.com/SignedAreaOfAPolygon/
template <class vecN>
float getSignedArea(const std::vector<vecN> & vertices) {
	auto area = 0.0f;
	auto len = vertices.size();

	for (auto i = 0; i != len; ++i) {
		auto p1 = vertices[i];
		auto p2 = vertices[(i + 1) % len];
		area += cross2d(p1, p2);
	}

	return area;
}

float getSignedArea(const dcel::Face & face) {
	auto halfEdgeIterator = dcel::HalfEdgesIterator(face);
	auto area = 0.0f;

	do {
		auto edge = halfEdgeIterator.getCurrent();
		auto p1 = edge.getOrigin().getPosition();
		auto p2 = edge.getDestination().getPosition();
		area += cross2d(p1, p2);
	} while (halfEdgeIterator.moveNext());

	return area;
}

ofPolygonWindingOrder dcel::getWindingOrder(const dcel::Face & face) {
	auto area = getSignedArea(face);
	return signedAreaToWindingOrder(area);
}

template <class vecN>
ofPolygonWindingOrder dcel::getWindingOrder(const std::vector<vecN> & vertices) {
	auto area = getSignedArea(vertices);
	return signedAreaToWindingOrder(area);
}

ofPolygonWindingOrder dcel::getWindingOrder(const std::vector<glm::vec3> & vertices) {
	return dcel::getWindingOrder<glm::vec3>(vertices);
}

ofPolygonWindingOrder dcel::getWindingOrder(const std::vector<glm::vec2> & vertices) {
	return dcel::getWindingOrder<glm::vec2>(vertices);
}

bool dcel::tryFindSharedFace(
	const dcel::Vertex & vertexA, const dcel::Vertex & vertexB,
	dcel::HalfEdge & halfEdgeA, dcel::HalfEdge & halfEdgeB) {
	// TODO Use iterator, bypass outer face with iterator.
	// Iterate over A's faces, try to find a matching face for B.
	halfEdgeA = vertexA.getIncidentEdge();
	halfEdgeB = vertexB.getIncidentEdge();

	auto itA = dcel::FacesOnVertexIterator(vertexA);
	do {
		halfEdgeA = itA.getCurrent();

		auto itB = dcel::FacesOnVertexIterator(vertexB);
		do {
			halfEdgeB = itB.getCurrent();
			if (halfEdgeA.getIncidentFace() == halfEdgeB.getIncidentFace()) {
				return true;
			}

		} while (itB.moveNext());

	} while (itA.moveNext());

	return false;
}

dcel::HalfEdge dcel::createEdge() {
	m_Edges.push_back(HalfEdgeData());
	return HalfEdge(this, m_Edges.size() - 1);
}

dcel::Face dcel::createFace() {
	m_Faces.push_back(FaceData());
	return Face(this, m_Faces.size() - 1);
}

// Leans on a convention, see k_InnerFaceIndex.
dcel::Face dcel::getInnerFace() {
	if (m_Faces.size() < k_InnerFaceIndex + 1) {
		throw std::runtime_error("Cannot access inner face.");
	}
	return dcel::Face(this, k_InnerFaceIndex);
}

template <class vecN>
void dcel::initializeFromCCWVertices(const std::vector<vecN> & vertices) {
#if _DEBUG
	if (getWindingOrder(vertices) != ofPolygonWindingOrder::CounterClockWise) {
		throw std::runtime_error("Passed vertices should be in counter clockwise order.");
	}
#endif

	const auto len = vertices.size();

	m_Vertices.resize(len);
	m_Edges.resize(len * 2); // *2, outer and inner faces, len edges per face.
	m_Faces.resize(2); // 2, outer and inner faces.

	// Outer face.
	m_Faces[k_OuterFaceIndex].outerComponent = len;

	// Inner face.
	m_Faces[k_InnerFaceIndex].outerComponent = 0;

	for (auto i = 0; i != len; ++i) {
		VertexData vert;
		vert.position = vertices[i];
		vert.chain = dcel::Chain::None;
		vert.incidentEdge = i;
		m_Vertices[i] = vert;

		auto prevIndex = (i - 1 + len) % len;
		auto nextIndex = (i + 1) % len;
		auto twinIndex = i + len;

		HalfEdgeData edge;
		edge.origin = i;
		edge.incidentFace = k_InnerFaceIndex;
		edge.twin = twinIndex;
		edge.prev = prevIndex;
		edge.next = nextIndex;
		m_Edges[i] = edge;

		edge.origin = nextIndex;
		edge.incidentFace = k_OuterFaceIndex;
		edge.twin = i;
		edge.prev = nextIndex + len;
		edge.next = prevIndex + len;
		m_Edges[twinIndex] = edge;
	}
}

void dcel::initializeFromCCWVertices(const std::vector<glm::vec2> & vertices) {
	initializeFromCCWVertices<glm::vec2>(vertices);
}

void dcel::initializeFromCCWVertices(const std::vector<glm::vec3> & vertices) {
	initializeFromCCWVertices<glm::vec3>(vertices);
}

void dcel::extractTriangles(
	std::vector<glm::vec3> & vertices, std::vector<unsigned int> & indices) {
	vertices.resize(m_Vertices.size());
	indices.resize((m_Faces.size() - 1) * 3); // -1 to exclude outer face.

	// Iteration starts at 1 to exclude outer face.
	// If the face isn't a triangle, we'll simply ignore vertices beyond the third.
	// It will lead to an incorrect geometry but will not raise errors.
	for (auto i = 1; i != m_Faces.size(); ++i) {
		auto edge = m_Edges[m_Faces[i].outerComponent];
		auto index = (i - 1) * 3;
		indices[index] = edge.origin;
		edge = m_Edges[edge.next];
		indices[index + 1] = edge.origin;
		edge = m_Edges[edge.next];
		indices[index + 2] = edge.origin;
#if _DEBUG

		// Check that the face actually is a triangle.
		if (m_Faces[i].outerComponent != edge.next) {
			//throw std::runtime_error("extractTriangles: encountered non triangular face.");
		}
#endif
	}

	for (auto i = 0; i != m_Vertices.size(); ++i) {
		auto position = m_Vertices[i].position;
		vertices[i] = glm::vec3(position.x, position.y, 0);
	}
}

dcel::HalfEdge dcel::splitFace(dcel::HalfEdge & edgeA, dcel::HalfEdge & edgeB) {
	if (edgeA.getIncidentFace().getIndex() == k_OuterFaceIndex) {
		throw std::runtime_error("Cannot split outer face.");
	}

	if (edgeA.getIncidentFace() != edgeB.getIncidentFace()) {
		throw std::runtime_error("Cannot split half edges are on different faces.");
	}

	if (edgeA.getNext() == edgeB || edgeA.getPrev() == edgeB) {
		throw std::runtime_error("Cannot split adjacent vertices.");
	}

	auto face = edgeA.getIncidentFace();

	// Create two new edges and a new face;
	auto newEdge = createEdge();
	auto newEdgeTwin = createEdge();
	auto newFace = createFace();

	// Set new edges origins.
	newEdge.setOrigin(edgeA.getOrigin());
	newEdgeTwin.setOrigin(edgeB.getOrigin());

	// Update faces outer components.
	face.setOuterComponent(newEdge);
	newFace.setOuterComponent(newEdgeTwin);

	// Connect twins.
	newEdge.setTwin(newEdgeTwin);
	newEdgeTwin.setTwin(newEdge);

	// Connect new edges.
	newEdge.setPrev(edgeA.getPrev());
	newEdge.setNext(edgeB);
	newEdgeTwin.setPrev(edgeB.getPrev());
	newEdgeTwin.setNext(edgeA);

	// Update connections on pre-existing edges.
	edgeA.getPrev().setNext(newEdge);
	edgeB.getPrev().setNext(newEdgeTwin);
	edgeB.setPrev(newEdge);
	edgeA.setPrev(newEdgeTwin);

	// Set incident faces.
	// Recall that pre-existing edges on the face already were set to "face".
	newEdge.setIncidentFace(face);
	auto edge = newEdgeTwin;
	do {
		edge.setIncidentFace(newFace);
		edge = edge.getNext();
	} while (edge != newEdgeTwin);

	return newEdge;
}

dcel::HalfEdge dcel::splitFace(dcel::Vertex & vertexA, dcel::Vertex & vertexB) {
	HalfEdge halfEdgeA;
	HalfEdge halfEdgeB;
	if (tryFindSharedFace(vertexA, vertexB, halfEdgeA, halfEdgeB)) {
		splitFace(halfEdgeA, halfEdgeB);
	} else {
		throw std::runtime_error("Vertices do not share a face.");
	}
}
