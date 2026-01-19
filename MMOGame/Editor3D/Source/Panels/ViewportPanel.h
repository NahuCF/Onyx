#pragma once

#include <Graphics/Shader.h>
#include <Graphics/Framebuffer.h>
#include <Graphics/Buffers.h>
#include <Graphics/Texture.h>
#include <Graphics/Model.h>
#include "World/WorldTypes.h"
#include "World/StaticObject.h"
#include "Gizmo/TransformGizmo.h"
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

namespace MMO {

class EditorWorld;

class ViewportPanel {
public:
    ViewportPanel();
    ~ViewportPanel() = default;

    void Init(EditorWorld* world);
    void OnImGuiRender();

    // Getters for camera state (other systems may need these)
    const glm::vec3& GetCameraPosition() const { return m_CameraPosition; }
    const glm::vec3& GetCameraFront() const { return m_CameraFront; }
    const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }

    float GetViewportWidth() const { return m_ViewportWidth; }
    float GetViewportHeight() const { return m_ViewportHeight; }
    bool IsHovered() const { return m_ViewportHovered; }
    bool IsFocused() const { return m_ViewportFocused; }

    // Statistics getters
    uint32_t GetTriangleCount() const { return m_TriangleCount; }
    uint32_t GetDrawCalls() const { return m_DrawCalls; }

    // Focus camera on object
    void FocusOnObject(const WorldObject* object);
    void FocusOnSelection();

    // Model cache access (for InspectorPanel)
    Onyx::Model* GetModel(const std::string& path);

private:
    void RenderScene();
    void RenderGrid();
    void RenderWorldObjects();
    void RenderSelectionOutline();
    void RenderGizmoIcons();
    void RenderGizmo();

    void HandleCameraInput();
    void HandleObjectPicking();
    void HandleGizmoInteraction();
    void UpdateCameraVectors();

    // GPU Picking
    struct PickResult {
        uint64_t objectGuid = 0;
        int meshIndex = -1;
        WorldObjectType type = WorldObjectType::STATIC_OBJECT;
        bool hit = false;
    };

    void RenderPickingPass();
    PickResult ReadPickingBuffer(float mouseX, float mouseY);
    void RenderObjectForPicking(StaticObject* obj);
    void RenderIconForPicking(WorldObject* obj, WorldObjectType type, float size);

    // Ray casting for picking (kept as fallback)
    glm::vec3 ScreenToWorldRay(float screenX, float screenY);

    // World reference
    EditorWorld* m_World = nullptr;

    // Framebuffer for viewport
    std::unique_ptr<Onyx::Framebuffer> m_Framebuffer;

    // GPU Picking framebuffer (non-multisampled for pixel reading)
    std::unique_ptr<Onyx::Framebuffer> m_PickingFramebuffer;

    // Billboard quad for icons
    std::unique_ptr<Onyx::VertexArray> m_BillboardVAO;
    std::unique_ptr<Onyx::VertexBuffer> m_BillboardVBO;
    std::unique_ptr<Onyx::IndexBuffer> m_BillboardEBO;

    // Icon textures for different entity types
    std::unique_ptr<Onyx::Texture> m_LightIconTexture;
    std::unique_ptr<Onyx::Texture> m_SpawnIconTexture;
    std::unique_ptr<Onyx::Texture> m_ParticleIconTexture;
    std::unique_ptr<Onyx::Texture> m_PortalIconTexture;
    std::unique_ptr<Onyx::Texture> m_TriggerIconTexture;

    // Shaders
    std::unique_ptr<Onyx::Shader> m_ObjectShader;     // For rendering simple objects
    std::unique_ptr<Onyx::Shader> m_ModelShader;      // For rendering loaded 3D models
    std::unique_ptr<Onyx::Shader> m_InfiniteGridShader; // For infinite ground grid
    std::unique_ptr<Onyx::Shader> m_OutlineShader;    // For selection outline
    std::unique_ptr<Onyx::Shader> m_IconShader;       // For gizmo icons
    std::unique_ptr<Onyx::Shader> m_PickingShader;    // For GPU picking (solid geometry)
    std::unique_ptr<Onyx::Shader> m_PickingBillboardShader; // For GPU picking (billboards)

    // Primitive geometry
    std::unique_ptr<Onyx::VertexArray> m_CubeVAO;
    std::unique_ptr<Onyx::VertexBuffer> m_CubeVBO;
    std::unique_ptr<Onyx::IndexBuffer> m_CubeEBO;

    // Grid geometry (fullscreen quad for infinite grid shader)
    std::unique_ptr<Onyx::VertexArray> m_GridVAO;
    std::unique_ptr<Onyx::VertexBuffer> m_GridVBO;
    std::unique_ptr<Onyx::IndexBuffer> m_GridEBO;

    // Camera state
    glm::vec3 m_CameraPosition = glm::vec3(0.0f, 10.0f, 20.0f);
    glm::vec3 m_CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_CameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
    float m_CameraYaw = -90.0f;
    float m_CameraPitch = -20.0f;
    float m_CameraSpeed = 20.0f;
    float m_CameraSensitivity = 0.15f;

    // Cached matrices
    glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
    glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);

    // Viewport state
    float m_ViewportWidth = 800.0f;
    float m_ViewportHeight = 600.0f;
    bool m_ViewportHovered = false;
    bool m_ViewportFocused = false;
    glm::vec2 m_ViewportPos = glm::vec2(0.0f);  // Screen position of viewport

    // Mouse state for camera
    bool m_RightMouseDown = false;
    bool m_MiddleMouseDown = false;
    bool m_FirstMouse = true;
    bool m_WantsContextMenu = false;  // Right-click context menu pending

    // Visual settings
    glm::vec3 m_LightDir = glm::vec3(-0.5f, -1.0f, -0.3f);
    float m_AmbientStrength = 0.3f;
    bool m_ShowGrid = true;
    bool m_ShowWireframe = false;
    bool m_EnableMSAA = true;  // 4x MSAA toggle
    float m_GridSize = 100.0f;
    float m_GridSpacing = 1.0f;

    // Statistics
    uint32_t m_TriangleCount = 0;
    uint32_t m_DrawCalls = 0;

    // Transform gizmo
    std::unique_ptr<TransformGizmo> m_Gizmo;
    glm::vec3 m_GizmoStartObjectPos = glm::vec3(0.0f);
    glm::quat m_GizmoStartObjectRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float m_GizmoStartObjectScale = 1.0f;

    // Mesh transform gizmo state
    glm::vec3 m_GizmoStartMeshOffset = glm::vec3(0.0f);
    glm::vec3 m_GizmoStartMeshRotation = glm::vec3(0.0f);
    float m_GizmoStartMeshScale = 1.0f;
    std::string m_GizmoSelectedMeshName;

    // Model cache - maps path to loaded model
    std::unordered_map<std::string, std::unique_ptr<Onyx::Model>> m_ModelCache;

    // Texture cache - maps path to Texture object
    std::unordered_map<std::string, std::unique_ptr<Onyx::Texture>> m_TextureCache;

    // Default white texture for models without textures
    std::unique_ptr<Onyx::Texture> m_DefaultWhiteTexture;

    // Get or load a model from cache
    Onyx::Model* GetOrLoadModel(const std::string& path);

    // Get or load a texture from cache
    Onyx::Texture* GetOrLoadTexture(const std::string& path);

    // Create default textures
    void CreateDefaultTextures();
};

} // namespace MMO
