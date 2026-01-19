#include "ViewportPanel.h"
#include "World/EditorWorld.h"
#include "Commands/EditorCommand.h"
#include <Graphics/RenderCommand.h>
#include <Graphics/VertexLayout.h>
#include <GL/glew.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <limits>
#include <iostream>

namespace MMO {

ViewportPanel::ViewportPanel() = default;

void ViewportPanel::Init(EditorWorld* world) {
    m_World = world;

    // Create framebuffer with 4x MSAA
    m_Framebuffer = std::make_unique<Onyx::Framebuffer>();
    m_Framebuffer->Create(800, 600, 4);  // 4x MSAA

    // Create object shader (for simple objects like grid, placeholder cubes)
    m_ObjectShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/basic3d.vert",
        "MMOGame/Editor3D/assets/shaders/basic3d.frag"
    );

    // Create model shader (for loaded 3D models with textures)
    m_ModelShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/model.vert",
        "MMOGame/Editor3D/assets/shaders/model.frag"
    );

    // Create cube geometry
    float cubeVertices[] = {
        // Position              // Color (will be replaced by uniform)
        // Front face
        -0.5f, -0.5f,  0.5f,    0.8f, 0.8f, 0.8f,
         0.5f, -0.5f,  0.5f,    0.8f, 0.8f, 0.8f,
         0.5f,  0.5f,  0.5f,    0.9f, 0.9f, 0.9f,
        -0.5f,  0.5f,  0.5f,    0.9f, 0.9f, 0.9f,
        // Back face
        -0.5f, -0.5f, -0.5f,    0.7f, 0.7f, 0.7f,
         0.5f, -0.5f, -0.5f,    0.7f, 0.7f, 0.7f,
         0.5f,  0.5f, -0.5f,    0.8f, 0.8f, 0.8f,
        -0.5f,  0.5f, -0.5f,    0.8f, 0.8f, 0.8f,
        // Top face
        -0.5f,  0.5f, -0.5f,    0.85f, 0.85f, 0.85f,
         0.5f,  0.5f, -0.5f,    0.85f, 0.85f, 0.85f,
         0.5f,  0.5f,  0.5f,    0.95f, 0.95f, 0.95f,
        -0.5f,  0.5f,  0.5f,    0.95f, 0.95f, 0.95f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,    0.6f, 0.6f, 0.6f,
         0.5f, -0.5f, -0.5f,    0.6f, 0.6f, 0.6f,
         0.5f, -0.5f,  0.5f,    0.7f, 0.7f, 0.7f,
        -0.5f, -0.5f,  0.5f,    0.7f, 0.7f, 0.7f,
        // Right face
         0.5f, -0.5f, -0.5f,    0.75f, 0.75f, 0.75f,
         0.5f,  0.5f, -0.5f,    0.75f, 0.75f, 0.75f,
         0.5f,  0.5f,  0.5f,    0.85f, 0.85f, 0.85f,
         0.5f, -0.5f,  0.5f,    0.85f, 0.85f, 0.85f,
        // Left face
        -0.5f, -0.5f, -0.5f,    0.65f, 0.65f, 0.65f,
        -0.5f,  0.5f, -0.5f,    0.65f, 0.65f, 0.65f,
        -0.5f,  0.5f,  0.5f,    0.75f, 0.75f, 0.75f,
        -0.5f, -0.5f,  0.5f,    0.75f, 0.75f, 0.75f,
    };

    uint32_t cubeIndices[] = {
        0, 1, 2,  0, 2, 3,    // Front
        4, 6, 5,  4, 7, 6,    // Back
        8, 9, 10,  8, 10, 11, // Top
        12, 14, 13,  12, 15, 14, // Bottom
        16, 17, 18,  16, 18, 19, // Right
        20, 22, 21,  20, 23, 22, // Left
    };

    m_CubeVBO = std::make_unique<Onyx::VertexBuffer>(static_cast<const void*>(cubeVertices), sizeof(cubeVertices));
    m_CubeEBO = std::make_unique<Onyx::IndexBuffer>(static_cast<const void*>(cubeIndices), sizeof(cubeIndices));
    m_CubeVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout cubeLayout;
    cubeLayout.PushFloat(3);  // Position
    cubeLayout.PushFloat(3);  // Color

    m_CubeVAO->SetVertexBuffer(m_CubeVBO.get());
    m_CubeVAO->SetIndexBuffer(m_CubeEBO.get());
    m_CubeVAO->SetLayout(cubeLayout);

    // Create infinite grid shader
    m_InfiniteGridShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/infinite_grid.vert",
        "MMOGame/Editor3D/assets/shaders/infinite_grid.frag"
    );

    // Create fullscreen quad for infinite grid
    // These are NDC coordinates that will be unprojected in the shader
    float gridVertices[] = {
        // Position (XY in NDC)
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    uint32_t gridIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    m_GridVBO = std::make_unique<Onyx::VertexBuffer>(static_cast<const void*>(gridVertices), sizeof(gridVertices));
    m_GridEBO = std::make_unique<Onyx::IndexBuffer>(static_cast<const void*>(gridIndices), sizeof(gridIndices));
    m_GridVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout gridLayout;
    gridLayout.PushFloat(2);  // Position (XY)

    m_GridVAO->SetVertexBuffer(m_GridVBO.get());
    m_GridVAO->SetIndexBuffer(m_GridEBO.get());
    m_GridVAO->SetLayout(gridLayout);

    // Initialize camera
    UpdateCameraVectors();

    // Initialize gizmo
    m_Gizmo = std::make_unique<TransformGizmo>();
    m_Gizmo->Init();

    // Create default textures
    CreateDefaultTextures();

    // ===== GPU Picking Setup =====

    // Create picking framebuffer (no MSAA - need exact colors for ID reading)
    m_PickingFramebuffer = std::make_unique<Onyx::Framebuffer>();
    m_PickingFramebuffer->Create(800, 600, 1);  // No MSAA

    // Create picking shaders
    m_PickingShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/picking.vert",
        "MMOGame/Editor3D/assets/shaders/picking.frag"
    );

    m_PickingBillboardShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/picking_billboard.vert",
        "MMOGame/Editor3D/assets/shaders/picking_billboard.frag"
    );

    // Create billboard quad for icons
    float billboardVertices[] = {
        // Position (XY)   // TexCoord
        -0.5f, -0.5f,      0.0f, 0.0f,
         0.5f, -0.5f,      1.0f, 0.0f,
         0.5f,  0.5f,      1.0f, 1.0f,
        -0.5f,  0.5f,      0.0f, 1.0f
    };

    uint32_t billboardIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    m_BillboardVBO = std::make_unique<Onyx::VertexBuffer>(static_cast<const void*>(billboardVertices), sizeof(billboardVertices));
    m_BillboardEBO = std::make_unique<Onyx::IndexBuffer>(static_cast<const void*>(billboardIndices), sizeof(billboardIndices));
    m_BillboardVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout billboardLayout;
    billboardLayout.PushFloat(2);  // Position (XY)
    billboardLayout.PushFloat(2);  // TexCoord

    m_BillboardVAO->SetVertexBuffer(m_BillboardVBO.get());
    m_BillboardVAO->SetIndexBuffer(m_BillboardEBO.get());
    m_BillboardVAO->SetLayout(billboardLayout);

    // Create simple colored icon textures (circles with different colors)
    // Yellow for lights
    m_LightIconTexture = Onyx::Texture::CreateSolidColor(255, 220, 50);
    // Green for spawn points
    m_SpawnIconTexture = Onyx::Texture::CreateSolidColor(50, 220, 100);
    // Magenta for particle emitters
    m_ParticleIconTexture = Onyx::Texture::CreateSolidColor(220, 50, 220);
    // Cyan for portals
    m_PortalIconTexture = Onyx::Texture::CreateSolidColor(50, 200, 255);
    // Orange for triggers
    m_TriggerIconTexture = Onyx::Texture::CreateSolidColor(255, 150, 50);
}

