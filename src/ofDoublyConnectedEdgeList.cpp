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

dcel::HalfEdge dcel::createEdge() {
	m_Edges.push_back(HalfEdgeData());
	return HalfEdge(this, m_Edges.size() - 1);
}

dcel::Face dcel::createFace() {
	m_Faces.push_back(FaceData());
	return Face(this, m_Faces.size() - 1);
}

int countHalfEdges(dcel::Face & face) {
	int count = 0;
	auto edge = face.getOuterComponent();
	do {
		edge = edge.getNext();
		++count;
	} while (edge != face.getOuterComponent());

	return count;
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
			throw std::runtime_error("extractTriangles: encountered non triangular face.");
		}
#endif
	}

	for (auto i = 0; i != m_Vertices.size(); ++i) {
		auto position = m_Vertices[i].position;
		vertices[i] = glm::vec3(position.x, position.y, 0);
	}
}

bool canSplitFace(
	dcel::HalfEdge & edgeA,
	dcel::HalfEdge & edgeB,
	int & halfEdgesOnFace, std::string & errorMessage) {
	if (edgeA == edgeB) {
		errorMessage = "Edges are equal.";
		halfEdgesOnFace = 0;
		return false;
	}

	// We always test for faces, and have additional test in debug mode.
	if (edgeA.getIncidentFace() != edgeB.getIncidentFace()) {
		errorMessage = "Edges are not on the same face.";
		halfEdgesOnFace = 0;
		return false;
	}

#if _DEBUG
	if (edgeA.getIncidentFace().getIndex() == dcel::getOuterFaceIndex()) {
		errorMessage = "Cannot split outer face.";
		halfEdgesOnFace = 0;
		return false;
	}

	if (edgeA.getOrigin() == edgeB.getDestination() || edgeA.getDestination() == edgeB.getOrigin()) {
		errorMessage = "Edges are already connected.";
		halfEdgesOnFace = 0;
		return false;
	}

	halfEdgesOnFace = countHalfEdges(edgeA.getIncidentFace());
	if (halfEdgesOnFace < 4) {
		errorMessage = "Can't split a face with less than 4 edges.";
		return false;
	}

	// That test should not be needed, redundant.
	auto e = edgeA;
	do {
		e = e.getNext();

		if (e == edgeB) {
			errorMessage = "";
			return true;
		}
	} while (e != edgeA);

	errorMessage = "Edges are not on the same cycle.";
	return false;
#else
	// halfEdgesOnFace is only used for checks in _DEBUG mode.
	halfEdgesOnFace = 0;
	errorMessage = "";
	return true;
#endif
}

dcel::HalfEdge dcel::splitFaceInternal(
	dcel::HalfEdge & edgeA, dcel::HalfEdge & edgeB, dcel::EdgeAssign edgeAssign) {
	auto face = edgeA.getIncidentFace();

	// Create two new edges and a new face;
	auto newEdge = createEdge();
	newEdge.setOrigin(edgeA.getOrigin());
	newEdge.setIncidentFace(face);
	face.setOuterComponent(newEdge);
	auto newEdgeTwin = createEdge();
	newEdgeTwin.setOrigin(edgeB.getOrigin());
	auto newFace = createFace();
	newFace.setOuterComponent(newEdgeTwin);

	// Connect twins.
	newEdge.setTwin(newEdgeTwin);
	newEdgeTwin.setTwin(newEdge);

	// Connect edges.
	newEdge.setPrev(edgeA.getPrev());
	newEdge.setNext(edgeB);
	newEdgeTwin.setPrev(edgeB.getPrev());
	newEdgeTwin.setNext(edgeA);

	edgeA.getPrev().setNext(newEdge);
	edgeB.getPrev().setNext(newEdgeTwin);
	edgeB.setPrev(newEdge);
	edgeA.setPrev(newEdgeTwin);

	switch (edgeAssign) {
	case EdgeAssign::None:
		break;
	case EdgeAssign::Origin:
		newEdge.getOrigin().setIncidentEdge(newEdge);
		break;
	case EdgeAssign::Destination:
		newEdgeTwin.getOrigin().setIncidentEdge(newEdgeTwin);
		break;
	}

	auto edge = newEdgeTwin;
	do {
		edge.setIncidentFace(newFace);
		edge = edge.getNext();
	} while (edge != newEdgeTwin);

	return newEdge;
}

