#pragma once

#include <glm/glm.hpp>
#include <array>

namespace Onyx {

// Represents a view frustum for culling
class Frustum {
public:
    void Update(const glm::mat4& viewProjection);

    bool IsBoxVisible(const glm::vec3& minPoint, const glm::vec3& maxPoint) const;
    bool IsSphereVisible(const glm::vec3& center, float radius) const;
    bool IsPointVisible(const glm::vec3& point) const;

private:
    enum Planes {
        PLANE_LEFT = 0,
        PLANE_RIGHT,
        PLANE_BOTTOM,
        PLANE_TOP,
        PLANE_NEAR,
        PLANE_FAR,
        PLANE_COUNT
    };

    // Plane equation: ax + by + cz + d = 0
    // Stored as vec4(a, b, c, d) where (a,b,c) is the normal
    std::array<glm::vec4, PLANE_COUNT> m_Planes;

    // Normalize a plane equation
    static glm::vec4 NormalizePlane(const glm::vec4& plane);
};

} // namespace Onyx