void ViewportPanel::OnImGuiRender() {
    ImGuiWindowFlags vpFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", nullptr, vpFlags);

    m_ViewportFocused = ImGui::IsWindowFocused();
    m_ViewportHovered = ImGui::IsWindowHovered();

    // Get viewport position for mouse picking
    ImVec2 vpPos = ImGui::GetCursorScreenPos();
    m_ViewportPos = glm::vec2(vpPos.x, vpPos.y);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    int newWidth = std::max(1, static_cast<int>(viewportSize.x));
    int newHeight = std::max(1, static_cast<int>(viewportSize.y));

    if (newWidth != static_cast<int>(m_ViewportWidth) || newHeight != static_cast<int>(m_ViewportHeight)) {
        m_ViewportWidth = static_cast<float>(newWidth);
        m_ViewportHeight = static_cast<float>(newHeight);
        uint32_t samples = m_EnableMSAA ? 4 : 1;
        m_Framebuffer->Create(newWidth, newHeight, samples);
        // Resize picking framebuffer (full resolution for accurate picking)
        m_PickingFramebuffer->Create(newWidth, newHeight, 1);
    }

    // Check for right-click context menu BEFORE camera input captures it
    // Only open menu if not already in camera rotation mode
    if (m_ViewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !m_RightMouseDown) {
        m_WantsContextMenu = true;
    }

    // Handle input before rendering
    HandleCameraInput();

    // Update matrices BEFORE picking and rendering (picking needs current matrices)
    m_ViewMatrix = glm::lookAt(m_CameraPosition, m_CameraPosition + m_CameraFront, m_CameraUp);
    m_ProjectionMatrix = glm::perspective(
        glm::radians(60.0f),
        m_ViewportWidth / m_ViewportHeight,
        0.1f,
        1000.0f
    );

    HandleGizmoInteraction();
    HandleObjectPicking();

    // Render scene to framebuffer
    RenderScene();

    // Display framebuffer
    ImGui::Image(
        static_cast<ImTextureID>(m_Framebuffer->GetColorBufferID()),
        ImVec2(m_ViewportWidth, m_ViewportHeight),
        ImVec2(0, 1), ImVec2(1, 0)
    );

    // Overlay controls
    ImGui::SetCursorPos(ImVec2(10, 30));
    ImGui::Checkbox("Show Grid", &m_ShowGrid);
    ImGui::Checkbox("Wireframe", &m_ShowWireframe);
    if (ImGui::Checkbox("MSAA 4x", &m_EnableMSAA)) {
        // Recreate framebuffer with new sample count
        uint32_t samples = m_EnableMSAA ? 4 : 1;
        m_Framebuffer->Create(static_cast<uint32_t>(m_ViewportWidth),
                              static_cast<uint32_t>(m_ViewportHeight), samples);
    }

    // Right-click context menu (open if we detected right-click earlier and mouse was released quickly)
    if (m_WantsContextMenu && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("ViewportContextMenu");
        m_WantsContextMenu = false;
    }
    // Cancel context menu if user starts dragging (camera rotation)
    if (m_WantsContextMenu && m_RightMouseDown) {
        m_WantsContextMenu = false;
    }

    if (ImGui::BeginPopup("ViewportContextMenu")) {
        bool hasSelection = m_World && m_World->HasSelection();
        bool hasGroupSelected = false;

        // Check if any selected object is a group
        if (hasSelection) {
            for (WorldObject* obj : m_World->GetSelectedObjects()) {
                if (obj->GetObjectType() == WorldObjectType::GROUP) {
                    hasGroupSelected = true;
                    break;
                }
            }
        }

        if (ImGui::MenuItem("Group", "Ctrl+G", false, hasSelection && m_World->GetSelectionCount() > 1)) {
            m_World->GroupSelected();
        }
        if (ImGui::MenuItem("Ungroup", "Ctrl+Shift+G", false, hasGroupSelected)) {
            m_World->UngroupSelected();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSelection)) {
            m_World->Duplicate();
        }
        if (ImGui::MenuItem("Delete", "Del", false, hasSelection)) {
            m_World->DeleteSelected();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::RenderScene() {
    // Reset statistics
    m_TriangleCount = 0;
    m_DrawCalls = 0;

    m_Framebuffer->Bind();
    m_Framebuffer->Clear(0.15f, 0.15f, 0.18f, 1.0f);

    // Set explicit GL state - must match picking pass exactly
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);  // Same as picking pass

    // Matrices are already built in OnImGuiRender before picking/rendering

    // Render grid
    if (m_ShowGrid) {
        RenderGrid();
    }

    // Render world objects
    RenderWorldObjects();

    // Render gizmo for selected object
    RenderGizmo();

    Onyx::RenderCommand::ResetState();
    m_Framebuffer->UnBind();

    // Resolve multisampled framebuffer to regular texture for display
    m_Framebuffer->Resolve();
}

void ViewportPanel::RenderGrid() {
    // Enable blending for grid transparency
    Onyx::RenderCommand::EnableBlending();
    // Keep depth test enabled - shader writes correct depth via gl_FragDepth

    m_InfiniteGridShader->Bind();
    m_InfiniteGridShader->SetMat4("u_View", m_ViewMatrix);
    m_InfiniteGridShader->SetMat4("u_Projection", m_ProjectionMatrix);
    m_InfiniteGridShader->SetFloat("u_GridScale", 0.5f);   // Larger cells (2 units)
    m_InfiniteGridShader->SetFloat("u_GridFadeStart", 80.0f);
    m_InfiniteGridShader->SetFloat("u_GridFadeEnd", 200.0f);

    // Draw fullscreen quad (6 indices = 2 triangles)
    Onyx::RenderCommand::DrawIndexed(*m_GridVAO, 6);

    Onyx::RenderCommand::DisableBlending();
}

void ViewportPanel::RenderWorldObjects() {
    if (!m_World) return;

    // Apply global wireframe mode if enabled
    if (m_ShowWireframe) {
        Onyx::RenderCommand::SetWireframeMode(true);
    }

    // Render static objects
    for (const auto& obj : m_World->GetStaticObjects()) {
        if (!obj->IsVisible()) continue;

        // Use world matrix to account for parent hierarchy
        glm::mat4 modelMatrix = m_World->GetWorldMatrix(obj.get());

        // Check if object has a model path
        const std::string& modelPath = obj->GetModelPath();
        if (!modelPath.empty()) {
            // Try to load and render the actual model
            Onyx::Model* model = GetOrLoadModel(modelPath);
            if (model) {
                glm::vec3 lightColor(1.0f);

                m_ModelShader->Bind();
                m_ModelShader->SetMat4("u_Model", modelMatrix);
                m_ModelShader->SetMat4("u_View", m_ViewMatrix);
                m_ModelShader->SetMat4("u_Projection", m_ProjectionMatrix);
                m_ModelShader->SetVec3("u_LightDir", m_LightDir);
                m_ModelShader->SetVec3("u_LightColor", lightColor);
                m_ModelShader->SetVec3("u_ViewPos", m_CameraPosition);
                m_ModelShader->SetFloat("u_AmbientStrength", m_AmbientStrength);
                m_ModelShader->SetInt("u_AlbedoMap", 0);
                m_ModelShader->SetInt("u_NormalMap", 1);

                // Default textures from object properties
                Onyx::Texture* defaultDiffuse = GetOrLoadTexture(obj->GetDiffuseTexture());
                Onyx::Texture* defaultNormal = GetOrLoadTexture(obj->GetNormalTexture());

                // Draw each mesh with per-mesh materials and transforms
                for (size_t meshIdx = 0; meshIdx < model->GetMeshes().size(); meshIdx++) {
                    auto& mesh = model->GetMeshes()[meshIdx];

                    // Look up per-mesh material
                    std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(meshIdx)) : mesh.m_Name;
                    const MeshMaterial* meshMat = obj->GetMeshMaterial(meshName);

                    // Skip hidden meshes
                    if (meshMat && !meshMat->visible) {
                        continue;
                    }

                    // Calculate mesh bounding box center for proper pivot scaling
                    glm::vec3 meshCenter(0.0f);
                    if (!mesh.m_Vertices.empty()) {
                        glm::vec3 minBounds(std::numeric_limits<float>::max());
                        glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
                        for (const auto& v : mesh.m_Vertices) {
                            minBounds = glm::min(minBounds, v.position);
                            maxBounds = glm::max(maxBounds, v.position);
                        }
                        meshCenter = (minBounds + maxBounds) * 0.5f;
                    }

                    // Calculate per-mesh model matrix (base + mesh offset)
                    // Transform order: position offset -> rotation -> scale around mesh center
                    glm::mat4 meshModelMatrix = modelMatrix;
                    if (meshMat) {
                        // Apply per-mesh transform offset
                        meshModelMatrix = glm::translate(meshModelMatrix, meshMat->positionOffset);
                        meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(meshMat->rotationOffset.x), glm::vec3(1, 0, 0));
                        meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(meshMat->rotationOffset.y), glm::vec3(0, 1, 0));
                        meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(meshMat->rotationOffset.z), glm::vec3(0, 0, 1));
                        // Scale around mesh center (not origin)
                        meshModelMatrix = glm::translate(meshModelMatrix, meshCenter);
                        meshModelMatrix = glm::scale(meshModelMatrix, glm::vec3(meshMat->scaleMultiplier));
                        meshModelMatrix = glm::translate(meshModelMatrix, -meshCenter);
                    }
                    m_ModelShader->SetMat4("u_Model", meshModelMatrix);

                    // Determine which textures to use (per-mesh or default)
                    Onyx::Texture* diffuseTexture = nullptr;
                    Onyx::Texture* normalTexture = nullptr;

                    if (meshMat) {
                        if (!meshMat->albedoPath.empty()) {
                            diffuseTexture = GetOrLoadTexture(meshMat->albedoPath);
                        }
                        if (!meshMat->normalPath.empty()) {
                            normalTexture = GetOrLoadTexture(meshMat->normalPath);
                        }
                    }

                    // Fall back to default textures if per-mesh not set
                    if (!diffuseTexture) diffuseTexture = defaultDiffuse;
                    if (!normalTexture) normalTexture = defaultNormal;

                    // Bind diffuse/albedo texture
                    if (diffuseTexture) {
                        diffuseTexture->Bind(0);
                    } else {
                        m_DefaultWhiteTexture->Bind(0);
                    }

                    // Bind normal map if available
                    if (normalTexture) {
                        normalTexture->Bind(1);
                        m_ModelShader->SetInt("u_UseNormalMap", 1);
                    } else {
                        m_ModelShader->SetInt("u_UseNormalMap", 0);
                    }

                    mesh.DrawGeometryOnly();
                    m_TriangleCount += static_cast<uint32_t>(mesh.m_Indices.size()) / 3;
                    m_DrawCalls++;
                }

                // Selection highlight - render wireframe on top
                if (obj->IsSelected()) {
                    Onyx::RenderCommand::SetWireframeMode(true);
                    Onyx::RenderCommand::SetLineWidth(2.0f);

                    int selectedMeshIdx = m_World->GetSelectedMeshIndex();
                    if (selectedMeshIdx >= 0 && selectedMeshIdx < static_cast<int>(model->GetMeshes().size())) {
                        // Only highlight the selected mesh with its per-mesh transform
                        auto& mesh = model->GetMeshes()[selectedMeshIdx];
                        std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(selectedMeshIdx)) : mesh.m_Name;
                        const MeshMaterial* meshMat = obj->GetMeshMaterial(meshName);

                        // Calculate mesh center for proper pivot scaling
                        glm::vec3 meshCenter(0.0f);
                        if (!mesh.m_Vertices.empty()) {
                            glm::vec3 minBounds(std::numeric_limits<float>::max());
                            glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
                            for (const auto& v : mesh.m_Vertices) {
                                minBounds = glm::min(minBounds, v.position);
                                maxBounds = glm::max(maxBounds, v.position);
                            }
                            meshCenter = (minBounds + maxBounds) * 0.5f;
                        }

                        glm::mat4 highlightMatrix = modelMatrix;
                        if (meshMat) {
                            highlightMatrix = glm::translate(highlightMatrix, meshMat->positionOffset);
                            highlightMatrix = glm::rotate(highlightMatrix, glm::radians(meshMat->rotationOffset.x), glm::vec3(1, 0, 0));
                            highlightMatrix = glm::rotate(highlightMatrix, glm::radians(meshMat->rotationOffset.y), glm::vec3(0, 1, 0));
                            highlightMatrix = glm::rotate(highlightMatrix, glm::radians(meshMat->rotationOffset.z), glm::vec3(0, 0, 1));
                            // Scale around mesh center (not origin)
                            highlightMatrix = glm::translate(highlightMatrix, meshCenter);
                            highlightMatrix = glm::scale(highlightMatrix, glm::vec3(meshMat->scaleMultiplier));
                            highlightMatrix = glm::translate(highlightMatrix, -meshCenter);
                        }
                        m_ModelShader->SetMat4("u_Model", highlightMatrix);
                        mesh.DrawGeometryOnly();
                        m_DrawCalls++;
                    } else {
                        // Highlight all meshes (no specific mesh selected) with their transforms
                        for (size_t idx = 0; idx < model->GetMeshes().size(); idx++) {
                            auto& mesh = model->GetMeshes()[idx];
                            std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(idx)) : mesh.m_Name;
                            const MeshMaterial* meshMat = obj->GetMeshMaterial(meshName);

                            // Calculate mesh center for proper pivot scaling
                            glm::vec3 meshCenter(0.0f);
                            if (!mesh.m_Vertices.empty()) {
                                glm::vec3 minBounds(std::numeric_limits<float>::max());
                                glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
                                for (const auto& v : mesh.m_Vertices) {
                                    minBounds = glm::min(minBounds, v.position);
                                    maxBounds = glm::max(maxBounds, v.position);
                                }
                                meshCenter = (minBounds + maxBounds) * 0.5f;
                            }

                            glm::mat4 highlightMatrix = modelMatrix;
                            if (meshMat) {
                                highlightMatrix = glm::translate(highlightMatrix, meshMat->positionOffset);
                                highlightMatrix = glm::rotate(highlightMatrix, glm::radians(meshMat->rotationOffset.x), glm::vec3(1, 0, 0));
                                highlightMatrix = glm::rotate(highlightMatrix, glm::radians(meshMat->rotationOffset.y), glm::vec3(0, 1, 0));
                                highlightMatrix = glm::rotate(highlightMatrix, glm::radians(meshMat->rotationOffset.z), glm::vec3(0, 0, 1));
                                // Scale around mesh center (not origin)
                                highlightMatrix = glm::translate(highlightMatrix, meshCenter);
                                highlightMatrix = glm::scale(highlightMatrix, glm::vec3(meshMat->scaleMultiplier));
                                highlightMatrix = glm::translate(highlightMatrix, -meshCenter);
                            }
                            m_ModelShader->SetMat4("u_Model", highlightMatrix);
                            mesh.DrawGeometryOnly();
                            m_DrawCalls++;
                        }
                    }
                    Onyx::RenderCommand::SetWireframeMode(false);
                }
                continue;
            }
        }

        // Fallback to placeholder cube
        m_ObjectShader->Bind();
        m_ObjectShader->SetMat4("u_View", m_ViewMatrix);
        m_ObjectShader->SetMat4("u_Projection", m_ProjectionMatrix);
        m_ObjectShader->SetMat4("u_Model", modelMatrix);

        if (obj->IsSelected()) {
            Onyx::RenderCommand::SetWireframeMode(true);
            Onyx::RenderCommand::SetLineWidth(2.0f);
            Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
            m_DrawCalls++;
            Onyx::RenderCommand::SetWireframeMode(false);
        }

        Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
        m_TriangleCount += 12;  // Cube has 12 triangles
        m_DrawCalls++;
    }

    // Render spawn points
    for (const auto& spawn : m_World->GetSpawnPoints()) {
        if (!spawn->IsVisible()) continue;

        // Use world matrix to account for parent hierarchy
        glm::mat4 modelMatrix = m_World->GetWorldMatrix(spawn.get());

        // Check if spawn point has a preview model
        const std::string& modelPath = spawn->GetModelPath();
        if (!modelPath.empty()) {
            Onyx::Model* model = GetOrLoadModel(modelPath);
            if (model) {
                glm::vec3 lightColor(1.0f);

                m_ModelShader->Bind();
                m_ModelShader->SetMat4("u_Model", modelMatrix);
                m_ModelShader->SetMat4("u_View", m_ViewMatrix);
                m_ModelShader->SetMat4("u_Projection", m_ProjectionMatrix);
                m_ModelShader->SetVec3("u_LightDir", m_LightDir);
                m_ModelShader->SetVec3("u_LightColor", lightColor);
                m_ModelShader->SetVec3("u_ViewPos", m_CameraPosition);
                m_ModelShader->SetFloat("u_AmbientStrength", m_AmbientStrength);
                m_ModelShader->SetInt("u_UseNormalMap", 0);

                // Check for custom textures from spawn point properties
                Onyx::Texture* diffuseTexture = GetOrLoadTexture(spawn->GetDiffuseTexture());
                Onyx::Texture* normalTexture = GetOrLoadTexture(spawn->GetNormalTexture());

                // Bind diffuse/albedo texture
                if (diffuseTexture) {
                    diffuseTexture->Bind(0);
                } else {
                    m_DefaultWhiteTexture->Bind(0);
                }
                m_ModelShader->SetInt("u_AlbedoMap", 0);

                // Bind normal map if available
                if (normalTexture) {
                    normalTexture->Bind(1);
                    m_ModelShader->SetInt("u_NormalMap", 1);
                    m_ModelShader->SetInt("u_UseNormalMap", 1);
                } else {
                    m_ModelShader->SetInt("u_UseNormalMap", 0);
                }

                // Draw each mesh
                for (auto& mesh : model->GetMeshes()) {
                    mesh.DrawGeometryOnly();
                    m_TriangleCount += static_cast<uint32_t>(mesh.m_Indices.size()) / 3;
                    m_DrawCalls++;
                }

                if (spawn->IsSelected()) {
                    Onyx::RenderCommand::SetWireframeMode(true);
                    Onyx::RenderCommand::SetLineWidth(2.0f);
                    for (auto& mesh : model->GetMeshes()) {
                        mesh.DrawGeometryOnly();
                        m_DrawCalls++;
                    }
                    Onyx::RenderCommand::SetWireframeMode(false);
                }
                continue;
            }
        }

        // Fallback to placeholder cube
        m_ObjectShader->Bind();
        m_ObjectShader->SetMat4("u_View", m_ViewMatrix);
        m_ObjectShader->SetMat4("u_Projection", m_ProjectionMatrix);
        glm::mat4 fallbackModel = glm::translate(glm::mat4(1.0f), spawn->GetPosition());
        fallbackModel = glm::scale(fallbackModel, glm::vec3(0.5f));
        m_ObjectShader->SetMat4("u_Model", fallbackModel);

        if (spawn->IsSelected()) {
            Onyx::RenderCommand::SetWireframeMode(true);
            Onyx::RenderCommand::SetLineWidth(2.0f);
            Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
            m_DrawCalls++;
            Onyx::RenderCommand::SetWireframeMode(false);
        }

        Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
        m_TriangleCount += 12;
        m_DrawCalls++;
    }

    // Ensure object shader is bound for remaining objects
    m_ObjectShader->Bind();
    m_ObjectShader->SetMat4("u_View", m_ViewMatrix);
    m_ObjectShader->SetMat4("u_Projection", m_ProjectionMatrix);

    // Render lights as small cubes
    for (const auto& light : m_World->GetLights()) {
        if (!light->IsVisible()) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), light->GetPosition());
        model = glm::scale(model, glm::vec3(0.3f));  // Even smaller
        m_ObjectShader->SetMat4("u_Model", model);

        if (light->IsSelected()) {
            Onyx::RenderCommand::SetWireframeMode(true);
            Onyx::RenderCommand::SetLineWidth(2.0f);
            Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
            m_DrawCalls++;
            Onyx::RenderCommand::SetWireframeMode(false);
        }

        Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
        m_TriangleCount += 12;
        m_DrawCalls++;
    }

    // Render particle emitters
    for (const auto& emitter : m_World->GetParticleEmitters()) {
        if (!emitter->IsVisible()) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), emitter->GetPosition());
        model = glm::scale(model, glm::vec3(0.4f));
        m_ObjectShader->SetMat4("u_Model", model);

        if (emitter->IsSelected()) {
            Onyx::RenderCommand::SetWireframeMode(true);
            Onyx::RenderCommand::SetLineWidth(2.0f);
            Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
            m_DrawCalls++;
            Onyx::RenderCommand::SetWireframeMode(false);
        }

        Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
        m_TriangleCount += 12;
        m_DrawCalls++;
    }

    // Render trigger volumes as wireframe cubes
    for (const auto& trigger : m_World->GetTriggerVolumes()) {
        if (!trigger->IsVisible()) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), trigger->GetPosition());
        model = glm::scale(model, trigger->GetHalfExtents() * 2.0f);
        m_ObjectShader->SetMat4("u_Model", model);

        Onyx::RenderCommand::SetWireframeMode(true);
        Onyx::RenderCommand::SetLineWidth(trigger->IsSelected() ? 2.0f : 1.0f);
        Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
        m_TriangleCount += 12;
        m_DrawCalls++;
        Onyx::RenderCommand::SetWireframeMode(false);
    }

    // Render instance portals
    for (const auto& portal : m_World->GetInstancePortals()) {
        if (!portal->IsVisible()) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), portal->GetPosition());
        model = glm::scale(model, glm::vec3(1.0f, 2.0f, 0.2f));  // Portal shape
        m_ObjectShader->SetMat4("u_Model", model);

        if (portal->IsSelected()) {
            Onyx::RenderCommand::SetWireframeMode(true);
            Onyx::RenderCommand::SetLineWidth(2.0f);
            Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
            m_DrawCalls++;
            Onyx::RenderCommand::SetWireframeMode(false);
        }

        Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
        m_TriangleCount += 12;
        m_DrawCalls++;
    }

    // Reset wireframe mode
    if (m_ShowWireframe) {
        Onyx::RenderCommand::SetWireframeMode(false);
    }
}

