#include "IsometricCamera.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace MMO {

	IsometricCamera::IsometricCamera() = default;

	void IsometricCamera::SetTarget(const glm::vec3& target)
	{
		m_DesiredTarget = target;
	}

	void IsometricCamera::Update(float dt)
	{
		float t = 1.0f - std::exp(-m_FollowSpeed * dt);
		m_CurrentTarget = glm::mix(m_CurrentTarget, m_DesiredTarget, t);
	}

	void IsometricCamera::RotateYaw(float deltaDegrees)
	{
		m_Yaw += deltaDegrees;
		if (m_Yaw >= 360.0f)
			m_Yaw -= 360.0f;
		if (m_Yaw < 0.0f)
			m_Yaw += 360.0f;
	}

	void IsometricCamera::Zoom(float deltaDistance)
	{
		m_Distance = std::clamp(m_Distance - deltaDistance, m_MinDistance, m_MaxDistance);
	}

	glm::vec3 IsometricCamera::GetPosition() const
	{
		float yawRad = glm::radians(m_Yaw);
		float pitchRad = glm::radians(m_Pitch);

		float cosP = std::cos(pitchRad);
		float sinP = std::sin(pitchRad);

		glm::vec3 offset(
			std::cos(yawRad) * cosP * m_Distance,
			sinP * m_Distance,
			std::sin(yawRad) * cosP * m_Distance);

		return m_CurrentTarget + offset;
	}

	glm::mat4 IsometricCamera::GetViewMatrix() const
	{
		return glm::lookAt(GetPosition(), m_CurrentTarget, glm::vec3(0, 1, 0));
	}

	glm::mat4 IsometricCamera::GetProjectionMatrix(float aspect) const
	{
		m_LastAspect = aspect;
		return glm::perspective(glm::radians(m_FovY), aspect, m_Near, m_Far);
	}

	void IsometricCamera::ScreenToWorldRay(float screenX, float screenY,
										   float viewportWidth, float viewportHeight,
										   glm::vec3& rayOrigin, glm::vec3& rayDir) const
	{
		// NDC coordinates [-1, 1]
		float ndcX = (2.0f * screenX / viewportWidth) - 1.0f;
		float ndcY = 1.0f - (2.0f * screenY / viewportHeight);

		glm::mat4 proj = GetProjectionMatrix(m_LastAspect);
		glm::mat4 view = GetViewMatrix();
		glm::mat4 invVP = glm::inverse(proj * view);

		glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
		glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

		nearPoint /= nearPoint.w;
		farPoint /= farPoint.w;

		rayOrigin = glm::vec3(nearPoint);
		rayDir = glm::normalize(glm::vec3(farPoint - nearPoint));
	}

	bool IsometricCamera::RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
											   float planeY, glm::vec3& hitPoint) const
	{
		if (std::abs(rayDir.y) < 0.0001f)
			return false;

		float t = (planeY - rayOrigin.y) / rayDir.y;
		if (t < 0.0f)
			return false;

		hitPoint = rayOrigin + rayDir * t;
		return true;
	}

} // namespace MMO
