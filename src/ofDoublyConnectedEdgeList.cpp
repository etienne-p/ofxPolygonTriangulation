#include "ofDoublyConnectedEdgeList.h"
#include <cassert>
#include <glm/gtx/vector_angle.hpp>
#include <stdexcept>

// Lighten below code.
using dcel = ofDoublyConnectedEdgeList;

dcel::HalfEdge dcel::Vertex::getIncidentEdge() const {
	return dcel::HalfEdge(m_Dcel, m_Dcel->m_VerticesIncidentEdge[m_Index]);
}

void dcel::Vertex::setIncidentEdge(const dcel::HalfEdge & halfEdge) {
	m_Dcel->m_VerticesIncidentEdge[m_Index] = halfEdge.getIndex();
}

dcel::Vertex dcel::HalfEdge::getOrigin() const {
	return dcel::Vertex(m_Dcel, m_Dcel->m_HalfEdgesOrigin[m_Index]);
}

void dcel::HalfEdge::setOrigin(const dcel::Vertex & vertex) {
	m_Dcel->m_HalfEdgesOrigin[m_Index] = vertex.getIndex();
}

dcel::Face dcel::HalfEdge::getIncidentFace() const {
	return dcel::Face(m_Dcel, m_Dcel->m_HalfEdgesIncidentFace[m_Index]);
}

void dcel::HalfEdge::setIncidentFace(const dcel::Face & face) {
	m_Dcel->m_HalfEdgesIncidentFace[m_Index] = face.getIndex();
}

glm::vec2 dcel::HalfEdge::getDirection() const {
	return getDestination().getPosition() - getOrigin().getPosition();
}

dcel::HalfEdge dcel::Face::getOuterComponent() const {
	return dcel::HalfEdge(m_Dcel, m_Dcel->m_FacesOuterComponent[m_Index]);
}

