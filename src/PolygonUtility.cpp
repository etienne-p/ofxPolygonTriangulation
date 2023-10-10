#include "PolygonUtility.h"
#include "ofMain.h"

void PolygonUtility::createPolygonRandom(std::vector<glm::vec3> & points, size_t numPoints) {
	constexpr auto angleNoiseY = 0.4f;
	constexpr auto radiusNoiseY = 4.0f;
	auto noiseX = 0.f;
	auto dAngle = glm::pi<float>() * 2.0f / (float)numPoints;

	for (auto i = 0; i != numPoints; ++i) {
		auto angle = glm::mix(dAngle * i, dAngle * (i + 1), ofNoise(noiseX, angleNoiseY));
		auto radius = glm::mix(0.2f, 1.0f, ofNoise(noiseX, radiusNoiseY));
		points[i] = glm::vec3(radius * glm::cos(angle), radius * glm::sin(angle), 0);
		noiseX += 0.4f;
	}
}

void PolygonUtility::createPolygonRandomMonotone(std::vector<glm::vec3> & points, size_t numPoints) {
	constexpr auto xNoiseY = 0.4f;
	constexpr auto noiseDx = 0.4f;
	constexpr auto yNoiseY = 0.8f;
	constexpr auto epsilon = 1e-6f;

	const auto midPoint = numPoints / 2;

	auto noiseX = 0.f;
	auto dY = 2.0f / (float)(midPoint - 1);
	auto yAcc = -1.0f;

	points[0] = glm::vec3(0, -1, 0);

	// Left chain.
	for (auto i = 1; i != midPoint; ++i) {
		auto x = glm::mix(-1.0f, -0.1f, ofNoise(noiseX, xNoiseY));
		auto y = glm::mix(yAcc + epsilon, yAcc + dY - epsilon, ofNoise(noiseX, yNoiseY));
		points[i] = glm::vec3(x, y, 0);
		noiseX += noiseDx;
		yAcc += dY;
	}

	points[midPoint] = glm::vec3(0, 1, 0);

	dY = 2.0f / (float)(numPoints - midPoint - 1);
	yAcc = 1.0f;

	// Right chain.
	for (auto i = midPoint + 1; i != numPoints; ++i) {
		auto x = glm::mix(0.1f, 1.0f, ofNoise(noiseX, xNoiseY));
		auto y = glm::mix(yAcc - dY + epsilon, yAcc - epsilon, ofNoise(noiseX, yNoiseY));
		points[i] = glm::vec3(x, y, 0);
		noiseX += noiseDx;
		yAcc -= dY;
	}
}