void ViewportPanel::RenderSelectionOutline() {
    // Already handled in RenderWorldObjects with wireframe
}

void ViewportPanel::RenderGizmoIcons() {
    // TODO: Render billboard icons for lights, spawns, etc.
}

void ViewportPanel::RenderGizmo() {
    if (!m_World || !m_Gizmo) return;

    WorldObject* selection = m_World->GetPrimarySelection();
    if (!selection) return;

    // Sync gizmo state from EditorWorld
    m_Gizmo->SetMode(m_World->GetGizmoMode());
    m_Gizmo->SetSnapEnabled(m_World->IsSnapEnabled());
    m_Gizmo->SetSnapValue(m_World->GetSnapValue());

    // Use world position for objects in groups
    glm::mat4 worldMatrix = m_World->GetWorldMatrix(selection);
    glm::vec3 gizmoPos = glm::vec3(worldMatrix[3]);

    // If a mesh is selected, position gizmo at mesh's geometric center
    int selectedMeshIdx = m_World->GetSelectedMeshIndex();
    if (selectedMeshIdx >= 0 && selection->GetObjectType() == WorldObjectType::STATIC_OBJECT) {
        StaticObject* staticObj = static_cast<StaticObject*>(selection);
        const std::string& modelPath = staticObj->GetModelPath();
        if (!modelPath.empty()) {
            Onyx::Model* model = GetOrLoadModel(modelPath);
            if (model && selectedMeshIdx < static_cast<int>(model->GetMeshes().size())) {
                auto& mesh = model->GetMeshes()[selectedMeshIdx];
                std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(selectedMeshIdx)) : mesh.m_Name;
                const MeshMaterial* meshMat = staticObj->GetMeshMaterial(meshName);

                // Calculate mesh bounding box center
                glm::vec3 meshCenter(0.0f);
                if (!mesh.m_Vertices.empty()) {
                    glm::vec3 minBounds(std::numeric_limits<float>::max());
                    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
                    for (const auto& v : mesh.m_Vertices) {
                        minBounds = glm::min(minBounds, v.position);
                        maxBounds = glm::max(maxBounds, v.position);
                    }
                    meshCenter = (minBounds + maxBounds) * 0.5f;
                }

                // Apply per-mesh transform offset
                glm::vec3 meshOffset = meshMat ? meshMat->positionOffset : glm::vec3(0.0f);
                glm::vec3 localPos = meshCenter + meshOffset;

                // Transform to world space (use world matrix for hierarchy support)
                glm::mat4 objMatrix = m_World->GetWorldMatrix(staticObj);
                gizmoPos = glm::vec3(objMatrix * glm::vec4(localPos, 1.0f));
            }
        }
    }

    float camDist = glm::length(m_CameraPosition - gizmoPos);

    m_Gizmo->Render(m_ViewMatrix, m_ProjectionMatrix, gizmoPos, camDist);
}