void dcel::Face::setOuterComponent(const dcel::HalfEdge & halfEdge) {
	m_Dcel->m_FacesOuterComponent[m_Index] = halfEdge.getIndex();
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

// Calculates the oriented angle,
// returning the result in the [0, 2PI] range,
// instead of the [-PI, PI] range.
float angle(glm::vec2 a, glm::vec2 b) {
	auto angle = glm::orientedAngle(a, b);
	if (angle < 0) {
		return angle + 2.0f * glm::pi<float>();
	}
	return angle;
}

float dcel::findMaxInnerAngle(ofDoublyConnectedEdgeList & dcel, const Face & face, HalfEdge & halfEdge) {
	// Find maximal inner angle.
	auto it = ofDoublyConnectedEdgeList::HalfEdgesIterator(face);
	auto prevDir = glm::vec2();
	auto currentDir = glm::normalize(it.getCurrent().getPrev().getDirection());
	auto maxInnerAngle = 0.0f;

	do {
		prevDir = currentDir;
		currentDir = glm::normalize(it.getCurrent().getDirection());
		auto innerAngle = angle(currentDir, -prevDir);
		if (maxInnerAngle < innerAngle) {
			maxInnerAngle = innerAngle;
			halfEdge = it.getCurrent();
		}

	} while (it.moveNext());

	return maxInnerAngle;
}

bool dcel::tryFindSharedFace(
	const dcel::Vertex & vertexA, const dcel::Vertex & vertexB,
	dcel::HalfEdge & halfEdgeA, dcel::HalfEdge & halfEdgeB) const {
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
	m_HalfEdgesOrigin.push_back({});
	m_HalfEdgesIncidentFace.push_back({});
	m_HalfEdgesTwin.push_back({});
	m_HalfEdgesPrev.push_back({});
	m_HalfEdgesNext.push_back({});
	return HalfEdge(this, m_HalfEdgesOrigin.size() - 1);
}

dcel::Face dcel::createFace() {
	m_FacesOuterComponent.push_back({});
	return Face(this, m_FacesOuterComponent.size() - 1);
}

// Leans on a convention, see k_InnerFaceIndex.
dcel::Face dcel::getInnerFace() {
	if (m_FacesOuterComponent.size() < k_InnerFaceIndex + 1) {
		throw std::runtime_error("Cannot access inner face.");
	}
	return dcel::Face(this, k_InnerFaceIndex);
}

template <class vecN>
void dcel::initializeFromCCWVertices(const std::vector<vecN> & vertices) {
	if (getWindingOrder(vertices) != ofPolygonWindingOrder::CounterClockWise) {
		throw std::runtime_error("Passed vertices should be in counter clockwise order.");
	}

	const auto len = vertices.size();

	m_VerticesPosition.resize(len);
	m_VerticesChain.resize(len);
	m_VerticesIncidentEdge.resize(len);

	// *2, outer and inner faces, len edges per face.
	m_HalfEdgesOrigin.resize(len * 2);
	m_HalfEdgesIncidentFace.resize(len * 2);
	m_HalfEdgesTwin.resize(len * 2);
	m_HalfEdgesPrev.resize(len * 2);
	m_HalfEdgesNext.resize(len * 2);

	// 2, outer and inner faces.
	m_FacesOuterComponent.resize(2);

	// Outer face.
	m_FacesOuterComponent[k_OuterFaceIndex] = len;

	// Inner face.
	m_FacesOuterComponent[k_InnerFaceIndex] = 0;

	for (auto i = 0; i != len; ++i) {
		m_VerticesPosition[i] = vertices[i];
		m_VerticesChain[i] = dcel::Chain::None;
		m_VerticesIncidentEdge[i] = i;

		auto prevIndex = (i - 1 + len) % len;
		auto nextIndex = (i + 1) % len;
		auto twinIndex = i + len;

		m_HalfEdgesOrigin[i] = i;
		m_HalfEdgesIncidentFace[i] = k_InnerFaceIndex;
		m_HalfEdgesTwin[i] = twinIndex;
		m_HalfEdgesPrev[i] = prevIndex;
		m_HalfEdgesNext[i] = nextIndex;

		m_HalfEdgesOrigin[twinIndex] = nextIndex;
		m_HalfEdgesIncidentFace[twinIndex] = k_OuterFaceIndex;
		m_HalfEdgesTwin[twinIndex] = i;
		m_HalfEdgesPrev[twinIndex] = nextIndex + len;
		m_HalfEdgesNext[twinIndex] = prevIndex + len;
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
	vertices.resize(m_VerticesPosition.size());
	indices.resize((m_FacesOuterComponent.size() - 1) * 3); // -1 to exclude outer face.

	// Iteration starts at 1 to exclude outer face.
	// If the face isn't a triangle, we'll simply ignore vertices beyond the third.
	// It will lead to an incorrect geometry but will not raise errors.
	for (auto i = 1; i != m_FacesOuterComponent.size(); ++i) {
		auto edgeIndex = m_FacesOuterComponent[i];
		auto index = (i - 1) * 3;
		indices[index] = m_HalfEdgesOrigin[edgeIndex];
		edgeIndex = m_HalfEdgesNext[edgeIndex];
		indices[index + 1] = m_HalfEdgesOrigin[edgeIndex];
		edgeIndex = m_HalfEdgesNext[edgeIndex];
		indices[index + 2] = m_HalfEdgesOrigin[edgeIndex];
	}

	for (auto i = 0; i != m_VerticesPosition.size(); ++i) {
		auto position = m_VerticesPosition[i];
		vertices[i] = glm::vec3(position.x, position.y, 0);
	}
}

dcel::HalfEdge dcel::addHalfEdge(dcel::HalfEdge & edgeA, dcel::HalfEdge & edgeB) {
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

dcel::HalfEdge dcel::addHalfEdge(dcel::Vertex & vertexA, dcel::Vertex & vertexB) {
	HalfEdge halfEdgeA;
	HalfEdge halfEdgeB;
	if (tryFindSharedFace(vertexA, vertexB, halfEdgeA, halfEdgeB)) {
		addHalfEdge(halfEdgeA, halfEdgeB);
	} else {
		throw std::runtime_error("Vertices do not share a face.");
	}
}
