#pragma once

#include <glm/glm.hpp>
#include <vector>

/// @brief Utilities to procedurally generate polygons.
///
/// Used for tests and demos.
class ofPolygonUtility {
public:
	/// @brief Generate a random polygon.
	/// @param points The vector to write generated points into.
	///
	/// The polygon has no self intersection by construction.
	/// Despite the use of random numbers the function is deterministic.
	static void createPolygonRandom(std::vector<glm::vec3> & points);

	/// @brief Generate a random monotone polygon.
	/// @param points The vector to write generated points into.
	///
	/// Despite the use of random numbers the function is deterministic.
	static void createPolygonRandomMonotone(std::vector<glm::vec3> & points);
};