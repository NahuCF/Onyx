#pragma once

#include <glm/glm.hpp>

namespace Onyx {

	class Frustum
	{
	public:
		void Update(const glm::mat4& viewProjection);
		bool IsPointVisible(const glm::vec3& point) const;
		bool IsSphereVisible(const glm::vec3& center, float radius) const;
		bool IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const;

	private:
		enum Planes
		{
			Left = 0,
			Right,
			Bottom,
			Top,
			Near,
			Far,
			Count
		};

		glm::vec4 m_Planes[Count];

		void NormalizePlane(glm::vec4& plane);
		float DistanceToPlane(const glm::vec4& plane, const glm::vec3& point) const;
	};

} // namespace Onyx
