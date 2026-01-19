#include "TransformGizmo.h"
#include <Graphics/VertexLayout.h>
#include <Graphics/RenderCommand.h>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace MMO {

TransformGizmo::TransformGizmo() = default;

void TransformGizmo::Init() {
    m_GizmoShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/gizmo.vert",
        "MMOGame/Editor3D/assets/shaders/gizmo.frag"
    );

    CreateArrowMesh();
    CreatePlaneMesh();
    CreateCubeMesh();
    CreateCircleMesh();
}

void TransformGizmo::CreateArrowMesh() {
    // Arrow shaft + cone for each axis
    // We'll draw each axis separately with different transforms

    const float shaftLength = 1.0f;
    const float shaftRadius = 0.02f;
    const float coneLength = 0.2f;
    const float coneRadius = 0.06f;
    const int segments = 8;

    std::vector<float> vertices;

    // Create arrow pointing along +X
    // Shaft (cylinder approximation as lines for simplicity)
    vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
    vertices.push_back(shaftLength); vertices.push_back(0.0f); vertices.push_back(0.0f);

    // Cone (simplified as lines from tip to base circle)
    for (int i = 0; i < segments; i++) {
        float angle = (float)i / segments * 2.0f * 3.14159f;
        float nextAngle = (float)(i + 1) / segments * 2.0f * 3.14159f;

        // Line from tip to base vertex
        vertices.push_back(shaftLength + coneLength);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);

        vertices.push_back(shaftLength);
        vertices.push_back(cos(angle) * coneRadius);
        vertices.push_back(sin(angle) * coneRadius);

        // Base circle segment
        vertices.push_back(shaftLength);
        vertices.push_back(cos(angle) * coneRadius);
        vertices.push_back(sin(angle) * coneRadius);

        vertices.push_back(shaftLength);
        vertices.push_back(cos(nextAngle) * coneRadius);
        vertices.push_back(sin(nextAngle) * coneRadius);
    }

    m_ArrowVertexCount = static_cast<uint32_t>(vertices.size() / 3);

    m_ArrowVBO = std::make_unique<Onyx::VertexBuffer>(
        static_cast<const void*>(vertices.data()),
        static_cast<uint32_t>(vertices.size() * sizeof(float))
    );
    m_ArrowVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout layout;
    layout.PushFloat(3);  // Position only

    m_ArrowVAO->SetVertexBuffer(m_ArrowVBO.get());
    m_ArrowVAO->SetLayout(layout);
}

void TransformGizmo::CreatePlaneMesh() {
    // Small quad for 2-axis manipulation
    float size = 0.25f;
    float vertices[] = {
        0.0f, 0.0f, 0.0f,
        size, 0.0f, 0.0f,
        size, size, 0.0f,
        0.0f, size, 0.0f
    };

    m_PlaneVBO = std::make_unique<Onyx::VertexBuffer>(
        static_cast<const void*>(vertices),
        sizeof(vertices)
    );
    m_PlaneVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout layout;
    layout.PushFloat(3);

    m_PlaneVAO->SetVertexBuffer(m_PlaneVBO.get());
    m_PlaneVAO->SetLayout(layout);
}

void TransformGizmo::CreateCubeMesh() {
    // Small cube for scale handles
    float s = 0.05f;
    float vertices[] = {
        -s, -s,  s,
         s, -s,  s,
         s,  s,  s,
        -s,  s,  s,
        -s, -s, -s,
         s, -s, -s,
         s,  s, -s,
        -s,  s, -s
    };

    uint32_t indices[] = {
        0, 1, 2,  0, 2, 3,  // Front
        4, 6, 5,  4, 7, 6,  // Back
        3, 2, 6,  3, 6, 7,  // Top
        0, 5, 1,  0, 4, 5,  // Bottom
        1, 5, 6,  1, 6, 2,  // Right
        0, 3, 7,  0, 7, 4   // Left
    };

    m_CubeVBO = std::make_unique<Onyx::VertexBuffer>(
        static_cast<const void*>(vertices),
        sizeof(vertices)
    );
    m_CubeEBO = std::make_unique<Onyx::IndexBuffer>(
        static_cast<const void*>(indices),
        sizeof(indices)
    );
    m_CubeVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout layout;
    layout.PushFloat(3);

    m_CubeVAO->SetVertexBuffer(m_CubeVBO.get());
    m_CubeVAO->SetIndexBuffer(m_CubeEBO.get());
    m_CubeVAO->SetLayout(layout);
}

