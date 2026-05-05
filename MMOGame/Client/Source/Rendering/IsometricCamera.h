#pragma once

#include <glm/glm.hpp>

namespace MMO {

	class IsometricCamera
	{
	public:
		IsometricCamera();

		void SetTarget(const glm::vec3& target);
		void Update(float dt);

		void RotateYaw(float deltaDegrees);
		void Zoom(float deltaDistance);

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix(float aspect) const;
		glm::vec3 GetPosition() const;
		glm::vec3 GetTarget() const { return m_CurrentTarget; }

		// Unproject screen point to world ray
		// Returns ray origin and direction
		void ScreenToWorldRay(float screenX, float screenY,
							  float viewportWidth, float viewportHeight,
							  glm::vec3& rayOrigin, glm::vec3& rayDir) const;

		// Intersect ray with horizontal plane at given Y
		bool RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
								  float planeY, glm::vec3& hitPoint) const;

		float GetYaw() const { return m_Yaw; }
		float GetDistance() const { return m_Distance; }
		float GetPitch() const { return m_Pitch; }
		float GetFovY() const { return m_FovY; }
		float GetNear() const { return m_Near; }
		float GetFar() const { return m_Far; }

	private:
		glm::vec3 m_DesiredTarget = {0, 0, 0};
		glm::vec3 m_CurrentTarget = {0, 0, 0};

		float m_Yaw = 45.0f;	  // Degrees, 0 = looking along +X
		float m_Pitch = 40.0f;	  // Degrees from horizontal (Diablo-style angle)
		float m_Distance = 60.0f; // Distance from target

		float m_MinDistance = 20.0f;
		float m_MaxDistance = 200.0f;

		float m_FollowSpeed = 8.0f;

		float m_FovY = 45.0f;
		float m_Near = 0.1f;
		float m_Far = 1000.0f;

		mutable float m_LastAspect = 16.0f / 9.0f;
	};

} // namespace MMO
