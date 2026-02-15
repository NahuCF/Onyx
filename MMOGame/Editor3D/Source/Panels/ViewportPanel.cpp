#include "ViewportPanel.h"
#include "World/EditorWorld.h"
#include "Commands/EditorCommand.h"
#include <Graphics/RenderCommand.h>
#include <Graphics/VertexLayout.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <limits>
#include <iostream>

namespace MMO {

ViewportPanel::ViewportPanel() {
    m_Name = "Viewport";
}

void ViewportPanel::OnInit() {
    m_Framebuffer = std::make_unique<Onyx::Framebuffer>();
    m_Framebuffer->Create(800, 600, 4);

    m_ObjectShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/basic3d.vert",
        "MMOGame/Editor3D/assets/shaders/basic3d.frag"
    );

    m_ModelShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/model.vert",
        "MMOGame/Editor3D/assets/shaders/model.frag"
    );

    float cubeVertices[] = {
        -0.5f, -0.5f,  0.5f,    0.8f, 0.8f, 0.8f,
         0.5f, -0.5f,  0.5f,    0.8f, 0.8f, 0.8f,
         0.5f,  0.5f,  0.5f,    0.9f, 0.9f, 0.9f,
        -0.5f,  0.5f,  0.5f,    0.9f, 0.9f, 0.9f,
        -0.5f, -0.5f, -0.5f,    0.7f, 0.7f, 0.7f,
         0.5f, -0.5f, -0.5f,    0.7f, 0.7f, 0.7f,
         0.5f,  0.5f, -0.5f,    0.8f, 0.8f, 0.8f,
        -0.5f,  0.5f, -0.5f,    0.8f, 0.8f, 0.8f,
        -0.5f,  0.5f, -0.5f,    0.85f, 0.85f, 0.85f,
         0.5f,  0.5f, -0.5f,    0.85f, 0.85f, 0.85f,
         0.5f,  0.5f,  0.5f,    0.95f, 0.95f, 0.95f,
        -0.5f,  0.5f,  0.5f,    0.95f, 0.95f, 0.95f,
        -0.5f, -0.5f, -0.5f,    0.6f, 0.6f, 0.6f,
         0.5f, -0.5f, -0.5f,    0.6f, 0.6f, 0.6f,
         0.5f, -0.5f,  0.5f,    0.7f, 0.7f, 0.7f,
        -0.5f, -0.5f,  0.5f,    0.7f, 0.7f, 0.7f,
         0.5f, -0.5f, -0.5f,    0.75f, 0.75f, 0.75f,
         0.5f,  0.5f, -0.5f,    0.75f, 0.75f, 0.75f,
         0.5f,  0.5f,  0.5f,    0.85f, 0.85f, 0.85f,
         0.5f, -0.5f,  0.5f,    0.85f, 0.85f, 0.85f,
        -0.5f, -0.5f, -0.5f,    0.65f, 0.65f, 0.65f,
        -0.5f,  0.5f, -0.5f,    0.65f, 0.65f, 0.65f,
        -0.5f,  0.5f,  0.5f,    0.75f, 0.75f, 0.75f,
        -0.5f, -0.5f,  0.5f,    0.75f, 0.75f, 0.75f,
    };

    uint32_t cubeIndices[] = {
        0, 1, 2,  0, 2, 3,
        4, 6, 5,  4, 7, 6,
        8, 9, 10,  8, 10, 11,
        12, 14, 13,  12, 15, 14,
        16, 17, 18,  16, 18, 19,
        20, 22, 21,  20, 23, 22,
    };

    m_CubeVBO = std::make_unique<Onyx::VertexBuffer>(static_cast<const void*>(cubeVertices), sizeof(cubeVertices));
    m_CubeEBO = std::make_unique<Onyx::IndexBuffer>(static_cast<const void*>(cubeIndices), sizeof(cubeIndices));
    m_CubeVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout cubeLayout;
    cubeLayout.PushFloat(3);
    cubeLayout.PushFloat(3);

    m_CubeVAO->SetVertexBuffer(m_CubeVBO.get());
    m_CubeVAO->SetIndexBuffer(m_CubeEBO.get());
    m_CubeVAO->SetLayout(cubeLayout);

    float planeVertices[] = {
        -0.5f, 0.0f, -0.5f,    0.0f, 1.0f, 0.0f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, 0.0f, -0.5f,    0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, 0.0f,  0.5f,    0.0f, 1.0f, 0.0f,  1.0f, 1.0f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,    0.0f, 1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
    };

    uint32_t planeIndices[] = {
        0, 2, 1,
        0, 3, 2
    };

    m_PlaneVBO = std::make_unique<Onyx::VertexBuffer>(static_cast<const void*>(planeVertices), sizeof(planeVertices));
    m_PlaneEBO = std::make_unique<Onyx::IndexBuffer>(static_cast<const void*>(planeIndices), sizeof(planeIndices));
    m_PlaneVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout planeLayout;
    planeLayout.PushFloat(3);
    planeLayout.PushFloat(3);
    planeLayout.PushFloat(2);
    planeLayout.PushFloat(3);
    planeLayout.PushFloat(3);

    m_PlaneVAO->SetVertexBuffer(m_PlaneVBO.get());
    m_PlaneVAO->SetIndexBuffer(m_PlaneEBO.get());
    m_PlaneVAO->SetLayout(planeLayout);

    m_InfiniteGridShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/infinite_grid.vert",
        "MMOGame/Editor3D/assets/shaders/infinite_grid.frag"
    );

    float gridVertices[] = {
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
    gridLayout.PushFloat(2);

    m_GridVAO->SetVertexBuffer(m_GridVBO.get());
    m_GridVAO->SetIndexBuffer(m_GridEBO.get());
    m_GridVAO->SetLayout(gridLayout);

    UpdateCameraVectors();

    m_Gizmo = std::make_unique<TransformGizmo>();
    m_Gizmo->Init();

    CreateDefaultTextures();

    m_PickingFramebuffer = std::make_unique<Onyx::Framebuffer>();
    m_PickingFramebuffer->Create(800, 600, 1);

    m_PickingShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/picking.vert",
        "MMOGame/Editor3D/assets/shaders/picking.frag"
    );

    m_PickingBillboardShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/picking_billboard.vert",
        "MMOGame/Editor3D/assets/shaders/picking_billboard.frag"
    );

    float billboardVertices[] = {
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
    billboardLayout.PushFloat(2);
    billboardLayout.PushFloat(2);

    m_BillboardVAO->SetVertexBuffer(m_BillboardVBO.get());
    m_BillboardVAO->SetIndexBuffer(m_BillboardEBO.get());
    m_BillboardVAO->SetLayout(billboardLayout);

    m_LightIconTexture = Onyx::Texture::CreateSolidColor(255, 220, 50);
    m_SpawnIconTexture = Onyx::Texture::CreateSolidColor(50, 220, 100);
    m_ParticleIconTexture = Onyx::Texture::CreateSolidColor(220, 50, 220);
    m_PortalIconTexture = Onyx::Texture::CreateSolidColor(50, 200, 255);
    m_TriggerIconTexture = Onyx::Texture::CreateSolidColor(255, 150, 50);

    m_CSM = std::make_unique<Onyx::CascadedShadowMap>();
    m_CSM->Create(m_ShadowMapSize);

    m_ShadowDepthShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/shadow_depth.vert",
        "MMOGame/Editor3D/assets/shaders/shadow_depth.frag"
    );

    m_TerrainShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor3D/assets/shaders/terrain.vert",
        "MMOGame/Editor3D/assets/shaders/terrain.frag"
    );
    m_TerrainMaterialLibrary.Init("MMOGame/Editor3D/assets");

    const char* defaultMatIds[] = {"dirt", "grass", "rock", "sand"};
    std::string defaults[Editor3D::MAX_TERRAIN_LAYERS] = {};
    for (int i = 0; i < 4; i++) {
        if (m_TerrainMaterialLibrary.GetMaterial(defaultMatIds[i])) {
            defaults[i] = defaultMatIds[i];
        }
    }
    m_TerrainSystem.SetDefaultMaterialIds(defaults);

    m_TerrainSystem.Init("");
}

