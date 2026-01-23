#include "pch.h"
#include "Frustum.h"

namespace Onyx {

void Frustum::Update(const glm::mat4& vp) {
    // Extract frustum planes from view-projection matrix
    // Using Gribb/Hartmann method

    // Left plane
    m_Planes[PLANE_LEFT] = NormalizePlane(glm::vec4(
        vp[0][3] + vp[0][0],
        vp[1][3] + vp[1][0],
        vp[2][3] + vp[2][0],
        vp[3][3] + vp[3][0]
    ));

    // Right plane
    m_Planes[PLANE_RIGHT] = NormalizePlane(glm::vec4(
        vp[0][3] - vp[0][0],
        vp[1][3] - vp[1][0],
        vp[2][3] - vp[2][0],
        vp[3][3] - vp[3][0]
    ));

    // Bottom plane
    m_Planes[PLANE_BOTTOM] = NormalizePlane(glm::vec4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]
    ));

    // Top plane
    m_Planes[PLANE_TOP] = NormalizePlane(glm::vec4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]
    ));

    // Near plane
    m_Planes[PLANE_NEAR] = NormalizePlane(glm::vec4(
        vp[0][3] + vp[0][2],
        vp[1][3] + vp[1][2],
        vp[2][3] + vp[2][2],
        vp[3][3] + vp[3][2]
    ));

    // Far plane
    m_Planes[PLANE_FAR] = NormalizePlane(glm::vec4(
        vp[0][3] - vp[0][2],
        vp[1][3] - vp[1][2],
        vp[2][3] - vp[2][2],
        vp[3][3] - vp[3][2]
    ));
}

glm::vec4 Frustum::NormalizePlane(const glm::vec4& plane) {
    float length = glm::length(glm::vec3(plane));
    if (length > 0.0001f) {
        return plane / length;
    }
    return plane;
}

bool Frustum::IsBoxVisible(const glm::vec3& minPoint, const glm::vec3& maxPoint) const {
    // Test AABB against all frustum planes
    // Using the "positive vertex" method for efficiency

    for (int i = 0; i < PLANE_COUNT; i++) {
        const glm::vec4& plane = m_Planes[i];
        glm::vec3 normal(plane.x, plane.y, plane.z);

        // Find the positive vertex (furthest in direction of plane normal)
        glm::vec3 positiveVertex;
        positiveVertex.x = (normal.x >= 0.0f) ? maxPoint.x : minPoint.x;
        positiveVertex.y = (normal.y >= 0.0f) ? maxPoint.y : minPoint.y;
        positiveVertex.z = (normal.z >= 0.0f) ? maxPoint.z : minPoint.z;

        // If positive vertex is behind the plane, box is completely outside
        float distance = glm::dot(normal, positiveVertex) + plane.w;
        if (distance < 0.0f) {
            return false; // Outside this plane, so outside frustum
        }
    }

    return true; // Inside or intersecting all planes
}

bool Frustum::IsSphereVisible(const glm::vec3& center, float radius) const {
    for (int i = 0; i < PLANE_COUNT; i++) {
        const glm::vec4& plane = m_Planes[i];
        glm::vec3 normal(plane.x, plane.y, plane.z);

        float distance = glm::dot(normal, center) + plane.w;
        if (distance < -radius) {
            return false; // Sphere is completely behind this plane
        }
    }

    return true;
}

bool Frustum::IsPointVisible(const glm::vec3& point) const {
    for (int i = 0; i < PLANE_COUNT; i++) {
        const glm::vec4& plane = m_Planes[i];
        glm::vec3 normal(plane.x, plane.y, plane.z);

        float distance = glm::dot(normal, point) + plane.w;
        if (distance < 0.0f) {
            return false; // Point is behind this plane
        }
    }

    return true;
}

} // namespace Onyx