void TransformGizmo::CreateCircleMesh() {
    // Circle for rotation gizmo (in XY plane, will be rotated for each axis)
    const int segments = 64;
    const float radius = 1.0f;

    std::vector<float> vertices;

    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2.0f * 3.14159265f;
        vertices.push_back(cos(angle) * radius);
        vertices.push_back(sin(angle) * radius);
        vertices.push_back(0.0f);
    }

    m_CircleVertexCount = segments + 1;

    m_CircleVBO = std::make_unique<Onyx::VertexBuffer>(
        static_cast<const void*>(vertices.data()),
        static_cast<uint32_t>(vertices.size() * sizeof(float))
    );
    m_CircleVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout layout;
    layout.PushFloat(3);

    m_CircleVAO->SetVertexBuffer(m_CircleVBO.get());
    m_CircleVAO->SetLayout(layout);
}

void TransformGizmo::Render(const glm::mat4& view, const glm::mat4& projection,
                             const glm::vec3& position, float cameraDistance) {
    // Scale gizmo based on camera distance so it stays same screen size
    float scale = cameraDistance * 0.15f;

    glDisable(GL_DEPTH_TEST);  // Draw gizmo on top

    switch (m_Mode) {
        case GizmoMode::TRANSLATE:
            RenderTranslateGizmo(view, projection, position, scale);
            break;
        case GizmoMode::ROTATE:
            RenderRotateGizmo(view, projection, position, scale);
            break;
        case GizmoMode::SCALE:
            RenderScaleGizmo(view, projection, position, scale);
            break;
        default:
            break;
    }

    glEnable(GL_DEPTH_TEST);
}

