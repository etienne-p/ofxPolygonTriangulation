#pragma once

#include <glm/glm.hpp>
#include <vector>

class ofPolygonUtility {
public:
	// No self-intersection by construction.
	static void createPolygonRandom(std::vector<glm::vec3> & points, size_t numPoints);
	static void createPolygonRandomMonotone(std::vector<glm::vec3> & points, size_t numPoints);
};