void ViewportPanel::OnImGuiRender() {
    ImGuiWindowFlags vpFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", nullptr, vpFlags);

    m_ViewportFocused = ImGui::IsWindowFocused();
    m_ViewportHovered = ImGui::IsWindowHovered();

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
        m_PickingFramebuffer->Create(newWidth, newHeight, 1);
    }

    if (m_ViewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !m_RightMouseDown) {
        m_WantsContextMenu = true;
    }

    HandleCameraInput();

    m_ViewMatrix = glm::lookAt(m_CameraPosition, m_CameraPosition + m_CameraFront, m_CameraUp);
    m_ProjectionMatrix = glm::perspective(
        glm::radians(60.0f),
        m_ViewportWidth / m_ViewportHeight,
        0.1f,
        1000.0f
    );

    HandleGizmoInteraction();
    HandleObjectPicking();

    if (m_TerrainEnabled) {
        m_TerrainSystem.SetNormalMode(m_TerrainTool.sobelNormals, m_TerrainTool.smoothNormals);
        m_TerrainSystem.SetDiamondGrid(m_TerrainTool.diamondGrid);
        m_TerrainSystem.SetMeshResolution(m_TerrainTool.meshResolution);
        glm::mat4 VP = m_ProjectionMatrix * m_ViewMatrix;
        m_TerrainSystem.Update(m_CameraPosition, VP, 0.016f);
        if (m_TerrainTool.toolActive && m_ViewportHovered) {
            HandleTerrainInput();
        }
    }

    RenderScene();

    ImGui::Image(
        static_cast<ImTextureID>(m_Framebuffer->GetColorBufferID()),
        ImVec2(m_ViewportWidth, m_ViewportHeight),
        ImVec2(0, 1), ImVec2(1, 0)
    );

    ImGui::SetCursorPos(ImVec2(0, 30));
    ImGui::Checkbox("Show Grid", &m_ShowGrid);
    ImGui::Checkbox("Wireframe", &m_ShowWireframe);
    if (ImGui::Checkbox("MSAA 4x", &m_EnableMSAA)) {
        uint32_t samples = m_EnableMSAA ? 4 : 1;
        m_Framebuffer->Create(static_cast<uint32_t>(m_ViewportWidth),
                              static_cast<uint32_t>(m_ViewportHeight), samples);
    }

    if (m_WantsContextMenu && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("ViewportContextMenu");
        m_WantsContextMenu = false;
    }
    if (m_WantsContextMenu && m_RightMouseDown) {
        m_WantsContextMenu = false;
    }

    if (ImGui::BeginPopup("ViewportContextMenu")) {
        bool hasSelection = m_World && m_World->HasSelection();
        bool hasGroupSelected = false;

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

void ViewportPanel::RenderShadowPass() {
    if (!m_EnableShadows || !m_EnableDirectionalLight || !m_CSM || !m_ShadowDepthShader) return;

    m_CSM->Update(m_ViewMatrix, m_ProjectionMatrix,
                  glm::normalize(m_LightDir), 0.1f, m_ShadowDistance, m_SplitLambda);
    m_CSM->ClearAll();

    Onyx::RenderCommand::EnableDepthTest();
    Onyx::RenderCommand::EnableCulling();
    Onyx::RenderCommand::SetCullFace(true);

    m_ShadowDepthShader->Bind();

    for (uint32_t cascade = 0; cascade < Onyx::NUM_SHADOW_CASCADES; cascade++) {
        m_CSM->BindCascade(cascade);
        m_ShadowDepthShader->SetMat4("u_LightSpaceMatrix", m_CSM->GetLightSpaceMatrix(cascade));

        if (m_World) {
            for (const auto& obj : m_World->GetStaticObjects()) {
                if (!obj->IsVisible() || !obj->CastsShadow()) continue;

                glm::mat4 modelMatrix = m_World->GetWorldMatrix(obj.get());
                const std::string& modelPath = obj->GetModelPath();

                if (modelPath == "#plane") {
                    m_ShadowDepthShader->SetMat4("u_Model", modelMatrix);
                    Onyx::RenderCommand::DrawIndexed(*m_PlaneVAO, 6);
                    continue;
                }

                if (!modelPath.empty()) {
                    Onyx::Model* model = GetOrLoadModel(modelPath);
                    if (model) {
                        for (size_t meshIdx = 0; meshIdx < model->GetMeshes().size(); meshIdx++) {
                            auto& mesh = model->GetMeshes()[meshIdx];
                            std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(meshIdx)) : mesh.m_Name;
                            const MeshMaterial* meshMat = obj->GetMeshMaterial(meshName);
                            if (meshMat && !meshMat->visible) continue;

                            glm::mat4 meshModelMatrix = modelMatrix;
                            if (meshMat) {
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
                                meshModelMatrix = glm::translate(meshModelMatrix, meshMat->positionOffset);
                                meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(meshMat->rotationOffset.x), glm::vec3(1, 0, 0));
                                meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(meshMat->rotationOffset.y), glm::vec3(0, 1, 0));
                                meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(meshMat->rotationOffset.z), glm::vec3(0, 0, 1));
                                meshModelMatrix = glm::translate(meshModelMatrix, meshCenter);
                                meshModelMatrix = glm::scale(meshModelMatrix, glm::vec3(meshMat->scaleMultiplier));
                                meshModelMatrix = glm::translate(meshModelMatrix, -meshCenter);
                            }
                            m_ShadowDepthShader->SetMat4("u_Model", meshModelMatrix);
                            mesh.DrawGeometryOnly();
                        }
                    }
                } else {
                    m_ShadowDepthShader->SetMat4("u_Model", modelMatrix);
                    Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
                }
            }
        }

            if (m_TerrainEnabled) {
            m_ShadowDepthShader->SetMat4("u_Model", glm::mat4(1.0f));
            for (auto& [key, chunk] : m_TerrainSystem.GetChunks()) {
                if (chunk->GetState() != Editor3D::ChunkState::Active) continue;
                chunk->Draw(m_ShadowDepthShader.get());
            }
        }
    }

    m_CSM->UnBind();
    Onyx::RenderCommand::SetCullFace(false);
    Onyx::RenderCommand::DisableCulling();
}