void TransformGizmo::RenderTranslateGizmo(const glm::mat4& view, const glm::mat4& projection,
                                           const glm::vec3& position, float scale) {
    m_GizmoShader->Bind();
    m_GizmoShader->SetMat4("u_View", const_cast<glm::mat4&>(view));
    m_GizmoShader->SetMat4("u_Projection", const_cast<glm::mat4&>(projection));

    glLineWidth(2.0f);

    // X axis (red)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::X) ? m_HighlightColor : m_XColor;
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        glBindVertexArray(m_ArrowVAO->GetBufferID());
        glDrawArrays(GL_LINES, 0, m_ArrowVertexCount);
    }

    // Y axis (green) - rotate arrow to point up
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::Y) ? m_HighlightColor : m_YColor;
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        glBindVertexArray(m_ArrowVAO->GetBufferID());
        glDrawArrays(GL_LINES, 0, m_ArrowVertexCount);
    }

    // Z axis (blue) - rotate arrow to point forward
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::Z) ? m_HighlightColor : m_ZColor;
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        glBindVertexArray(m_ArrowVAO->GetBufferID());
        glDrawArrays(GL_LINES, 0, m_ArrowVertexCount);
    }

    // XY plane (small quad)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::XY) ? m_HighlightColor : glm::vec3(0.9f, 0.9f, 0.2f);
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        glBindVertexArray(m_PlaneVAO->GetBufferID());
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    // XZ plane
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::XZ) ? m_HighlightColor : glm::vec3(0.9f, 0.5f, 0.2f);
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        glBindVertexArray(m_PlaneVAO->GetBufferID());
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    // YZ plane
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::YZ) ? m_HighlightColor : glm::vec3(0.2f, 0.9f, 0.9f);
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        glBindVertexArray(m_PlaneVAO->GetBufferID());
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void TransformGizmo::RenderRotateGizmo(const glm::mat4& view, const glm::mat4& projection,
                                        const glm::vec3& position, float scale) {
    m_GizmoShader->Bind();
    m_GizmoShader->SetMat4("u_View", const_cast<glm::mat4&>(view));
    m_GizmoShader->SetMat4("u_Projection", const_cast<glm::mat4&>(projection));

    glLineWidth(2.0f);

    // X axis rotation (rotate around X, so circle is in YZ plane)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::X) ? m_HighlightColor : m_XColor;
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        glBindVertexArray(m_CircleVAO->GetBufferID());
        glDrawArrays(GL_LINE_STRIP, 0, m_CircleVertexCount);
    }

    // Y axis rotation (rotate around Y, so circle is in XZ plane)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::Y) ? m_HighlightColor : m_YColor;
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        glBindVertexArray(m_CircleVAO->GetBufferID());
        glDrawArrays(GL_LINE_STRIP, 0, m_CircleVertexCount);
    }

    // Z axis rotation (rotate around Z, so circle is in XY plane)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::Z) ? m_HighlightColor : m_ZColor;
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        glBindVertexArray(m_CircleVAO->GetBufferID());
        glDrawArrays(GL_LINE_STRIP, 0, m_CircleVertexCount);
    }

    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void TransformGizmo::RenderScaleGizmo(const glm::mat4& view, const glm::mat4& projection,
                                       const glm::vec3& position, float scale) {
    // Similar to translate but with cubes at the ends instead of cones
    m_GizmoShader->Bind();
    m_GizmoShader->SetMat4("u_View", const_cast<glm::mat4&>(view));
    m_GizmoShader->SetMat4("u_Projection", const_cast<glm::mat4&>(projection));

    glLineWidth(2.0f);

    float axisLength = 1.0f;

    // X axis line + cube
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(scale));
        m_GizmoShader->SetMat4("u_Model", model);

        glm::vec3 color = (m_ActiveAxis == GizmoAxis::X) ? m_HighlightColor : m_XColor;
        m_GizmoShader->SetVec3("u_Color", color.x, color.y, color.z);

        // Draw line
        glBindVertexArray(m_ArrowVAO->GetBufferID());
        glDrawArrays(GL_LINES, 0, 2);  // Just the shaft

        // Draw cube at end
        glm::mat4 cubeModel = glm::translate(glm::mat4(1.0f), position + glm::vec3(axisLength * scale, 0.0f, 0.0f));
        cubeModel = glm::scale(cubeModel, glm::vec3(scale));
        m_GizmoShader->SetMat4("u_Model", cubeModel);

        glBindVertexArray(m_CubeVAO->GetBufferID());
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }

    // Similar for Y and Z...

    glBindVertexArray(0);
    glLineWidth(1.0f);
}