void ViewportPanel::HandleGizmoInteraction() {
    if (!m_World || !m_Gizmo || !m_ViewportHovered) return;

    WorldObject* selection = m_World->GetPrimarySelection();
    if (!selection) {
        m_Gizmo->SetActiveAxis(GizmoAxis::NONE);
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    // Get mouse position relative to viewport
    float mouseX = io.MousePos.x - m_ViewportPos.x;
    float mouseY = io.MousePos.y - m_ViewportPos.y;

    glm::vec3 rayDir = ScreenToWorldRay(mouseX, mouseY);
    glm::vec3 rayOrigin = m_CameraPosition;

    // Determine if we're working with a mesh or the whole object
    int selectedMeshIdx = m_World->GetSelectedMeshIndex();
    bool isMeshSelected = false;
    StaticObject* staticObj = nullptr;
    MeshMaterial* meshMaterial = nullptr;

    if (selectedMeshIdx >= 0 && selection->GetObjectType() == WorldObjectType::STATIC_OBJECT) {
        staticObj = static_cast<StaticObject*>(selection);
        const std::string& modelPath = staticObj->GetModelPath();
        if (!modelPath.empty()) {
            Onyx::Model* model = GetOrLoadModel(modelPath);
            if (model && selectedMeshIdx < static_cast<int>(model->GetMeshes().size())) {
                auto& mesh = model->GetMeshes()[selectedMeshIdx];
                m_GizmoSelectedMeshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(selectedMeshIdx)) : mesh.m_Name;
                meshMaterial = &staticObj->GetOrCreateMeshMaterial(m_GizmoSelectedMeshName);
                isMeshSelected = true;
            }
        }
    }

    // Calculate gizmo position (at mesh geometric center + offset)
    // Use world position for objects in groups
    glm::mat4 selectionWorldMatrix = m_World->GetWorldMatrix(selection);
    glm::vec3 gizmoPos = glm::vec3(selectionWorldMatrix[3]);
    glm::vec3 meshCenter(0.0f);
    if (isMeshSelected && staticObj) {
        const std::string& modelPath = staticObj->GetModelPath();
        Onyx::Model* model = GetOrLoadModel(modelPath);
        if (model && selectedMeshIdx < static_cast<int>(model->GetMeshes().size())) {
            auto& mesh = model->GetMeshes()[selectedMeshIdx];
            // Calculate mesh bounding box center
            if (!mesh.m_Vertices.empty()) {
                glm::vec3 minBounds(std::numeric_limits<float>::max());
                glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
                for (const auto& v : mesh.m_Vertices) {
                    minBounds = glm::min(minBounds, v.position);
                    maxBounds = glm::max(maxBounds, v.position);
                }
                meshCenter = (minBounds + maxBounds) * 0.5f;
            }
        }

        glm::vec3 meshOffset = meshMaterial ? meshMaterial->positionOffset : glm::vec3(0.0f);
        glm::vec3 localPos = meshCenter + meshOffset;
        glm::mat4 objMatrix = m_World->GetWorldMatrix(staticObj);
        gizmoPos = glm::vec3(objMatrix * glm::vec4(localPos, 1.0f));
    }

    float camDist = glm::length(m_CameraPosition - gizmoPos);

    // Check for gizmo hover/hit (when not dragging)
    if (!m_Gizmo->IsDragging()) {
        GizmoAxis hoverAxis = m_Gizmo->TestHit(rayOrigin, rayDir, gizmoPos, camDist);
        m_Gizmo->SetActiveAxis(hoverAxis);
    }

    // Start dragging on click if we hit an axis
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_Gizmo->GetActiveAxis() != GizmoAxis::NONE) {
        m_Gizmo->BeginDrag(gizmoPos, rayOrigin, rayDir, m_ViewMatrix);

        if (isMeshSelected && meshMaterial) {
            // Store mesh transform state
            m_GizmoStartMeshOffset = meshMaterial->positionOffset;
            m_GizmoStartMeshRotation = meshMaterial->rotationOffset;
            m_GizmoStartMeshScale = meshMaterial->scaleMultiplier;
        } else {
            // Store object transform state
            m_GizmoStartObjectPos = selection->GetPosition();
            m_GizmoStartObjectRot = selection->GetRotation();
            m_GizmoStartObjectScale = selection->GetScale();
            std::cout << "[Gizmo] Drag START - captured pos=("
                      << m_GizmoStartObjectPos.x << "," << m_GizmoStartObjectPos.y << "," << m_GizmoStartObjectPos.z
                      << ") scale=" << m_GizmoStartObjectScale << std::endl;
        }
    }

    // Continue dragging
    if (m_Gizmo->IsDragging() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        GizmoMode mode = m_Gizmo->GetMode();
        GizmoAxis axis = m_Gizmo->GetActiveAxis();

        if (isMeshSelected && meshMaterial && staticObj) {
            // Apply transforms to mesh offset
            if (mode == GizmoMode::TRANSLATE) {
                glm::vec3 translation = m_Gizmo->CalculateTranslation(
                    rayOrigin, rayDir, gizmoPos, axis,
                    m_ViewMatrix, m_ProjectionMatrix
                );

                // Convert world translation to local object space
                // Use inverse rotation (not transpose of scaled matrix)
                glm::quat invRot = glm::inverse(staticObj->GetRotation());
                glm::vec3 localTranslation = invRot * translation / staticObj->GetScale();

                meshMaterial->positionOffset = m_GizmoStartMeshOffset + localTranslation;
            }
            else if (mode == GizmoMode::ROTATE) {
                float deltaAngle = m_Gizmo->CalculateRotation(
                    rayOrigin, rayDir, gizmoPos, axis, m_ViewMatrix
                );

                // Apply rotation as euler angle offset
                float deltaDegrees = glm::degrees(deltaAngle);
                glm::vec3 newRotation = m_GizmoStartMeshRotation;
                if (axis == GizmoAxis::X) newRotation.x += deltaDegrees;
                else if (axis == GizmoAxis::Y) newRotation.y += deltaDegrees;
                else if (axis == GizmoAxis::Z) newRotation.z += deltaDegrees;

                meshMaterial->rotationOffset = newRotation;
            }
            else if (mode == GizmoMode::SCALE) {
                glm::vec3 scaleFactor = m_Gizmo->CalculateScale(
                    rayOrigin, rayDir, gizmoPos, axis, m_ViewMatrix
                );

                // For uniform scale
                float factor = 1.0f;
                if (axis == GizmoAxis::X) factor = scaleFactor.x;
                else if (axis == GizmoAxis::Y) factor = scaleFactor.y;
                else if (axis == GizmoAxis::Z) factor = scaleFactor.z;
                else factor = scaleFactor.x;

                meshMaterial->scaleMultiplier = m_GizmoStartMeshScale * factor;
            }
        } else {
            // Apply transforms to object
            if (mode == GizmoMode::TRANSLATE) {
                glm::vec3 translation = m_Gizmo->CalculateTranslation(
                    rayOrigin, rayDir, gizmoPos, axis,
                    m_ViewMatrix, m_ProjectionMatrix
                );

                // Apply translation to all selected objects
                for (WorldObject* obj : m_World->GetSelectedObjects()) {
                    obj->SetPosition(m_GizmoStartObjectPos + translation);
                }
            }
            else if (mode == GizmoMode::ROTATE) {
                float deltaAngle = m_Gizmo->CalculateRotation(
                    rayOrigin, rayDir, gizmoPos, axis, m_ViewMatrix
                );

                // Determine rotation axis
                glm::vec3 rotAxis;
                if (axis == GizmoAxis::X) rotAxis = glm::vec3(1, 0, 0);
                else if (axis == GizmoAxis::Y) rotAxis = glm::vec3(0, 1, 0);
                else rotAxis = glm::vec3(0, 0, 1);

                // Apply rotation
                glm::quat deltaRot = glm::angleAxis(deltaAngle, rotAxis);
                for (WorldObject* obj : m_World->GetSelectedObjects()) {
                    obj->SetRotation(deltaRot * m_GizmoStartObjectRot);
                }
            }
            else if (mode == GizmoMode::SCALE) {
                glm::vec3 scaleFactor = m_Gizmo->CalculateScale(
                    rayOrigin, rayDir, gizmoPos, axis, m_ViewMatrix
                );

                // For uniform scale, use the component matching the active axis
                float newScale = m_GizmoStartObjectScale;
                if (axis == GizmoAxis::X) newScale *= scaleFactor.x;
                else if (axis == GizmoAxis::Y) newScale *= scaleFactor.y;
                else if (axis == GizmoAxis::Z) newScale *= scaleFactor.z;
                else newScale *= scaleFactor.x;  // XYZ uniform

                for (WorldObject* obj : m_World->GetSelectedObjects()) {
                    obj->SetScale(newScale);
                }
            }
        }
    }

    // End dragging - record undo command
    bool mouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    bool gizmoDragging = m_Gizmo->IsDragging();
    if (mouseReleased && gizmoDragging) {
        std::cout << "[Gizmo] Drag END triggered (mouseReleased=" << mouseReleased << " gizmoDragging=" << gizmoDragging << ")" << std::endl;
        if (!isMeshSelected) {
            // Create undo command for object transform
            glm::vec3 newPos = selection->GetPosition();
            glm::quat newRot = selection->GetRotation();
            float newScale = selection->GetScale();

            std::cout << "[Gizmo] Drag END - startPos=("
                      << m_GizmoStartObjectPos.x << "," << m_GizmoStartObjectPos.y << "," << m_GizmoStartObjectPos.z
                      << ") currentPos=(" << newPos.x << "," << newPos.y << "," << newPos.z
                      << ") startScale=" << m_GizmoStartObjectScale << " currentScale=" << newScale << std::endl;

            // Only record if something actually changed
            bool posChanged = newPos != m_GizmoStartObjectPos;
            bool rotChanged = newRot != m_GizmoStartObjectRot;
            bool scaleChanged = newScale != m_GizmoStartObjectScale;
            std::cout << "[Gizmo] Changes: pos=" << posChanged << " rot=" << rotChanged << " scale=" << scaleChanged << std::endl;
            if (posChanged || rotChanged || scaleChanged) {
                auto cmd = std::make_unique<TransformCommand>(
                    selection,
                    m_GizmoStartObjectPos, m_GizmoStartObjectRot, m_GizmoStartObjectScale,
                    newPos, newRot, newScale
                );
                // Use AddWithoutExecute since we already applied the transform during drag
                CommandHistory::Get().AddWithoutExecute(std::move(cmd));
            }
        } else if (isMeshSelected && meshMaterial && staticObj) {
            // Create undo command for mesh transform
            glm::vec3 newOffset = meshMaterial->positionOffset;
            glm::vec3 newRotation = meshMaterial->rotationOffset;
            float newScale = meshMaterial->scaleMultiplier;

            // Only record if something actually changed
            bool offsetChanged = newOffset != m_GizmoStartMeshOffset;
            bool rotChanged = newRotation != m_GizmoStartMeshRotation;
            bool scaleChanged = newScale != m_GizmoStartMeshScale;

            if (offsetChanged || rotChanged || scaleChanged) {
                auto cmd = std::make_unique<MeshTransformCommand>(
                    staticObj, m_GizmoSelectedMeshName,
                    m_GizmoStartMeshOffset, m_GizmoStartMeshRotation, m_GizmoStartMeshScale,
                    newOffset, newRotation, newScale
                );
                CommandHistory::Get().AddWithoutExecute(std::move(cmd));
            }
        }

        m_Gizmo->EndDrag();
    }

    // Keyboard shortcuts for gizmo mode (only when not using camera)
    if (!io.WantTextInput && !m_RightMouseDown) {
        if (ImGui::IsKeyPressed(ImGuiKey_1)) {
            m_World->SetGizmoMode(GizmoMode::TRANSLATE);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_2)) {
            m_World->SetGizmoMode(GizmoMode::ROTATE);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_3)) {
            m_World->SetGizmoMode(GizmoMode::SCALE);
        }
        // Toggle grid snap with G (only when Ctrl not held - Ctrl+G is for grouping)
        if (ImGui::IsKeyPressed(ImGuiKey_G) && !io.KeyCtrl) {
            m_World->SetSnapEnabled(!m_World->IsSnapEnabled());
        }

        // Copy/Paste/Duplicate shortcuts
        // Ctrl+C = copy object (if mesh selected, copy object with only that mesh visible)
        // Ctrl+V = paste object
        // Ctrl+D = duplicate (if mesh selected, duplicate with only that mesh visible)
        // Ctrl+Shift+C/V = copy/paste mesh transforms (advanced)
        bool ctrlHeld = io.KeyCtrl;
        bool shiftHeld = io.KeyShift;
        if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_C)) {
            if (shiftHeld && m_World->HasMeshSelected()) {
                // Ctrl+Shift+C: Copy mesh transform only
                std::cout << "[Copy] Copying mesh transform" << std::endl;
                m_World->CopyMesh();
            } else if (m_World->HasMeshSelected()) {
                // Ctrl+C with mesh selected: Copy object with only this mesh visible
                std::cout << "[Copy] Copying single mesh as object" << std::endl;
                m_World->CopySingleMesh();
            } else {
                // Ctrl+C: Copy whole object
                std::cout << "[Copy] Copying object" << std::endl;
                m_World->Copy();
            }
        }
        if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_V)) {
            if (shiftHeld && m_World->HasMeshClipboard() && m_World->HasMeshSelected()) {
                std::cout << "[Paste] Pasting mesh transform" << std::endl;
                m_World->PasteMesh();
            } else {
                std::cout << "[Paste] Pasting object" << std::endl;
                m_World->Paste();
            }
        }
        if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_D)) {
            if (m_World->HasMeshSelected()) {
                // Ctrl+D with mesh selected: Duplicate with only this mesh visible
                std::cout << "[Duplicate] Duplicating single mesh as object" << std::endl;
                m_World->CopySingleMesh();
                m_World->Paste();
            } else {
                // Ctrl+D: Duplicate whole object
                m_World->Duplicate();
            }
        }

        // Delete selected objects
        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            m_World->DeleteSelected();
        }

        // Grouping shortcuts
        // Ctrl+G = Group selected objects
        // Ctrl+Shift+G = Ungroup selected groups
        if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_G)) {
            if (shiftHeld) {
                std::cout << "[Ungroup] Ungrouping selected groups" << std::endl;
                m_World->UngroupSelected();
            } else {
                std::cout << "[Group] Grouping selected objects" << std::endl;
                m_World->GroupSelected();
            }
        }
    }
}