void ViewportPanel::RenderScene() {
    m_TriangleCount = 0;
    m_DrawCalls = 0;

    double totalStart = glfwGetTime();

    double t0 = glfwGetTime();
    RenderShadowPass();
    Onyx::RenderCommand::Finish();
    m_ShadowPassTime = static_cast<float>((glfwGetTime() - t0) * 1000.0);

    m_Framebuffer->Bind();
    m_Framebuffer->Clear(0.15f, 0.15f, 0.18f, 1.0f);

    Onyx::RenderCommand::EnableDepthTest();
    Onyx::RenderCommand::SetDepthMask(true);
    Onyx::RenderCommand::DisableCulling();

    if (m_ShowGrid) {
        RenderGrid();
    }

    t0 = glfwGetTime();
    if (m_TerrainEnabled) {
        if (m_ShowWireframe) Onyx::RenderCommand::SetWireframeMode(true);
        RenderTerrain();
        if (m_ShowWireframe) Onyx::RenderCommand::SetWireframeMode(false);
    }
    Onyx::RenderCommand::Finish();
    m_TerrainPassTime = static_cast<float>((glfwGetTime() - t0) * 1000.0);

    t0 = glfwGetTime();
    RenderWorldObjects();
    Onyx::RenderCommand::Finish();
    m_WorldObjectsPassTime = static_cast<float>((glfwGetTime() - t0) * 1000.0);

    RenderGizmo();

    Onyx::RenderCommand::ResetState();
    m_Framebuffer->UnBind();

    m_Framebuffer->Resolve();

    Onyx::RenderCommand::Finish();
    m_TotalRenderTime = static_cast<float>((glfwGetTime() - totalStart) * 1000.0);
}

void ViewportPanel::RenderGrid() {
    Onyx::RenderCommand::EnableBlending();

    m_InfiniteGridShader->Bind();
    m_InfiniteGridShader->SetMat4("u_View", m_ViewMatrix);
    m_InfiniteGridShader->SetMat4("u_Projection", m_ProjectionMatrix);
    m_InfiniteGridShader->SetFloat("u_GridScale", 0.5f);   // Larger cells (2 units)
    m_InfiniteGridShader->SetFloat("u_GridFadeStart", 80.0f);
    m_InfiniteGridShader->SetFloat("u_GridFadeEnd", 200.0f);

    Onyx::RenderCommand::DrawIndexed(*m_GridVAO, 6);

    Onyx::RenderCommand::DisableBlending();
}