GizmoAxis TransformGizmo::TestHit(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                   const glm::vec3& gizmoPos, float cameraDistance) {
    float scale = cameraDistance * 0.15f;
    float hitThreshold = 0.25f * scale;

    GizmoAxis closest = GizmoAxis::NONE;
    float minDist = std::numeric_limits<float>::max();

    if (m_Mode == GizmoMode::TRANSLATE || m_Mode == GizmoMode::SCALE) {
        // Test axis lines
        float xDist, yDist, zDist;
        bool xHit = RayIntersectsLine(rayOrigin, rayDir, gizmoPos, gizmoPos + glm::vec3(scale, 0, 0), hitThreshold, xDist);
        bool yHit = RayIntersectsLine(rayOrigin, rayDir, gizmoPos, gizmoPos + glm::vec3(0, scale, 0), hitThreshold, yDist);
        bool zHit = RayIntersectsLine(rayOrigin, rayDir, gizmoPos, gizmoPos + glm::vec3(0, 0, scale), hitThreshold, zDist);

        if (xHit && xDist < minDist) { minDist = xDist; closest = GizmoAxis::X; }
        if (yHit && yDist < minDist) { minDist = yDist; closest = GizmoAxis::Y; }
        if (zHit && zDist < minDist) { minDist = zDist; closest = GizmoAxis::Z; }
    }
    else if (m_Mode == GizmoMode::ROTATE) {
        // Test rotation circles - check ray-plane intersection and distance from circle
        float circleThreshold = hitThreshold * 1.5f;

        // X rotation (YZ plane)
        {
            glm::vec3 planeNormal(1, 0, 0);
            float denom = glm::dot(planeNormal, rayDir);
            if (std::abs(denom) > 0.001f) {
                float t = glm::dot(gizmoPos - rayOrigin, planeNormal) / denom;
                if (t > 0) {
                    glm::vec3 hitPoint = rayOrigin + t * rayDir;
                    float distFromCenter = glm::length(hitPoint - gizmoPos);
                    float distFromCircle = std::abs(distFromCenter - scale);
                    if (distFromCircle < circleThreshold && t < minDist) {
                        minDist = t;
                        closest = GizmoAxis::X;
                    }
                }
            }
        }

        // Y rotation (XZ plane)
        {
            glm::vec3 planeNormal(0, 1, 0);
            float denom = glm::dot(planeNormal, rayDir);
            if (std::abs(denom) > 0.001f) {
                float t = glm::dot(gizmoPos - rayOrigin, planeNormal) / denom;
                if (t > 0) {
                    glm::vec3 hitPoint = rayOrigin + t * rayDir;
                    float distFromCenter = glm::length(hitPoint - gizmoPos);
                    float distFromCircle = std::abs(distFromCenter - scale);
                    if (distFromCircle < circleThreshold && t < minDist) {
                        minDist = t;
                        closest = GizmoAxis::Y;
                    }
                }
            }
        }

        // Z rotation (XY plane)
        {
            glm::vec3 planeNormal(0, 0, 1);
            float denom = glm::dot(planeNormal, rayDir);
            if (std::abs(denom) > 0.001f) {
                float t = glm::dot(gizmoPos - rayOrigin, planeNormal) / denom;
                if (t > 0) {
                    glm::vec3 hitPoint = rayOrigin + t * rayDir;
                    float distFromCenter = glm::length(hitPoint - gizmoPos);
                    float distFromCircle = std::abs(distFromCenter - scale);
                    if (distFromCircle < circleThreshold && t < minDist) {
                        minDist = t;
                        closest = GizmoAxis::Z;
                    }
                }
            }
        }
    }

    return closest;
}

bool TransformGizmo::RayIntersectsLine(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                        const glm::vec3& lineStart, const glm::vec3& lineEnd,
                                        float threshold, float& distance) {
    // Find closest point between ray and line segment
    glm::vec3 u = rayDir;
    glm::vec3 v = lineEnd - lineStart;
    glm::vec3 w = rayOrigin - lineStart;

    float a = glm::dot(u, u);
    float b = glm::dot(u, v);
    float c = glm::dot(v, v);
    float d = glm::dot(u, w);
    float e = glm::dot(v, w);

    float denom = a * c - b * b;
    if (std::abs(denom) < 1e-6f) {
        return false;  // Lines are parallel
    }

    float sc = (b * e - c * d) / denom;
    float tc = (a * e - b * d) / denom;

    // Clamp tc to [0, 1] (line segment)
    tc = glm::clamp(tc, 0.0f, 1.0f);

    // Recalculate sc based on clamped tc (from equation: d + sc*a - tc*b = 0)
    sc = (b * tc - d) / a;
    if (sc < 0.0f) sc = 0.0f;

    // Closest points
    glm::vec3 rayPoint = rayOrigin + sc * u;
    glm::vec3 linePoint = lineStart + tc * v;

    float dist = glm::length(rayPoint - linePoint);
    distance = sc;  // Return distance along ray

    return dist < threshold;
}