void ViewportPanel::HandleCameraInput() {
    if (!m_ViewportHovered) return;

    ImGuiIO& io = ImGui::GetIO();
    float deltaTime = io.DeltaTime > 0.0f ? io.DeltaTime : 0.016f;

    // Right mouse button - look around + WASD movement
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if (!m_RightMouseDown) {
            m_RightMouseDown = true;
            m_FirstMouse = true;
        }

        // Mouse look
        if (!m_FirstMouse) {
            float xOffset = io.MouseDelta.x * m_CameraSensitivity;
            float yOffset = -io.MouseDelta.y * m_CameraSensitivity;

            m_CameraYaw += xOffset;
            m_CameraPitch += yOffset;
            m_CameraPitch = glm::clamp(m_CameraPitch, -89.0f, 89.0f);

            UpdateCameraVectors();
        }
        m_FirstMouse = false;

        // WASD movement while right mouse is held
        float speed = m_CameraSpeed * deltaTime;

        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            m_CameraPosition += speed * m_CameraFront;
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            m_CameraPosition -= speed * m_CameraFront;
        }
        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            m_CameraPosition -= speed * m_CameraRight;
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            m_CameraPosition += speed * m_CameraRight;
        }
        if (ImGui::IsKeyDown(ImGuiKey_Q)) {
            m_CameraPosition -= speed * m_CameraUp;
        }
        if (ImGui::IsKeyDown(ImGuiKey_E)) {
            m_CameraPosition += speed * m_CameraUp;
        }
    } else {
        m_RightMouseDown = false;
    }

    // Middle mouse button - pan
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        float panSpeed = 0.02f;
        m_CameraPosition -= m_CameraRight * io.MouseDelta.x * panSpeed;
        m_CameraPosition += m_CameraUp * io.MouseDelta.y * panSpeed;
    }

    // Scroll wheel - dolly forward/back
    if (io.MouseWheel != 0.0f) {
        m_CameraPosition += m_CameraFront * io.MouseWheel * m_CameraSpeed * 0.1f;
    }

    // F key - focus on selection
    if (ImGui::IsKeyPressed(ImGuiKey_F)) {
        FocusOnSelection();
    }
}

