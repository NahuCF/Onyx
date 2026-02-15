#pragma once

#include <Onyx.h>
#include <glm/glm.hpp>
#include <string>

namespace Editor3D {

using namespace Onyx;

class IconVisual {
public:
    IconVisual() = default;
    IconVisual(const std::string& texturePath, const glm::vec2& size = glm::vec2(1.0f))
        : m_TexturePath(texturePath), m_Size(size) {}

    void SetTexturePath(const std::string& path) { m_TexturePath = path; }
    void SetSize(const glm::vec2& size) { m_Size = size; }
    void SetSize(float uniformSize) { m_Size = glm::vec2(uniformSize); }
    void SetColor(const glm::vec4& color) { m_Color = color; }
    void SetTexture(Texture* texture) { m_Texture = texture; }

    const std::string& GetTexturePath() const { return m_TexturePath; }
    const glm::vec2& GetSize() const { return m_Size; }
    const glm::vec4& GetColor() const { return m_Color; }
    Texture* GetTexture() const { return m_Texture; }

private:
    std::string m_TexturePath;
    Texture* m_Texture = nullptr;
    glm::vec2 m_Size = glm::vec2(1.0f);
    glm::vec4 m_Color = glm::vec4(1.0f);
};

enum class WireframeShape {
    Box,
    Sphere,
    Capsule,
    Cylinder,
    Cone
};

class WireframeVisual {
public:
    WireframeVisual() = default;
    WireframeVisual(WireframeShape shape, const glm::vec3& extents = glm::vec3(1.0f))
        : m_Shape(shape), m_Extents(extents) {}

    void SetShape(WireframeShape shape) { m_Shape = shape; }
    void SetExtents(const glm::vec3& extents) { m_Extents = extents; }
    void SetColor(const glm::vec4& color) { m_Color = color; }
    void SetLineWidth(float width) { m_LineWidth = width; }

    WireframeShape GetShape() const { return m_Shape; }
    const glm::vec3& GetExtents() const { return m_Extents; }
    const glm::vec4& GetColor() const { return m_Color; }
    float GetLineWidth() const { return m_LineWidth; }

private:
    WireframeShape m_Shape = WireframeShape::Box;
    glm::vec3 m_Extents = glm::vec3(1.0f);
    glm::vec4 m_Color = glm::vec4(0.0f, 1.0f, 0.0f, 0.5f);
    float m_LineWidth = 1.0f;
};

class LightGizmoVisual {
public:
    enum class LightType { Point, Spot, Directional };

    void SetType(LightType type) { m_Type = type; }
    void SetRange(float range) { m_Range = range; }
    void SetSpotAngle(float angle) { m_SpotAngle = angle; }
    void SetColor(const glm::vec4& color) { m_Color = color; }
    void SetDirection(const glm::vec3& dir) { m_Direction = dir; }

    LightType GetType() const { return m_Type; }
    float GetRange() const { return m_Range; }
    float GetSpotAngle() const { return m_SpotAngle; }
    const glm::vec4& GetColor() const { return m_Color; }
    const glm::vec3& GetDirection() const { return m_Direction; }

private:
    LightType m_Type = LightType::Point;
    float m_Range = 10.0f;
    float m_SpotAngle = 45.0f;
    glm::vec4 m_Color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    glm::vec3 m_Direction = glm::vec3(0.0f, -1.0f, 0.0f);
};

class PathVisual {
public:
    void SetColor(const glm::vec4& color) { m_Color = color; }
    void SetLineWidth(float width) { m_LineWidth = width; }
    void SetShowArrows(bool show) { m_ShowArrows = show; }

    const glm::vec4& GetColor() const { return m_Color; }
    float GetLineWidth() const { return m_LineWidth; }
    bool GetShowArrows() const { return m_ShowArrows; }

private:
    glm::vec4 m_Color = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
    float m_LineWidth = 2.0f;
    bool m_ShowArrows = true;
};

} // namespace Editor3D