glm::vec3 TransformGizmo::CalculateTranslation(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                                const glm::vec3& gizmoPos, GizmoAxis axis,
                                                const glm::mat4& view, const glm::mat4& projection) {
    if (!m_IsDragging) return glm::vec3(0.0f);

    // Ray-plane intersection using the stored plane normal from BeginDrag
    float denom = glm::dot(m_DragPlaneNormal, rayDir);
    if (std::abs(denom) < 0.001f) {
        return glm::vec3(0.0f);
    }

    // Intersect with plane at the original object position
    float t = glm::dot(m_DragStartPos - rayOrigin, m_DragPlaneNormal) / denom;
    glm::vec3 currentHitPoint = rayOrigin + t * rayDir;

    // Calculate delta from initial hit point to current hit point
    glm::vec3 delta = currentHitPoint - m_DragStartHitPoint;

    // Project delta onto the constrained axis
    float projection_val = glm::dot(delta, m_DragAxisDir);
    glm::vec3 result = m_DragAxisDir * projection_val;

    // Apply snapping
    if (m_SnapEnabled && m_SnapValue > 0.0f) {
        result.x = std::round(result.x / m_SnapValue) * m_SnapValue;
        result.y = std::round(result.y / m_SnapValue) * m_SnapValue;
        result.z = std::round(result.z / m_SnapValue) * m_SnapValue;
    }

    return result;
}

void TransformGizmo::BeginDrag(const glm::vec3& objectPos, const glm::vec3& rayOrigin,
                                const glm::vec3& rayDir, const glm::mat4& view) {
    m_IsDragging = true;
    m_DragStartPos = objectPos;

    // Determine axis direction based on active axis
    switch (m_ActiveAxis) {
        case GizmoAxis::X: m_DragAxisDir = glm::vec3(1, 0, 0); break;
        case GizmoAxis::Y: m_DragAxisDir = glm::vec3(0, 1, 0); break;
        case GizmoAxis::Z: m_DragAxisDir = glm::vec3(0, 0, 1); break;
        default: m_DragAxisDir = glm::vec3(0, 0, 0); break;
    }

    // Calculate the constraint plane
    glm::vec3 viewDir = glm::normalize(glm::vec3(glm::inverse(view)[2]));

    if (m_Mode == GizmoMode::ROTATE) {
        // For rotation, the plane normal IS the rotation axis
        m_DragPlaneNormal = m_DragAxisDir;
    } else {
        // For translate/scale, plane is perpendicular to camera but containing the axis
        m_DragPlaneNormal = glm::cross(m_DragAxisDir, glm::cross(viewDir, m_DragAxisDir));
        if (glm::length(m_DragPlaneNormal) < 0.001f) {
            m_DragPlaneNormal = viewDir;
        }
        m_DragPlaneNormal = glm::normalize(m_DragPlaneNormal);
    }

    // Calculate where the initial click hits the constraint plane
    float denom = glm::dot(m_DragPlaneNormal, rayDir);
    if (std::abs(denom) > 0.001f) {
        float t = glm::dot(objectPos - rayOrigin, m_DragPlaneNormal) / denom;
        m_DragStartHitPoint = rayOrigin + t * rayDir;
    } else {
        m_DragStartHitPoint = objectPos;
    }

    // For rotation: calculate initial angle
    if (m_Mode == GizmoMode::ROTATE) {
        glm::vec3 toHit = m_DragStartHitPoint - objectPos;
        // Project onto the rotation plane and get angle
        glm::vec3 projX, projY;
        if (m_ActiveAxis == GizmoAxis::X) {
            projX = glm::vec3(0, 1, 0);
            projY = glm::vec3(0, 0, 1);
        } else if (m_ActiveAxis == GizmoAxis::Y) {
            projX = glm::vec3(1, 0, 0);
            projY = glm::vec3(0, 0, 1);
        } else {
            projX = glm::vec3(1, 0, 0);
            projY = glm::vec3(0, 1, 0);
        }
        m_DragStartAngle = std::atan2(glm::dot(toHit, projY), glm::dot(toHit, projX));
    }
}

