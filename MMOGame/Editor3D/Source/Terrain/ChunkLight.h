#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Editor3D {

	enum class LightType : uint8_t
	{
		Point = 0,
		Spot = 1
	};

	struct ChunkLight
	{
		LightType type = LightType::Point;
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
		glm::vec3 color = glm::vec3(1.0f);
		float intensity = 1.0f;
		float range = 10.0f;
		float innerAngle = 30.0f;
		float outerAngle = 45.0f;
		bool castShadows = false;
		uint32_t uniqueId = 0;
	};

	constexpr int MAX_POINT_LIGHTS = 32;
	constexpr int MAX_SPOT_LIGHTS = 8;

} // namespace Editor3D