void ViewportPanel::HandleObjectPicking() {
    if (!m_World || !m_ViewportHovered) return;

    // Don't pick objects if we're interacting with the gizmo
    if (m_Gizmo && m_Gizmo->GetActiveAxis() != GizmoAxis::NONE) return;

    // Left click to select
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        ImGuiIO& io = ImGui::GetIO();

        // Get mouse position relative to viewport
        float mouseX = io.MousePos.x - m_ViewportPos.x;
        float mouseY = io.MousePos.y - m_ViewportPos.y;

        std::cout << "[Pick] Mouse: (" << io.MousePos.x << "," << io.MousePos.y << ") "
                  << "ViewportPos: (" << m_ViewportPos.x << "," << m_ViewportPos.y << ") "
                  << "Relative: (" << mouseX << "," << mouseY << ") "
                  << "ViewportSize: (" << m_ViewportWidth << "x" << m_ViewportHeight << ")" << std::endl;

        // Render picking pass and read result
        RenderPickingPass();
        PickResult pick = ReadPickingBuffer(mouseX, mouseY);

        // Select (with shift for multi-select)
        bool addToSelection = io.KeyShift;
        if (pick.hit) {
            m_World->SelectByGuid(pick.objectGuid, addToSelection);

            // Get mesh name and set it for clipboard operations
            std::string selectedMeshName;
            const auto& selected = m_World->GetSelectedObjects();
            if (!selected.empty()) {
                if (auto* staticObj = dynamic_cast<const StaticObject*>(selected[0])) {
                    Onyx::Model* model = GetOrLoadModel(staticObj->GetModelPath());
                    if (model && pick.meshIndex >= 0 && pick.meshIndex < static_cast<int>(model->GetMeshes().size())) {
                        auto& mesh = model->GetMeshes()[pick.meshIndex];
                        selectedMeshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(pick.meshIndex)) : mesh.m_Name;
                        std::cout << "[Pick] Model has " << model->GetMeshes().size()
                                  << " meshes. Index " << pick.meshIndex
                                  << " = \"" << selectedMeshName << "\"" << std::endl;
                    }
                }
            }

            m_World->SelectMesh(pick.meshIndex);
            m_World->SetSelectedMeshName(selectedMeshName);
        } else if (!addToSelection) {
            m_World->DeselectAll();
        }
    }
}

