#pragma once

#include <Graphics/Shader.h>
#include <Graphics/Buffers.h>
#include <glm/glm.hpp>
#include <memory>

namespace MMO {

enum class GizmoMode {
    NONE = 0,
    TRANSLATE,
    ROTATE,
    SCALE
};

enum class GizmoAxis {
    NONE = 0,
    X,
    Y,
    Z,
    XY,
    XZ,
    YZ,
    XYZ  // Uniform scale
};

class TransformGizmo {
public:
    TransformGizmo();
    ~TransformGizmo() = default;

    void Init();
    void Render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position, float cameraDistance);

    // Interaction
    GizmoAxis TestHit(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                      const glm::vec3& gizmoPos, float cameraDistance);

    // Calculate delta based on mouse movement
    glm::vec3 CalculateTranslation(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                    const glm::vec3& gizmoPos, GizmoAxis axis,
                                    const glm::mat4& view, const glm::mat4& projection);

    // Calculate rotation angle delta
    float CalculateRotation(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                            const glm::vec3& gizmoPos, GizmoAxis axis,
                            const glm::mat4& view);

    // Calculate scale delta
    glm::vec3 CalculateScale(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                             const glm::vec3& gizmoPos, GizmoAxis axis,
                             const glm::mat4& view);

    // State
    void SetMode(GizmoMode mode) { m_Mode = mode; }
    GizmoMode GetMode() const { return m_Mode; }

    void SetActiveAxis(GizmoAxis axis) { m_ActiveAxis = axis; }
    GizmoAxis GetActiveAxis() const { return m_ActiveAxis; }

    bool IsActive() const { return m_ActiveAxis != GizmoAxis::NONE; }

    // Drag state
    void BeginDrag(const glm::vec3& objectPos, const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                   const glm::mat4& view);
    void EndDrag();
    bool IsDragging() const { return m_IsDragging; }
    glm::vec3 GetDragStartPos() const { return m_DragStartPos; }

    // Settings
    void SetSnapEnabled(bool enabled) { m_SnapEnabled = enabled; }
    bool IsSnapEnabled() const { return m_SnapEnabled; }
    void SetSnapValue(float value) { m_SnapValue = value; }
    float GetSnapValue() const { return m_SnapValue; }

private:
    void CreateArrowMesh();
    void CreatePlaneMesh();
    void CreateCubeMesh();
    void CreateCircleMesh();

    void RenderTranslateGizmo(const glm::mat4& view, const glm::mat4& projection,
                              const glm::vec3& position, float scale);
    void RenderRotateGizmo(const glm::mat4& view, const glm::mat4& projection,
                           const glm::vec3& position, float scale);
    void RenderScaleGizmo(const glm::mat4& view, const glm::mat4& projection,
                          const glm::vec3& position, float scale);

    bool RayIntersectsLine(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                           const glm::vec3& lineStart, const glm::vec3& lineEnd,
                           float threshold, float& distance);

    GizmoMode m_Mode = GizmoMode::TRANSLATE;
    GizmoAxis m_ActiveAxis = GizmoAxis::NONE;

    // Shaders
    std::unique_ptr<Onyx::Shader> m_GizmoShader;

    // Arrow geometry (for translate/scale)
    std::unique_ptr<Onyx::VertexArray> m_ArrowVAO;
    std::unique_ptr<Onyx::VertexBuffer> m_ArrowVBO;
    uint32_t m_ArrowVertexCount = 0;

    // Plane geometry (for 2-axis translate)
    std::unique_ptr<Onyx::VertexArray> m_PlaneVAO;
    std::unique_ptr<Onyx::VertexBuffer> m_PlaneVBO;

    // Cube geometry (for scale handles)
    std::unique_ptr<Onyx::VertexArray> m_CubeVAO;
    std::unique_ptr<Onyx::VertexBuffer> m_CubeVBO;
    std::unique_ptr<Onyx::IndexBuffer> m_CubeEBO;

    // Circle geometry (for rotation)
    std::unique_ptr<Onyx::VertexArray> m_CircleVAO;
    std::unique_ptr<Onyx::VertexBuffer> m_CircleVBO;
    uint32_t m_CircleVertexCount = 0;

    // Interaction state
    bool m_IsDragging = false;
    glm::vec3 m_DragStartPos = glm::vec3(0.0f);        // Object position at drag start
    glm::vec3 m_DragStartHitPoint = glm::vec3(0.0f);   // Where the initial click hit the constraint plane
    glm::vec3 m_DragPlaneNormal = glm::vec3(0.0f);
    glm::vec3 m_DragAxisDir = glm::vec3(0.0f);
    float m_DragStartAngle = 0.0f;                     // For rotation: initial angle
    float m_DragStartScale = 1.0f;                     // For scale: initial scale factor

    // Settings
    bool m_SnapEnabled = false;
    float m_SnapValue = 1.0f;

    // Colors
    const glm::vec3 m_XColor = glm::vec3(0.9f, 0.2f, 0.2f);
    const glm::vec3 m_YColor = glm::vec3(0.2f, 0.9f, 0.2f);
    const glm::vec3 m_ZColor = glm::vec3(0.2f, 0.4f, 0.9f);
    const glm::vec3 m_HighlightColor = glm::vec3(1.0f, 1.0f, 0.0f);
};

} // namespace MMO