float TransformGizmo::CalculateRotation(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                         const glm::vec3& gizmoPos, GizmoAxis axis,
                                         const glm::mat4& view) {
    if (!m_IsDragging) return 0.0f;

    // For rotation, the plane normal IS the axis
    glm::vec3 planeNormal = m_DragAxisDir;

    float denom = glm::dot(planeNormal, rayDir);
    if (std::abs(denom) < 0.001f) {
        return 0.0f;
    }

    float t = glm::dot(m_DragStartPos - rayOrigin, planeNormal) / denom;
    glm::vec3 currentHitPoint = rayOrigin + t * rayDir;

    // Calculate current angle
    glm::vec3 toHit = currentHitPoint - m_DragStartPos;

    glm::vec3 projX, projY;
    if (axis == GizmoAxis::X) {
        projX = glm::vec3(0, 1, 0);
        projY = glm::vec3(0, 0, 1);
    } else if (axis == GizmoAxis::Y) {
        projX = glm::vec3(1, 0, 0);
        projY = glm::vec3(0, 0, 1);
    } else {
        projX = glm::vec3(1, 0, 0);
        projY = glm::vec3(0, 1, 0);
    }

    float currentAngle = std::atan2(glm::dot(toHit, projY), glm::dot(toHit, projX));
    float deltaAngle = currentAngle - m_DragStartAngle;

    // Invert Y axis rotation to match intuitive behavior (right = clockwise from above)
    if (axis == GizmoAxis::Y) {
        deltaAngle = -deltaAngle;
    }

    // Apply snapping (15 degree increments)
    if (m_SnapEnabled) {
        float snapAngle = glm::radians(15.0f);
        deltaAngle = std::round(deltaAngle / snapAngle) * snapAngle;
    }

    return deltaAngle;
}

glm::vec3 TransformGizmo::CalculateScale(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                          const glm::vec3& gizmoPos, GizmoAxis axis,
                                          const glm::mat4& view) {
    if (!m_IsDragging) return glm::vec3(1.0f);

    // Use same plane as translation
    float denom = glm::dot(m_DragPlaneNormal, rayDir);
    if (std::abs(denom) < 0.001f) {
        return glm::vec3(1.0f);
    }

    float t = glm::dot(m_DragStartPos - rayOrigin, m_DragPlaneNormal) / denom;
    glm::vec3 currentHitPoint = rayOrigin + t * rayDir;

    // Calculate scale factor based on distance from center
    float startDist = glm::length(m_DragStartHitPoint - m_DragStartPos);
    float currentDist = glm::length(currentHitPoint - m_DragStartPos);

    if (startDist < 0.001f) startDist = 0.001f;

    float scaleFactor = currentDist / startDist;

    // Clamp to reasonable range
    scaleFactor = glm::clamp(scaleFactor, 0.1f, 10.0f);

    // Apply to the correct axis
    glm::vec3 result(1.0f);
    if (axis == GizmoAxis::X) result.x = scaleFactor;
    else if (axis == GizmoAxis::Y) result.y = scaleFactor;
    else if (axis == GizmoAxis::Z) result.z = scaleFactor;
    else if (axis == GizmoAxis::XYZ) result = glm::vec3(scaleFactor);

    // Apply snapping
    if (m_SnapEnabled && m_SnapValue > 0.0f) {
        result.x = std::round(result.x / 0.1f) * 0.1f;
        result.y = std::round(result.y / 0.1f) * 0.1f;
        result.z = std::round(result.z / 0.1f) * 0.1f;
    }

    return result;
}

void TransformGizmo::EndDrag() {
    m_IsDragging = false;
    m_ActiveAxis = GizmoAxis::NONE;
}

} // namespace MMO