glm::vec3 ViewportPanel::ScreenToWorldRay(float screenX, float screenY) {
    // Convert screen coordinates to normalized device coordinates
    float x = (2.0f * screenX) / m_ViewportWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / m_ViewportHeight;

    // Create ray in clip space
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    // Convert to eye space
    glm::vec4 rayEye = glm::inverse(m_ProjectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // Convert to world space
    glm::vec3 rayWorld = glm::vec3(glm::inverse(m_ViewMatrix) * rayEye);
    return glm::normalize(rayWorld);
}

// ===== GPU PICKING IMPLEMENTATION =====

void ViewportPanel::RenderPickingPass() {
    if (!m_World) return;

    m_PickingFramebuffer->Bind();
    m_PickingFramebuffer->Clear(0.0f, 0.0f, 0.0f, 0.0f);  // Black = no object

    // Explicitly set all relevant GL state to ensure consistency with main render
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);  // Render all faces for picking
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // Ensure solid fill, not wireframe

    // ===== Render solid geometry (StaticObjects with models) =====
    m_PickingShader->Bind();
    m_PickingShader->SetMat4("u_View", m_ViewMatrix);
    m_PickingShader->SetMat4("u_Projection", m_ProjectionMatrix);

    // StaticObjects - render each mesh with unique ID
    for (const auto& obj : m_World->GetStaticObjects()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        RenderObjectForPicking(obj.get());
    }

    // SpawnPoints with models
    for (const auto& obj : m_World->GetSpawnPoints()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        const std::string& modelPath = obj->GetModelPath();
        if (!modelPath.empty()) {
            Onyx::Model* model = GetOrLoadModel(modelPath);
            if (model) {
                uint32_t objID = static_cast<uint32_t>(obj->GetGuid() & 0xFFFF);
                glm::mat4 modelMatrix = m_World->GetWorldMatrix(obj.get());

                for (size_t meshIdx = 0; meshIdx < model->GetMeshes().size(); meshIdx++) {
                    m_PickingShader->SetMat4("u_Model", modelMatrix);
                    m_PickingShader->SetInt("u_ObjectID", static_cast<int>(objID));
                    m_PickingShader->SetInt("u_MeshIndex", static_cast<int>(meshIdx));
                    m_PickingShader->SetInt("u_ObjectType", static_cast<int>(WorldObjectType::SPAWN_POINT));
                    model->GetMeshes()[meshIdx].DrawGeometryOnly();
                }
                continue;
            }
        }
        // Fallback to billboard for spawn points without models
        RenderIconForPicking(obj.get(), WorldObjectType::SPAWN_POINT, 1.0f);
    }

    // TriggerVolumes - render as solid cubes
    for (const auto& obj : m_World->GetTriggerVolumes()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        uint32_t objID = static_cast<uint32_t>(obj->GetGuid() & 0xFFFF);

        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), obj->GetPosition());
        modelMatrix = glm::scale(modelMatrix, obj->GetHalfExtents() * 2.0f);

        m_PickingShader->SetMat4("u_Model", modelMatrix);
        m_PickingShader->SetInt("u_ObjectID", static_cast<int>(objID));
        m_PickingShader->SetInt("u_MeshIndex", 0xFFF);  // 4095 = no mesh
        m_PickingShader->SetInt("u_ObjectType", static_cast<int>(WorldObjectType::TRIGGER_VOLUME));
        Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
    }

    // ===== Render billboard icons =====
    m_PickingBillboardShader->Bind();
    m_PickingBillboardShader->SetMat4("u_View", m_ViewMatrix);
    m_PickingBillboardShader->SetMat4("u_Projection", m_ProjectionMatrix);
    m_PickingBillboardShader->SetInt("u_IconTexture", 0);
    m_PickingBillboardShader->SetInt("u_UseTexture", 0);  // Simple solid color icons

    // Lights
    for (const auto& obj : m_World->GetLights()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        RenderIconForPicking(obj.get(), WorldObjectType::LIGHT, 0.8f);
    }

    // Particle Emitters
    for (const auto& obj : m_World->GetParticleEmitters()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        RenderIconForPicking(obj.get(), WorldObjectType::PARTICLE_EMITTER, 0.6f);
    }

    // Instance Portals
    for (const auto& obj : m_World->GetInstancePortals()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        RenderIconForPicking(obj.get(), WorldObjectType::INSTANCE_PORTAL, 1.2f);
    }

    m_PickingFramebuffer->UnBind();
}

