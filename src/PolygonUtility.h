#pragma once

#include <vector>
#include <glm/glm.hpp>

class PolygonUtility
{
public:
	// No self-intersection by construction.
	static void createPolygonRandom(std::vector<glm::vec3>& points, size_t numPoints);
	static void createPolygonRandomMonotone(std::vector<glm::vec3>& points, size_t numPoints);
};