void ViewportPanel::RenderWorldObjects() {
    if (!m_World) return;

    if (m_ShowWireframe) {
        Onyx::RenderCommand::SetWireframeMode(true);
    }

    for (const auto& obj : m_World->GetStaticObjects()) {
        if (!obj->IsVisible()) continue;

        glm::mat4 modelMatrix = m_World->GetWorldMatrix(obj.get());
        const std::string& modelPath = obj->GetModelPath();

        if (modelPath == "#plane") {
            m_ModelShader->Bind();
            m_ModelShader->SetMat4("u_Model", modelMatrix);
            m_ModelShader->SetMat4("u_View", m_ViewMatrix);
            m_ModelShader->SetMat4("u_Projection", m_ProjectionMatrix);
            m_ModelShader->SetVec3("u_LightDir", m_LightDir);
            m_ModelShader->SetVec3("u_LightColor", m_EnableDirectionalLight ? m_LightColor : glm::vec3(0.0f));
            m_ModelShader->SetVec3("u_ViewPos", m_CameraPosition);
            m_ModelShader->SetFloat("u_AmbientStrength", m_AmbientStrength);
            m_ModelShader->SetInt("u_AlbedoMap", 0);
            m_ModelShader->SetInt("u_NormalMap", 1);
            m_ModelShader->SetInt("u_ShadowMap", 2);
            m_ModelShader->SetInt("u_EnableShadows", m_EnableShadows ? 1 : 0);
            m_ModelShader->SetFloat("u_ShadowBias", m_ShadowBias);
            m_ModelShader->SetInt("u_ShowCascades", m_ShowCascades ? 1 : 0);
            m_ModelShader->SetInt("u_UseNormalMap", 0);

            if (m_EnableShadows && m_CSM) {
                auto& matrices = m_CSM->GetLightSpaceMatrices();
                auto& splits = m_CSM->GetCascadeSplits();
                for (uint32_t i = 0; i < Onyx::NUM_SHADOW_CASCADES; i++) {
                    m_ModelShader->SetMat4("u_LightSpaceMatrices[" + std::to_string(i) + "]", matrices[i]);
                    m_ModelShader->SetFloat("u_CascadeSplits[" + std::to_string(i) + "]", splits[i]);
                }
                Onyx::RenderCommand::BindTextureArray(2, m_CSM->GetDepthTextureArray());
            }

            Onyx::Texture* diffuseTexture = GetOrLoadTexture(obj->GetDiffuseTexture());
            if (diffuseTexture) {
                diffuseTexture->Bind(0);
            } else {
                m_DefaultWhiteTexture->Bind(0);
            }

            Onyx::RenderCommand::DrawIndexed(*m_PlaneVAO, 6);
            m_TriangleCount += 2;
            m_DrawCalls++;

            if (obj->IsSelected()) {
                Onyx::RenderCommand::SetWireframeMode(true);
                Onyx::RenderCommand::SetLineWidth(2.0f);
                Onyx::RenderCommand::DrawIndexed(*m_PlaneVAO, 6);
                m_DrawCalls++;
                Onyx::RenderCommand::SetWireframeMode(false);
            }
            continue;
        }

        if (!modelPath.empty()) {
            Onyx::Model* model = GetOrLoadModel(modelPath);
            if (model) {
                glm::vec3 lightColor(1.0f);

                m_ModelShader->Bind();
                m_ModelShader->SetMat4("u_Model", modelMatrix);
                m_ModelShader->SetMat4("u_View", m_ViewMatrix);
                m_ModelShader->SetMat4("u_Projection", m_ProjectionMatrix);
                m_ModelShader->SetVec3("u_LightDir", m_LightDir);
                m_ModelShader->SetVec3("u_LightColor", m_EnableDirectionalLight ? m_LightColor : glm::vec3(0.0f));
                m_ModelShader->SetVec3("u_ViewPos", m_CameraPosition);
                m_ModelShader->SetFloat("u_AmbientStrength", m_AmbientStrength);
                m_ModelShader->SetInt("u_AlbedoMap", 0);
                m_ModelShader->SetInt("u_NormalMap", 1);
                m_ModelShader->SetInt("u_ShadowMap", 2);
                m_ModelShader->SetInt("u_EnableShadows", m_EnableShadows ? 1 : 0);
                m_ModelShader->SetFloat("u_ShadowBias", m_ShadowBias);
                m_ModelShader->SetInt("u_ShowCascades", m_ShowCascades ? 1 : 0);

                if (m_EnableShadows && m_CSM) {
                    auto& matrices = m_CSM->GetLightSpaceMatrices();
                    auto& splits = m_CSM->GetCascadeSplits();
                    for (uint32_t i = 0; i < Onyx::NUM_SHADOW_CASCADES; i++) {
                        m_ModelShader->SetMat4("u_LightSpaceMatrices[" + std::to_string(i) + "]", matrices[i]);
                        m_ModelShader->SetFloat("u_CascadeSplits[" + std::to_string(i) + "]", splits[i]);
                    }
                    Onyx::RenderCommand::BindTextureArray(2, m_CSM->GetDepthTextureArray());
                }

                Onyx::Texture* defaultDiffuse = GetOrLoadTexture(obj->GetDiffuseTexture());
                Onyx::Texture* defaultNormal = GetOrLoadTexture(obj->GetNormalTexture());

                for (size_t meshIdx = 0; meshIdx < model->GetMeshes().size(); meshIdx++) {
                    auto& mesh = model->GetMeshes()[meshIdx];

                    std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(meshIdx)) : mesh.m_Name;
                    const MeshMaterial* meshMat = obj->GetMeshMaterial(meshName);

                    if (meshMat && !meshMat->visible) {
                        continue;
                    }

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

                    glm::mat4 meshModelMatrix = modelMatrix;
                    if (meshMat) {
                        meshModelMatrix = glm::translate(meshModelMatrix, meshMat->positionOffset);
                        meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(meshMat->rotationOffset.x), glm::vec3(1, 0, 0));
                        meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(meshMat->rotationOffset.y), glm::vec3(0, 1, 0));
                        meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(meshMat->rotationOffset.z), glm::vec3(0, 0, 1));
                        meshModelMatrix = glm::translate(meshModelMatrix, meshCenter);
                        meshModelMatrix = glm::scale(meshModelMatrix, glm::vec3(meshMat->scaleMultiplier));
                        meshModelMatrix = glm::translate(meshModelMatrix, -meshCenter);
                    }
                    m_ModelShader->SetMat4("u_Model", meshModelMatrix);

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

                    if (!diffuseTexture) diffuseTexture = defaultDiffuse;
                    if (!normalTexture) normalTexture = defaultNormal;

                    if (diffuseTexture) {
                        diffuseTexture->Bind(0);
                    } else {
                        m_DefaultWhiteTexture->Bind(0);
                    }

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

                if (obj->IsSelected()) {
                    Onyx::RenderCommand::SetWireframeMode(true);
                    Onyx::RenderCommand::SetLineWidth(2.0f);

                    int selectedMeshIdx = m_World->GetSelectedMeshIndex();
                    if (selectedMeshIdx >= 0 && selectedMeshIdx < static_cast<int>(model->GetMeshes().size())) {
                            auto& mesh = model->GetMeshes()[selectedMeshIdx];
                        std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(selectedMeshIdx)) : mesh.m_Name;
                        const MeshMaterial* meshMat = obj->GetMeshMaterial(meshName);

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
                            highlightMatrix = glm::translate(highlightMatrix, meshCenter);
                            highlightMatrix = glm::scale(highlightMatrix, glm::vec3(meshMat->scaleMultiplier));
                            highlightMatrix = glm::translate(highlightMatrix, -meshCenter);
                        }
                        m_ModelShader->SetMat4("u_Model", highlightMatrix);
                        mesh.DrawGeometryOnly();
                        m_DrawCalls++;
                    } else {
                        for (size_t idx = 0; idx < model->GetMeshes().size(); idx++) {
                            auto& mesh = model->GetMeshes()[idx];
                            std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(idx)) : mesh.m_Name;
                            const MeshMaterial* meshMat = obj->GetMeshMaterial(meshName);

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

    for (const auto& spawn : m_World->GetSpawnPoints()) {
        if (!spawn->IsVisible()) continue;

        glm::mat4 modelMatrix = m_World->GetWorldMatrix(spawn.get());
        const std::string& modelPath = spawn->GetModelPath();
        if (!modelPath.empty()) {
            Onyx::Model* model = GetOrLoadModel(modelPath);
            if (model) {
                glm::vec3 lightColor = m_EnableDirectionalLight ? glm::vec3(1.0f) : glm::vec3(0.0f);

                m_ModelShader->Bind();
                m_ModelShader->SetMat4("u_Model", modelMatrix);
                m_ModelShader->SetMat4("u_View", m_ViewMatrix);
                m_ModelShader->SetMat4("u_Projection", m_ProjectionMatrix);
                m_ModelShader->SetVec3("u_LightDir", m_LightDir);
                m_ModelShader->SetVec3("u_LightColor", lightColor);
                m_ModelShader->SetVec3("u_ViewPos", m_CameraPosition);
                m_ModelShader->SetFloat("u_AmbientStrength", m_AmbientStrength);
                m_ModelShader->SetInt("u_UseNormalMap", 0);

                Onyx::Texture* diffuseTexture = GetOrLoadTexture(spawn->GetDiffuseTexture());
                Onyx::Texture* normalTexture = GetOrLoadTexture(spawn->GetNormalTexture());

                if (diffuseTexture) {
                    diffuseTexture->Bind(0);
                } else {
                    m_DefaultWhiteTexture->Bind(0);
                }
                m_ModelShader->SetInt("u_AlbedoMap", 0);

                if (normalTexture) {
                    normalTexture->Bind(1);
                    m_ModelShader->SetInt("u_NormalMap", 1);
                    m_ModelShader->SetInt("u_UseNormalMap", 1);
                } else {
                    m_ModelShader->SetInt("u_UseNormalMap", 0);
                }

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

    m_ObjectShader->Bind();
    m_ObjectShader->SetMat4("u_View", m_ViewMatrix);
    m_ObjectShader->SetMat4("u_Projection", m_ProjectionMatrix);

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

    if (m_ShowWireframe) {
        Onyx::RenderCommand::SetWireframeMode(false);
    }
}

void ViewportPanel::RenderSelectionOutline() {
}

void ViewportPanel::RenderGizmoIcons() {
    // TODO: Render billboard icons for lights, spawns, etc.
}

void ViewportPanel::RenderGizmo() {
    if (!m_World || !m_Gizmo) return;

    WorldObject* selection = m_World->GetPrimarySelection();
    if (!selection) return;

    m_Gizmo->SetMode(m_World->GetGizmoMode());
    m_Gizmo->SetSnapEnabled(m_World->IsSnapEnabled());
    m_Gizmo->SetSnapValue(m_World->GetSnapValue());

    glm::mat4 worldMatrix = m_World->GetWorldMatrix(selection);
    glm::vec3 gizmoPos = glm::vec3(worldMatrix[3]);

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

                glm::vec3 meshOffset = meshMat ? meshMat->positionOffset : glm::vec3(0.0f);
                glm::vec3 localPos = meshCenter + meshOffset;

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

    float mouseX = io.MousePos.x - m_ViewportPos.x;
    float mouseY = io.MousePos.y - m_ViewportPos.y;

    glm::vec3 rayDir = ScreenToWorldRay(mouseX, mouseY);
    glm::vec3 rayOrigin = m_CameraPosition;

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
    glm::mat4 selectionWorldMatrix = m_World->GetWorldMatrix(selection);
    glm::vec3 gizmoPos = glm::vec3(selectionWorldMatrix[3]);
    glm::vec3 meshCenter(0.0f);
    if (isMeshSelected && staticObj) {
        const std::string& modelPath = staticObj->GetModelPath();
        Onyx::Model* model = GetOrLoadModel(modelPath);
        if (model && selectedMeshIdx < static_cast<int>(model->GetMeshes().size())) {
            auto& mesh = model->GetMeshes()[selectedMeshIdx];
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

    if (!m_Gizmo->IsDragging()) {
        GizmoAxis hoverAxis = m_Gizmo->TestHit(rayOrigin, rayDir, gizmoPos, camDist);
        m_Gizmo->SetActiveAxis(hoverAxis);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_Gizmo->GetActiveAxis() != GizmoAxis::NONE) {
        m_Gizmo->BeginDrag(gizmoPos, rayOrigin, rayDir, m_ViewMatrix);

        if (isMeshSelected && meshMaterial) {
            m_GizmoStartMeshOffset = meshMaterial->positionOffset;
            m_GizmoStartMeshRotation = meshMaterial->rotationOffset;
            m_GizmoStartMeshScale = meshMaterial->scaleMultiplier;
        } else {
            m_GizmoStartObjectPos = selection->GetPosition();
            m_GizmoStartObjectRot = selection->GetRotation();
            m_GizmoStartObjectScale = selection->GetScale();
            std::cout << "[Gizmo] Drag START - captured pos=("
                      << m_GizmoStartObjectPos.x << "," << m_GizmoStartObjectPos.y << "," << m_GizmoStartObjectPos.z
                      << ") scale=" << m_GizmoStartObjectScale << std::endl;
        }
    }

    if (m_Gizmo->IsDragging() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        GizmoMode mode = m_Gizmo->GetMode();
        GizmoAxis axis = m_Gizmo->GetActiveAxis();

        if (isMeshSelected && meshMaterial && staticObj) {
            if (mode == GizmoMode::TRANSLATE) {
                glm::vec3 translation = m_Gizmo->CalculateTranslation(
                    rayOrigin, rayDir, gizmoPos, axis,
                    m_ViewMatrix, m_ProjectionMatrix
                );

                glm::quat invRot = glm::inverse(staticObj->GetRotation());
                glm::vec3 localTranslation = invRot * translation / staticObj->GetScale();

                meshMaterial->positionOffset = m_GizmoStartMeshOffset + localTranslation;
            }
            else if (mode == GizmoMode::ROTATE) {
                float deltaAngle = m_Gizmo->CalculateRotation(
                    rayOrigin, rayDir, gizmoPos, axis, m_ViewMatrix
                );

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

                float factor = 1.0f;
                if (axis == GizmoAxis::X) factor = scaleFactor.x;
                else if (axis == GizmoAxis::Y) factor = scaleFactor.y;
                else if (axis == GizmoAxis::Z) factor = scaleFactor.z;
                else factor = scaleFactor.x;

                meshMaterial->scaleMultiplier = m_GizmoStartMeshScale * factor;
            }
        } else {
            if (mode == GizmoMode::TRANSLATE) {
                glm::vec3 translation = m_Gizmo->CalculateTranslation(
                    rayOrigin, rayDir, gizmoPos, axis,
                    m_ViewMatrix, m_ProjectionMatrix
                );

                for (WorldObject* obj : m_World->GetSelectedObjects()) {
                    obj->SetPosition(m_GizmoStartObjectPos + translation);
                }
            }
            else if (mode == GizmoMode::ROTATE) {
                float deltaAngle = m_Gizmo->CalculateRotation(
                    rayOrigin, rayDir, gizmoPos, axis, m_ViewMatrix
                );

                glm::vec3 rotAxis;
                if (axis == GizmoAxis::X) rotAxis = glm::vec3(1, 0, 0);
                else if (axis == GizmoAxis::Y) rotAxis = glm::vec3(0, 1, 0);
                else rotAxis = glm::vec3(0, 0, 1);

                glm::quat deltaRot = glm::angleAxis(deltaAngle, rotAxis);
                for (WorldObject* obj : m_World->GetSelectedObjects()) {
                    obj->SetRotation(deltaRot * m_GizmoStartObjectRot);
                }
            }
            else if (mode == GizmoMode::SCALE) {
                glm::vec3 scaleFactor = m_Gizmo->CalculateScale(
                    rayOrigin, rayDir, gizmoPos, axis, m_ViewMatrix
                );

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

    bool mouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    bool gizmoDragging = m_Gizmo->IsDragging();
    if (mouseReleased && gizmoDragging) {
        std::cout << "[Gizmo] Drag END triggered (mouseReleased=" << mouseReleased << " gizmoDragging=" << gizmoDragging << ")" << std::endl;
        if (!isMeshSelected) {
            glm::vec3 newPos = selection->GetPosition();
            glm::quat newRot = selection->GetRotation();
            float newScale = selection->GetScale();

            std::cout << "[Gizmo] Drag END - startPos=("
                      << m_GizmoStartObjectPos.x << "," << m_GizmoStartObjectPos.y << "," << m_GizmoStartObjectPos.z
                      << ") currentPos=(" << newPos.x << "," << newPos.y << "," << newPos.z
                      << ") startScale=" << m_GizmoStartObjectScale << " currentScale=" << newScale << std::endl;

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
                CommandHistory::Get().AddWithoutExecute(std::move(cmd));
            }
        } else if (isMeshSelected && meshMaterial && staticObj) {
            glm::vec3 newOffset = meshMaterial->positionOffset;
            glm::vec3 newRotation = meshMaterial->rotationOffset;
            float newScale = meshMaterial->scaleMultiplier;

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
        if (ImGui::IsKeyPressed(ImGuiKey_G) && !io.KeyCtrl) {
            m_World->SetSnapEnabled(!m_World->IsSnapEnabled());
        }

        bool ctrlHeld = io.KeyCtrl;
        bool shiftHeld = io.KeyShift;
        if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_C)) {
            if (shiftHeld && m_World->HasMeshSelected()) {
                std::cout << "[Copy] Copying mesh transform" << std::endl;
                m_World->CopyMesh();
            } else if (m_World->HasMeshSelected()) {
                std::cout << "[Copy] Copying single mesh as object" << std::endl;
                m_World->CopySingleMesh();
            } else {
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
                std::cout << "[Duplicate] Duplicating single mesh as object" << std::endl;
                m_World->CopySingleMesh();
                m_World->Paste();
            } else {
                m_World->Duplicate();
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            m_World->DeleteSelected();
        }

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

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if (!m_RightMouseDown) {
            m_RightMouseDown = true;
            m_FirstMouse = true;
        }

        if (!m_FirstMouse) {
            float xOffset = io.MouseDelta.x * m_CameraSensitivity;
            float yOffset = -io.MouseDelta.y * m_CameraSensitivity;

            m_CameraYaw += xOffset;
            m_CameraPitch += yOffset;
            m_CameraPitch = glm::clamp(m_CameraPitch, -89.0f, 89.0f);

            UpdateCameraVectors();
        }
        m_FirstMouse = false;

        float speed = m_CameraSpeed * deltaTime;
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift))
            speed *= 3.0f;

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

    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        float panSpeed = 0.02f;
        m_CameraPosition -= m_CameraRight * io.MouseDelta.x * panSpeed;
        m_CameraPosition += m_CameraUp * io.MouseDelta.y * panSpeed;
    }

    if (io.MouseWheel != 0.0f) {
        m_CameraPosition += m_CameraFront * io.MouseWheel * m_CameraSpeed * 0.1f;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F)) {
        FocusOnSelection();
    }
}

void ViewportPanel::HandleObjectPicking() {
    if (!m_World || !m_ViewportHovered) return;

    if (m_Gizmo && m_Gizmo->GetActiveAxis() != GizmoAxis::NONE) return;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        ImGuiIO& io = ImGui::GetIO();

            float mouseX = io.MousePos.x - m_ViewportPos.x;
        float mouseY = io.MousePos.y - m_ViewportPos.y;

        std::cout << "[Pick] Mouse: (" << io.MousePos.x << "," << io.MousePos.y << ") "
                  << "ViewportPos: (" << m_ViewportPos.x << "," << m_ViewportPos.y << ") "
                  << "Relative: (" << mouseX << "," << mouseY << ") "
                  << "ViewportSize: (" << m_ViewportWidth << "x" << m_ViewportHeight << ")" << std::endl;

        RenderPickingPass();
        PickResult pick = ReadPickingBuffer(mouseX, mouseY);

        bool addToSelection = io.KeyShift;
        if (pick.hit) {
            m_World->SelectByGuid(pick.objectGuid, addToSelection);

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
    float x = (2.0f * screenX) / m_ViewportWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / m_ViewportHeight;

    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    glm::vec4 rayEye = glm::inverse(m_ProjectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec3 rayWorld = glm::vec3(glm::inverse(m_ViewMatrix) * rayEye);
    return glm::normalize(rayWorld);
}


void ViewportPanel::RenderPickingPass() {
    if (!m_World) return;

    m_PickingFramebuffer->Bind();
    m_PickingFramebuffer->Clear(0.0f, 0.0f, 0.0f, 0.0f);

    Onyx::RenderCommand::EnableDepthTest();
    Onyx::RenderCommand::SetDepthMask(true);
    Onyx::RenderCommand::DisableBlending();
    Onyx::RenderCommand::DisableCulling();
    Onyx::RenderCommand::SetWireframeMode(false);

    m_PickingShader->Bind();
    m_PickingShader->SetMat4("u_View", m_ViewMatrix);
    m_PickingShader->SetMat4("u_Projection", m_ProjectionMatrix);

    for (const auto& obj : m_World->GetStaticObjects()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        RenderObjectForPicking(obj.get());
    }

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
        RenderIconForPicking(obj.get(), WorldObjectType::SPAWN_POINT, 1.0f);
    }

    for (const auto& obj : m_World->GetTriggerVolumes()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        uint32_t objID = static_cast<uint32_t>(obj->GetGuid() & 0xFFFF);

        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), obj->GetPosition());
        modelMatrix = glm::scale(modelMatrix, obj->GetHalfExtents() * 2.0f);

        m_PickingShader->SetMat4("u_Model", modelMatrix);
        m_PickingShader->SetInt("u_ObjectID", static_cast<int>(objID));
        m_PickingShader->SetInt("u_MeshIndex", 0xFFF);
        m_PickingShader->SetInt("u_ObjectType", static_cast<int>(WorldObjectType::TRIGGER_VOLUME));
        Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
    }

    m_PickingBillboardShader->Bind();
    m_PickingBillboardShader->SetMat4("u_View", m_ViewMatrix);
    m_PickingBillboardShader->SetMat4("u_Projection", m_ProjectionMatrix);
    m_PickingBillboardShader->SetInt("u_IconTexture", 0);
    m_PickingBillboardShader->SetInt("u_UseTexture", 0);

    for (const auto& obj : m_World->GetLights()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        RenderIconForPicking(obj.get(), WorldObjectType::LIGHT, 0.8f);
    }

    for (const auto& obj : m_World->GetParticleEmitters()) {
        if (!obj->IsVisible() || obj->IsLocked()) continue;
        RenderIconForPicking(obj.get(), WorldObjectType::PARTICLE_EMITTER, 0.6f);
    }

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

                    static int s_DebugCount = 0;
            if (s_DebugCount < 3) {
                std::cout << "[PickRender] Rendering " << model->GetMeshes().size() << " meshes for object " << objID << std::endl;
                s_DebugCount++;
            }

            for (size_t meshIdx = 0; meshIdx < model->GetMeshes().size(); meshIdx++) {
                auto& mesh = model->GetMeshes()[meshIdx];
                std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(meshIdx)) : mesh.m_Name;
                const MeshMaterial* meshMat = obj->GetMeshMaterial(meshName);

                if (meshMat && !meshMat->visible) {
                    continue;
                }

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

                glm::mat4 meshMatrix = baseMatrix;
                if (meshMat) {
                    meshMatrix = glm::translate(meshMatrix, meshMat->positionOffset);
                    meshMatrix = glm::rotate(meshMatrix, glm::radians(meshMat->rotationOffset.x), glm::vec3(1, 0, 0));
                    meshMatrix = glm::rotate(meshMatrix, glm::radians(meshMat->rotationOffset.y), glm::vec3(0, 1, 0));
                    meshMatrix = glm::rotate(meshMatrix, glm::radians(meshMat->rotationOffset.z), glm::vec3(0, 0, 1));
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

    glm::mat4 modelMatrix = m_World->GetWorldMatrix(obj);
    m_PickingShader->SetMat4("u_Model", modelMatrix);
    m_PickingShader->SetInt("u_ObjectID", static_cast<int>(objID));
    m_PickingShader->SetInt("u_MeshIndex", 0xFFF);
    m_PickingShader->SetInt("u_ObjectType", static_cast<int>(WorldObjectType::STATIC_OBJECT));
    Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, 36);
}

void ViewportPanel::RenderIconForPicking(WorldObject* obj, WorldObjectType type, float size) {
    uint32_t objID = static_cast<uint32_t>(obj->GetGuid() & 0xFFFF);

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

    int pickWidth = m_PickingFramebuffer->GetWidth();
    int pickHeight = m_PickingFramebuffer->GetHeight();

    int x = static_cast<int>((mouseX / m_ViewportWidth) * pickWidth);
    int y = static_cast<int>(((m_ViewportHeight - mouseY) / m_ViewportHeight) * pickHeight);

    x = std::clamp(x, 0, pickWidth - 1);
    y = std::clamp(y, 0, pickHeight - 1);

    Onyx::RenderCommand::Finish();

    m_PickingFramebuffer->Bind();
    unsigned char pixel[4];
    Onyx::RenderCommand::ReadPixels(x, y, 1, 1, pixel);
    m_PickingFramebuffer->UnBind();

    if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0 && pixel[3] == 0) {
        return result;
    }

    result.hit = true;
    result.objectGuid = (static_cast<uint64_t>(pixel[0]) << 8) | static_cast<uint64_t>(pixel[1]);

    int meshLow = pixel[2];
    int meshHigh = (pixel[3] >> 4) & 0x0F;
    int meshIndex12 = (meshHigh << 8) | meshLow;
    result.meshIndex = (meshIndex12 == 0xFFF) ? -1 : meshIndex12;

    result.type = static_cast<WorldObjectType>(pixel[3] & 0x0F);

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
    float distance = 10.0f;
    m_CameraPosition = targetPos - m_CameraFront * distance;
}

void ViewportPanel::SetShadowMapSize(uint32_t size) {
    if (size == m_ShadowMapSize) return;
    m_ShadowMapSize = size;
    if (m_CSM) {
        m_CSM->Resize(size);
    }
}

void ViewportPanel::FocusOnSelection() {
    if (!m_World || !m_World->HasSelection()) return;

    const auto& selected = m_World->GetSelectedObjects();

    if (selected.size() == 1) {
        FocusOnObject(selected[0]);
    } else {
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

    auto it = m_ModelCache.find(path);
    if (it != m_ModelCache.end()) {
        return it->second.get();
    }

    try {
        auto model = std::make_unique<Onyx::Model>(path.c_str(), false);
        Onyx::Model* modelPtr = model.get();
        m_ModelCache[path] = std::move(model);
        return modelPtr;
    } catch (...) {
        std::cerr << "Failed to load model: " << path << std::endl;
        m_ModelCache[path] = nullptr;
        return nullptr;
    }
}

void ViewportPanel::CreateDefaultTextures() {
    m_DefaultWhiteTexture = Onyx::Texture::CreateSolidColor(255, 255, 255);
}

Onyx::Texture* ViewportPanel::GetOrLoadTexture(const std::string& path) {
    if (path.empty()) return nullptr;

    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end()) {
        return it->second.get();
    }

    try {
        auto texture = std::make_unique<Onyx::Texture>(path.c_str());
        Onyx::Texture* texturePtr = texture.get();
        m_TextureCache[path] = std::move(texture);
        return texturePtr;
    } catch (...) {
        m_TextureCache[path] = nullptr;
        return nullptr;
    }
}


bool ViewportPanel::IsAnimatedModel(const std::string& path) {
    auto* model = GetOrLoadAnimatedModel(path);
    return model && model->GetAnimationCount() > 0;
}

Onyx::AnimatedModel* ViewportPanel::GetAnimatedModel(const std::string& path) {
    return GetOrLoadAnimatedModel(path);
}

Onyx::Animator* ViewportPanel::GetAnimator(uint64_t objectGuid) {
    auto it = m_AnimatorCache.find(objectGuid);
    if (it != m_AnimatorCache.end()) return it->second.get();

    auto animator = std::make_unique<Onyx::Animator>();
    Onyx::Animator* ptr = animator.get();
    m_AnimatorCache[objectGuid] = std::move(animator);
    return ptr;
}

void ViewportPanel::InvalidateAnimator(uint64_t objectGuid) {
    m_AnimatorCache.erase(objectGuid);
}

void ViewportPanel::ReloadAnimations(uint64_t objectGuid, StaticObject* object) {
    if (!object) return;
    const std::string& modelPath = object->GetModelPath();
    if (modelPath.empty()) return;

    m_AnimatedModelCache.erase(modelPath);

    auto* animModel = GetOrLoadAnimatedModel(modelPath);
    if (!animModel) return;

    for (const auto& animPath : object->GetAnimationPaths()) {
        animModel->LoadAnimation(animPath);
    }

    InvalidateAnimator(objectGuid);
}

Onyx::AnimatedModel* ViewportPanel::GetOrLoadAnimatedModel(const std::string& path) {
    if (path.empty()) return nullptr;

    auto it = m_AnimatedModelCache.find(path);
    if (it != m_AnimatedModelCache.end()) return it->second.get();

    auto model = std::make_unique<Onyx::AnimatedModel>();
    if (!model->Load(path)) {
        return nullptr;
    }
    Onyx::AnimatedModel* ptr = model.get();
    m_AnimatedModelCache[path] = std::move(model);
    return ptr;
}

void ViewportPanel::RenderTerrain() {
    if (!m_TerrainShader) return;

    if (m_TerrainMaterialLibrary.IsArraysDirty()) {
        m_TerrainMaterialLibrary.RebuildTextureArrays();
    }

    m_TerrainShader->Bind();
    m_TerrainShader->SetMat4("u_View", m_ViewMatrix);
    m_TerrainShader->SetMat4("u_Projection", m_ProjectionMatrix);
    m_TerrainShader->SetVec3("u_LightDir", m_LightDir);
    m_TerrainShader->SetVec3("u_LightColor", m_EnableDirectionalLight ? m_LightColor : glm::vec3(0.0f));
    m_TerrainShader->SetVec3("u_ViewPos", m_CameraPosition);
    m_TerrainShader->SetFloat("u_AmbientStrength", m_AmbientStrength);

    m_TerrainShader->SetInt("u_Splatmap0", 1);
    m_TerrainShader->SetInt("u_Splatmap1", 2);

    m_TerrainShader->SetInt("u_DiffuseArray", 3);
    m_TerrainShader->SetInt("u_NormalArray", 4);
    m_TerrainShader->SetInt("u_RMAArray", 5);

    auto& lib = m_TerrainMaterialLibrary;
    if (lib.GetDiffuseArray()) lib.GetDiffuseArray()->Bind(3);
    if (lib.GetNormalArray()) lib.GetNormalArray()->Bind(4);
    if (lib.GetRMAArray()) lib.GetRMAArray()->Bind(5);

    m_TerrainShader->SetInt("u_ShadowMap", 6);
    m_TerrainShader->SetInt("u_EnableShadows", m_EnableShadows ? 1 : 0);
    m_TerrainShader->SetFloat("u_ShadowBias", m_ShadowBias);
    m_TerrainShader->SetInt("u_ShowCascades", m_ShowCascades ? 1 : 0);
    if (m_EnableShadows && m_CSM) {
        auto& matrices = m_CSM->GetLightSpaceMatrices();
        auto& splits = m_CSM->GetCascadeSplits();
        for (uint32_t i = 0; i < Onyx::NUM_SHADOW_CASCADES; i++) {
            m_TerrainShader->SetMat4("u_LightSpaceMatrices[" + std::to_string(i) + "]", matrices[i]);
            m_TerrainShader->SetFloat("u_CascadeSplits[" + std::to_string(i) + "]", splits[i]);
        }
        m_TerrainShader->SetMat4("u_View", m_ViewMatrix);
        Onyx::RenderCommand::BindTextureArray(6, m_CSM->GetDepthTextureArray());
    }

    m_TerrainShader->SetInt("u_ShowBrush", (m_TerrainTool.toolActive && m_TerrainTool.brushValid) ? 1 : 0);
    m_TerrainShader->SetVec3("u_BrushPos", m_TerrainTool.brushPos);
    m_TerrainShader->SetFloat("u_BrushRadius", m_TerrainTool.brushRadius);

    m_TerrainShader->SetInt("u_Heightmap", 0);
    m_TerrainShader->SetFloat("u_HeightmapTexelSize", 1.0f / (Editor3D::CHUNK_RESOLUTION + 2));
    m_TerrainShader->SetFloat("u_ChunkSize", Editor3D::CHUNK_SIZE);
    m_TerrainShader->SetInt("u_UsePixelNormals", m_TerrainTool.pixelNormals ? 1 : 0);
    m_TerrainShader->SetInt("u_EnablePBR", m_TerrainTool.pbr ? 1 : 0);
    m_TerrainShader->SetInt("u_DebugSplatmap", m_TerrainTool.debugSplatmap);

    glm::mat4 VP = m_ProjectionMatrix * m_ViewMatrix;
    m_TerrainSystem.Render(m_TerrainShader.get(), VP,
        [this](Editor3D::TerrainChunk* chunk, Onyx::Shader* shader) {
            auto& lib = m_TerrainMaterialLibrary;
            const auto& data = chunk->GetData();

            glm::vec3 origin = chunk->GetWorldPosition();
            shader->SetVec2("u_ChunkOrigin", origin.x, origin.z);

            int arrayIndices[Editor3D::MAX_TERRAIN_LAYERS];
            float tilingScales[Editor3D::MAX_TERRAIN_LAYERS];
            float normalStrengths[Editor3D::MAX_TERRAIN_LAYERS];
            for (int i = 0; i < Editor3D::MAX_TERRAIN_LAYERS; i++) {
                const auto& matId = data.materialIds[i];
                arrayIndices[i] = lib.GetMaterialArrayIndex(matId);
                const auto* mat = lib.GetMaterial(matId);
                tilingScales[i] = mat ? mat->tilingScale : 8.0f;
                normalStrengths[i] = mat ? mat->normalStrength : 1.0f;
            }
            shader->SetIntArray("u_LayerArrayIndex[0]", arrayIndices, Editor3D::MAX_TERRAIN_LAYERS);
            shader->SetFloatArray("u_LayerTiling[0]", tilingScales, Editor3D::MAX_TERRAIN_LAYERS);
            shader->SetFloatArray("u_LayerNormalStrength[0]", normalStrengths, Editor3D::MAX_TERRAIN_LAYERS);
        });
}

bool ViewportPanel::RaycastTerrain(const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3& hitPoint) {
    float stepSize = 0.5f;
    float maxDistance = 500.0f;

    for (float t = stepSize; t < maxDistance; t += stepSize) {
        glm::vec3 pos = rayOrigin + rayDir * t;
        float terrainHeight;
        bool hasChunk = m_TerrainSystem.GetHeightAt(pos.x, pos.z, terrainHeight);

        if (!hasChunk) terrainHeight = 0.0f;

        if (pos.y <= terrainHeight) {
            float lo = t - stepSize;
            float hi = t;
            for (int i = 0; i < 16; i++) {
                float mid = (lo + hi) * 0.5f;
                glm::vec3 midPos = rayOrigin + rayDir * mid;
                float midHeight;
                if (!m_TerrainSystem.GetHeightAt(midPos.x, midPos.z, midHeight))
                    midHeight = 0.0f;
                if (midPos.y <= midHeight) {
                    hi = mid;
                } else {
                    lo = mid;
                }
            }
            float finalT = (lo + hi) * 0.5f;
            hitPoint = rayOrigin + rayDir * finalT;
            float finalHeight;
            if (!m_TerrainSystem.GetHeightAt(hitPoint.x, hitPoint.z, finalHeight))
                finalHeight = 0.0f;
            hitPoint.y = finalHeight;
            return true;
        }
    }
    return false;
}

void ViewportPanel::HandleTerrainInput() {
    ImGuiIO& io = ImGui::GetIO();

    float mouseX = io.MousePos.x - m_ViewportPos.x;
    float mouseY = io.MousePos.y - m_ViewportPos.y;

    glm::vec3 rayDir = ScreenToWorldRay(mouseX, mouseY);
    glm::vec3 hitPoint;
    m_TerrainTool.brushValid = RaycastTerrain(m_CameraPosition, rayDir, hitPoint);
    if (m_TerrainTool.brushValid) {
        m_TerrainTool.brushPos = hitPoint;
    }

    if (m_TerrainTool.mode == TerrainToolMode::Ramp) {
        if (m_TerrainTool.rampPlacing && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            m_TerrainTool.rampPlacing = false;
            return;
        }
        if (m_TerrainTool.brushValid && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && !m_RightMouseDown && !m_MiddleMouseDown) {
            if (!m_TerrainTool.rampPlacing) {
                m_TerrainTool.rampPlacing = true;
                m_TerrainTool.rampStart = m_TerrainTool.brushPos;
                m_TerrainTool.rampStartHeight = m_TerrainTool.brushPos.y;
            } else {
                m_TerrainTool.rampPlacing = false;
                float endHeight = m_TerrainTool.brushPos.y;
                float rampWidth = m_TerrainTool.brushRadius;
                m_TerrainSystem.BeginEdit(
                    (m_TerrainTool.rampStart.x + m_TerrainTool.brushPos.x) * 0.5f,
                    (m_TerrainTool.rampStart.z + m_TerrainTool.brushPos.z) * 0.5f,
                    glm::length(glm::vec2(m_TerrainTool.brushPos.x - m_TerrainTool.rampStart.x,
                                          m_TerrainTool.brushPos.z - m_TerrainTool.rampStart.z)) + rampWidth);
                m_TerrainSystem.RampTerrain(
                    m_TerrainTool.rampStart.x, m_TerrainTool.rampStart.z, m_TerrainTool.rampStartHeight,
                    m_TerrainTool.brushPos.x, m_TerrainTool.brushPos.z, endHeight, rampWidth);
                m_TerrainSystem.EndEdit();
            }
        }
        return;
    }

    bool leftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    if (leftDown && m_TerrainTool.brushValid && !m_RightMouseDown && !m_MiddleMouseDown) {
        if (!m_TerrainPainting) {
            m_TerrainPainting = true;
            m_TerrainSystem.BeginEdit(m_TerrainTool.brushPos.x, m_TerrainTool.brushPos.z, m_TerrainTool.brushRadius);
        }

        float dt = io.DeltaTime;
        float amount = m_TerrainTool.brushStrength * dt;

        switch (m_TerrainTool.mode) {
            case TerrainToolMode::Raise:
                m_TerrainSystem.RaiseTerrain(m_TerrainTool.brushPos.x, m_TerrainTool.brushPos.z,
                    m_TerrainTool.brushRadius, amount);
                break;
            case TerrainToolMode::Lower:
                m_TerrainSystem.LowerTerrain(m_TerrainTool.brushPos.x, m_TerrainTool.brushPos.z,
                    m_TerrainTool.brushRadius, amount);
                break;
            case TerrainToolMode::Flatten:
                m_TerrainSystem.FlattenTerrain(m_TerrainTool.brushPos.x, m_TerrainTool.brushPos.z,
                    m_TerrainTool.brushRadius, m_TerrainTool.flattenHeight, m_TerrainTool.flattenHardness);
                break;
            case TerrainToolMode::Smooth:
                m_TerrainSystem.SmoothTerrain(m_TerrainTool.brushPos.x, m_TerrainTool.brushPos.z,
                    m_TerrainTool.brushRadius, amount);
                break;
            case TerrainToolMode::Paint:
                if (!m_TerrainTool.selectedMaterialId.empty()) {
                    m_TerrainSystem.PaintMaterial(m_TerrainTool.brushPos.x, m_TerrainTool.brushPos.z,
                        m_TerrainTool.brushRadius, m_TerrainTool.selectedMaterialId, amount);
                } else {
                    m_TerrainSystem.PaintTexture(m_TerrainTool.brushPos.x, m_TerrainTool.brushPos.z,
                        m_TerrainTool.brushRadius, m_TerrainTool.paintLayer, amount);
                }
                break;
            case TerrainToolMode::Hole:
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    m_TerrainSystem.SetHole(m_TerrainTool.brushPos.x, m_TerrainTool.brushPos.z, true);
                }
                break;
            case TerrainToolMode::Ramp:
                break;
        }
    }

    if (!leftDown && m_TerrainPainting) {
        m_TerrainPainting = false;
        m_TerrainSystem.EndEdit();
    }
}

} // namespace MMO