void ViewportPanel::RenderObjectForPicking(StaticObject* obj) {
    const std::string& modelPath = obj->GetModelPath();
    uint32_t objID = static_cast<uint32_t>(obj->GetGuid() & 0xFFFF);

    if (!modelPath.empty()) {
        Onyx::Model* model = GetOrLoadModel(modelPath);
        if (model) {
            glm::mat4 baseMatrix = m_World->GetWorldMatrix(obj);

            // Debug: print first few mesh indices being rendered
            static int s_DebugCount = 0;
            if (s_DebugCount < 3) {
                std::cout << "[PickRender] Rendering " << model->GetMeshes().size() << " meshes for object " << objID << std::endl;
                s_DebugCount++;
            }

            for (size_t meshIdx = 0; meshIdx < model->GetMeshes().size(); meshIdx++) {
                auto& mesh = model->GetMeshes()[meshIdx];
                std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(meshIdx)) : mesh.m_Name;
                const MeshMaterial* meshMat = obj->GetMeshMaterial(meshName);

                // Skip hidden meshes
                if (meshMat && !meshMat->visible) {
                    continue;
                }

                // Calculate mesh center for proper pivot scaling
                glm::vec3 meshCenter(0.0f);
                if (!mesh.m_Vertices.empty()) {
                    glm::vec3 minBounds(std::numeric_limits<float>::max());
                    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
                    for (const auto& v : mesh.m_Vertices) {
                        minBounds = glm::min(minBounds, v.position);
                        maxBounds = glm::max(maxBounds, v.position);
                    }
                    meshCenter = (minBounds + maxBounds) * 0.5f;
                }

                // Apply per-mesh transform
                glm::mat4 meshMatrix = baseMatrix;
                if (meshMat) {
                    meshMatrix = glm::translate(meshMatrix, meshMat->positionOffset);
                    meshMatrix = glm::rotate(meshMatrix, glm::radians(meshMat->rotationOffset.x), glm::vec3(1, 0, 0));
                    meshMatrix = glm::rotate(meshMatrix, glm::radians(meshMat->rotationOffset.y), glm::vec3(0, 1, 0));
                    meshMatrix = glm::rotate(meshMatrix, glm::radians(meshMat->rotationOffset.z), glm::vec3(0, 0, 1));
                    // Scale around mesh center (not origin)
                    meshMatrix = glm::translate(meshMatrix, meshCenter);
                    meshMatrix = glm::scale(meshMatrix, glm::vec3(meshMat->scaleMultiplier));
                    meshMatrix = glm::translate(meshMatrix, -meshCenter);
                }

                m_PickingShader->SetMat4("u_Model", meshMatrix);
                m_PickingShader->SetInt("u_ObjectID", static_cast<int>(objID));
                m_PickingShader->SetInt("u_MeshIndex", static_cast<int>(meshIdx));
                m_PickingShader->SetInt("u_ObjectType", static_cast<int>(WorldObjectType::STATIC_OBJECT));
                mesh.DrawGeometryOnly();
            }
            return;
        }
    }

    // Fallback to placeholder cube
    glm::mat4 modelMatrix = m_World->GetWorldMatrix(obj);
    m_PickingShader->SetMat4("u_Model", modelMatrix);
    m_PickingShader->SetInt("u_ObjectID", static_cast<int>(objID));
    m_PickingShader->SetInt("u_MeshIndex", 0xFFF);  // 4095 = no mesh
    m_PickingShader->SetInt("u_ObjectType", static_cast<int>(WorldObjectType::STATIC_OBJECT));
    Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
}

void ViewportPanel::RenderIconForPicking(WorldObject* obj, WorldObjectType type, float size) {
    uint32_t objID = static_cast<uint32_t>(obj->GetGuid() & 0xFFFF);

    // Use world position for hierarchy support
    glm::mat4 worldMatrix = m_World->GetWorldMatrix(obj);
    glm::vec3 pos = glm::vec3(worldMatrix[3]);
    m_PickingBillboardShader->SetVec3("u_Position", pos.x, pos.y, pos.z);
    m_PickingBillboardShader->SetFloat("u_Size", size);
    m_PickingBillboardShader->SetInt("u_ObjectID", static_cast<int>(objID));
    m_PickingBillboardShader->SetInt("u_ObjectType", static_cast<int>(type));

    Onyx::RenderCommand::DrawIndexed(*m_BillboardVAO, 6);
}

ViewportPanel::PickResult ViewportPanel::ReadPickingBuffer(float mouseX, float mouseY) {
    PickResult result;

    // Convert viewport coordinates to picking buffer coordinates
    int pickWidth = m_PickingFramebuffer->GetWidth();
    int pickHeight = m_PickingFramebuffer->GetHeight();

    int x = static_cast<int>((mouseX / m_ViewportWidth) * pickWidth);
    int y = static_cast<int>(((m_ViewportHeight - mouseY) / m_ViewportHeight) * pickHeight);

    // Clamp to valid range
    x = std::clamp(x, 0, pickWidth - 1);
    y = std::clamp(y, 0, pickHeight - 1);

    // Ensure GPU has finished rendering before reading
    glFinish();

    // Read pixel from picking buffer
    m_PickingFramebuffer->Bind();
    unsigned char pixel[4];
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    m_PickingFramebuffer->UnBind();

    // Check if we hit anything (black = nothing)
    if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0 && pixel[3] == 0) {
        return result;
    }

    // Decode the pick result
    // R + G = Object ID (16 bits)
    // B = Mesh Index low 8 bits
    // A upper 4 bits = Mesh Index high 4 bits
    // A lower 4 bits = Object Type
    result.hit = true;
    result.objectGuid = (static_cast<uint64_t>(pixel[0]) << 8) | static_cast<uint64_t>(pixel[1]);

    int meshLow = pixel[2];
    int meshHigh = (pixel[3] >> 4) & 0x0F;
    int meshIndex12 = (meshHigh << 8) | meshLow;  // 12-bit mesh index
    result.meshIndex = (meshIndex12 == 0xFFF) ? -1 : meshIndex12;  // 4095 = no mesh

    result.type = static_cast<WorldObjectType>(pixel[3] & 0x0F);

    // Debug output
    std::cout << "[Pick] Pixel at (" << x << "," << y << "): RGBA=("
              << (int)pixel[0] << "," << (int)pixel[1] << ","
              << (int)pixel[2] << "," << (int)pixel[3] << ") -> "
              << "ObjID=" << result.objectGuid << " MeshIdx=" << result.meshIndex
              << " Type=" << static_cast<int>(result.type) << std::endl;

    return result;
}

void ViewportPanel::FocusOnObject(const WorldObject* object) {
    if (!object) return;

    glm::vec3 targetPos = object->GetPosition();
    float distance = 10.0f;  // Distance from object

    // Move camera to look at object
    m_CameraPosition = targetPos - m_CameraFront * distance;
}

void ViewportPanel::FocusOnSelection() {
    if (!m_World || !m_World->HasSelection()) return;

    const auto& selected = m_World->GetSelectedObjects();

    if (selected.size() == 1) {
        FocusOnObject(selected[0]);
    } else {
        // Focus on center of all selected objects
        glm::vec3 center(0.0f);
        for (const auto* obj : selected) {
            center += obj->GetPosition();
        }
        center /= static_cast<float>(selected.size());

        float distance = 15.0f;
        m_CameraPosition = center - m_CameraFront * distance;
    }
}

void ViewportPanel::UpdateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));
    front.y = sin(glm::radians(m_CameraPitch));
    front.z = sin(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));
    m_CameraFront = glm::normalize(front);
    m_CameraRight = glm::normalize(glm::cross(m_CameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_CameraUp = glm::normalize(glm::cross(m_CameraRight, m_CameraFront));
}

Onyx::Model* ViewportPanel::GetModel(const std::string& path) {
    return GetOrLoadModel(path);
}

Onyx::Model* ViewportPanel::GetOrLoadModel(const std::string& path) {
    if (path.empty()) return nullptr;

    // Check if model is already cached
    auto it = m_ModelCache.find(path);
    if (it != m_ModelCache.end()) {
        return it->second.get();
    }

    // Try to load the model (geometry only - textures assigned via Inspector)
    try {
        auto model = std::make_unique<Onyx::Model>(path.c_str(), false);
        Onyx::Model* modelPtr = model.get();
        m_ModelCache[path] = std::move(model);
        return modelPtr;
    } catch (...) {
        // Model failed to load - cache nullptr to avoid repeated attempts
        std::cerr << "Failed to load model: " << path << std::endl;
        m_ModelCache[path] = nullptr;
        return nullptr;
    }
}

void ViewportPanel::CreateDefaultTextures() {
    // Create a 1x1 white texture for models without textures
    m_DefaultWhiteTexture = Onyx::Texture::CreateSolidColor(255, 255, 255);
}

Onyx::Texture* ViewportPanel::GetOrLoadTexture(const std::string& path) {
    if (path.empty()) return nullptr;

    // Check cache
    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end()) {
        return it->second.get();
    }

    // Try to load the texture using Onyx::Texture
    try {
        auto texture = std::make_unique<Onyx::Texture>(path.c_str());
        Onyx::Texture* texturePtr = texture.get();
        m_TextureCache[path] = std::move(texture);
        return texturePtr;
    } catch (...) {
        // Failed to load - cache nullptr to avoid repeated attempts
        m_TextureCache[path] = nullptr;
        return nullptr;
    }
}

} // namespace MMO