dcel::HalfEdge dcel::splitFace(
	dcel::HalfEdge & edgeA, dcel::HalfEdge & edgeB, dcel::EdgeAssign edgeAssign) {
#if _DEBUG
	auto halfEdgesOnFace = 0;
	std::string errorMessage;
	if (!canSplitFace(edgeA, edgeB, halfEdgesOnFace, errorMessage)) {
		throw std::runtime_error(errorMessage);
	}
#endif
	auto newEdge = splitFaceInternal(edgeA, edgeB, edgeAssign);
#if _DEBUG

	// Verify that the number of half-edges before and after split are consistent.
	auto nEdges1 = countHalfEdges(newEdge.getIncidentFace());
	auto nEdges2 = countHalfEdges(newEdge.getTwin().getIncidentFace());
	auto edgesCountsMatch = halfEdgesOnFace + 2 == nEdges1 + nEdges2;
	if (!edgesCountsMatch) {
		throw std::runtime_error(
			"Edge counts don't match, from " + std::to_string(halfEdgesOnFace) + " to " + std::to_string(nEdges1) + " and " + std::to_string(nEdges2));
	}
#endif
	return newEdge;
}

dcel::HalfEdge dcel::splitFace(
	dcel::HalfEdge & edge, dcel::Vertex & vertex, dcel::EdgeAssign edgeAssign) {
	std::string error;
	int _ = 0;

	// Iterate over edges connected to the vertex.
	auto vertexEdge = vertex.getIncidentEdge();
	do {
		if (canSplitFace(edge, vertexEdge, _, error)) {
			return splitFaceInternal(edge, vertexEdge, edgeAssign);
		}
		vertexEdge = vertexEdge.getPrev().getTwin();
	} while (vertexEdge != vertex.getIncidentEdge());

	throw std::runtime_error(error);
}

ofPolygonWindingOrder dcel::getOrder(const dcel::Face & face) {
	auto halfEdgeIterator = dcel::HalfEdgesIterator(face);
	auto sum = 0.0f;

	do {
		auto edge = halfEdgeIterator.getCurrent();
		auto p1 = edge.getOrigin().getPosition();
		auto p2 = edge.getDestination().getPosition();
		sum += (p2.x - p1.x) * (p2.y + p1.y);
	} while (halfEdgeIterator.moveNext());

	// Handled out of due diligence but unlikely.
	if (sum == 0.0f) {
		return ofPolygonWindingOrder::None;
	}

	return sum > 0 ? ofPolygonWindingOrder::ClockWise : ofPolygonWindingOrder::CounterClockWise;
}

template <class vecN>
ofPolygonWindingOrder getVerticesOrder(const std::vector<vecN> & vertices) {
	auto sum = 0.0f;
	auto len = vertices.size();

	for (auto i = 0; i != len; ++i) {
		auto p1 = vertices[i];
		auto p2 = vertices[(i + 1) % len];
		sum += (p2.x - p1.x) * (p2.y + p1.y);
	}

	// Handled out of due diligence but unlikely.
	if (sum == 0.0f) {
		return ofPolygonWindingOrder::None;
	}

	return sum > 0 ? ofPolygonWindingOrder::ClockWise : ofPolygonWindingOrder::CounterClockWise;
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
	if (getVerticesOrder(vertices) != ofPolygonWindingOrder::CounterClockWise) {
		throw std::runtime_error("Passed vertices should be in counter clockwise order.");
	}
#endif

	const auto len = vertices.size();

	m_Vertices.resize(len);
	m_Edges.resize(len * 2); // *2, outer and inner faces, len edges per face.
	m_Faces.resize(2); // 2, outer and inner faces.

	// Outer face.
	m_Faces[0].outerComponent = len;

	// Inner face.
	m_Faces[1].outerComponent = 0;

	for (auto i = 0; i != len; ++i) {
		VertexData vert;
		vert.position = vertices[i];
		vert.chain = dcel::Chain::None;
		vert.incidentEdge = i;
		m_Vertices[i] = vert;

		HalfEdgeData edge;
		edge.origin = i;
		edge.incidentFace = 1;
		edge.twin = i + len;
		edge.prev = (i - 1 + len) % len;
		edge.next = (i + 1) % len;
		m_Edges[i] = edge;

		edge.origin = (i + 1) % len; // Twin(i) = i + len
		edge.incidentFace = 0; // Outer face, by convention.
		edge.twin = i;
		edge.prev = (i - 1 + len) % len + len;
		edge.next = (i + 1) % len + len;
		m_Edges[i + len] = edge;
	}
}

void dcel::initializeFromCCWVertices(const std::vector<glm::vec2> & vertices) {
	initializeFromCCWVertices<glm::vec2>(vertices);
}

void dcel::initializeFromCCWVertices(const std::vector<glm::vec3> & vertices) {
	initializeFromCCWVertices<glm::vec3>(vertices);
}