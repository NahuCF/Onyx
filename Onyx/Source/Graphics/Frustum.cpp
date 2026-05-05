#include "Frustum.h"
#include <cmath>

namespace Onyx {

	void Frustum::Update(const glm::mat4& vp)
	{
		// Extract frustum planes from view-projection matrix
		// Using Gribb/Hartmann method

		// Left plane
		m_Planes[Left] = glm::vec4(
			vp[0][3] + vp[0][0],
			vp[1][3] + vp[1][0],
			vp[2][3] + vp[2][0],
			vp[3][3] + vp[3][0]);

		// Right plane
		m_Planes[Right] = glm::vec4(
			vp[0][3] - vp[0][0],
			vp[1][3] - vp[1][0],
			vp[2][3] - vp[2][0],
			vp[3][3] - vp[3][0]);

		// Bottom plane
		m_Planes[Bottom] = glm::vec4(
			vp[0][3] + vp[0][1],
			vp[1][3] + vp[1][1],
			vp[2][3] + vp[2][1],
			vp[3][3] + vp[3][1]);

		// Top plane
		m_Planes[Top] = glm::vec4(
			vp[0][3] - vp[0][1],
			vp[1][3] - vp[1][1],
			vp[2][3] - vp[2][1],
			vp[3][3] - vp[3][1]);

		// Near plane
		m_Planes[Near] = glm::vec4(
			vp[0][3] + vp[0][2],
			vp[1][3] + vp[1][2],
			vp[2][3] + vp[2][2],
			vp[3][3] + vp[3][2]);

		// Far plane
		m_Planes[Far] = glm::vec4(
			vp[0][3] - vp[0][2],
			vp[1][3] - vp[1][2],
			vp[2][3] - vp[2][2],
			vp[3][3] - vp[3][2]);

		// Normalize all planes
		for (int i = 0; i < Count; i++)
		{
			NormalizePlane(m_Planes[i]);
		}
	}

	void Frustum::NormalizePlane(glm::vec4& plane)
	{
		float length = std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
		if (length > 0.0f)
		{
			plane /= length;
		}
	}

	float Frustum::DistanceToPlane(const glm::vec4& plane, const glm::vec3& point) const
	{
		return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
	}

	bool Frustum::IsPointVisible(const glm::vec3& point) const
	{
		for (int i = 0; i < Count; i++)
		{
			if (DistanceToPlane(m_Planes[i], point) < 0.0f)
			{
				return false;
			}
		}
		return true;
	}

	bool Frustum::IsSphereVisible(const glm::vec3& center, float radius) const
	{
		for (int i = 0; i < Count; i++)
		{
			if (DistanceToPlane(m_Planes[i], center) < -radius)
			{
				return false;
			}
		}
		return true;
	}

	bool Frustum::IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const
	{
		for (int i = 0; i < Count; i++)
		{
			// Find the corner of the box that is most in the direction of the plane normal
			glm::vec3 positive;
			positive.x = (m_Planes[i].x >= 0.0f) ? max.x : min.x;
			positive.y = (m_Planes[i].y >= 0.0f) ? max.y : min.y;
			positive.z = (m_Planes[i].z >= 0.0f) ? max.z : min.z;

			// If the most positive corner is behind the plane, the box is outside
			if (DistanceToPlane(m_Planes[i], positive) < 0.0f)
			{
				return false;
			}
		}
		return true;
	}

} // namespace Onyx
