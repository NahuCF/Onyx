#include "pch.h"
#include "SceneRenderer.h"
#include "RenderCommand.h"
#include <GL/glew.h>
#include <limits>
#include <cstdio>

namespace Onyx {

SceneRenderer::SceneRenderer() = default;
SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::Init(AssetManager* assetManager, const std::string& shaderBasePath) {
    m_AssetManager = assetManager;

    std::string base = shaderBasePath.empty() ? "Onyx/Assets/Shaders/" : shaderBasePath;

    m_StaticBatchedShader = std::make_unique<Shader>(
        (base + "model_batched.vert").c_str(),
        (base + "model.frag").c_str());

    m_SkinnedBatchedShader = std::make_unique<Shader>(
        (base + "skinned_batched.vert").c_str(),
        (base + "model.frag").c_str());

    m_StaticShadowShader = std::make_unique<Shader>(
        (base + "shadow_depth_batched.vert").c_str(),
        (base + "shadow_depth.frag").c_str());

    m_SkinnedShadowShader = std::make_unique<Shader>(
        (base + "shadow_depth_skinned_batched.vert").c_str(),
        (base + "shadow_depth.frag").c_str());

    m_StaticSSBO = std::make_unique<ShaderStorageBuffer>();
    m_StaticCmdBO = std::make_unique<DrawCommandBuffer>();
    m_SkinnedSSBO = std::make_unique<ShaderStorageBuffer>();
    m_BoneSSBO = std::make_unique<ShaderStorageBuffer>();
    m_SkinnedCmdBO = std::make_unique<DrawCommandBuffer>();

    m_CSM = std::make_unique<CascadedShadowMap>();
    m_CSM->Create(2048);
}

void SceneRenderer::Begin(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    m_View = view;
    m_Projection = projection;
    m_CameraPos = cameraPos;

    m_StaticBatches.clear();
    m_SkinnedQueue.clear();
    m_PointLights.clear();
    m_SpotLights.clear();
    m_Stats = {};
}

void SceneRenderer::SetDirectionalLight(const DirectionalLight& light) { m_DirLight = light; }
void SceneRenderer::SetAmbientStrength(float strength) { m_AmbientStrength = strength; }
void SceneRenderer::AddPointLight(const PointLightData& light) { m_PointLights.push_back(light); }
void SceneRenderer::AddSpotLight(const SpotLightData& light) { m_SpotLights.push_back(light); }

void SceneRenderer::SetShadowsEnabled(bool enabled) { m_ShadowsEnabled = enabled; }
void SceneRenderer::SetShadowBias(float bias) { m_ShadowBias = bias; }
void SceneRenderer::SetShadowDistance(float distance) { m_ShadowDistance = distance; }
void SceneRenderer::SetSplitLambda(float lambda) { m_SplitLambda = lambda; }
void SceneRenderer::SetShowCascades(bool show) { m_ShowCascades = show; }

void SceneRenderer::SetShadowMapSize(uint32_t resolution) {
    if (m_CSM) m_CSM->Resize(resolution);
}

void SceneRenderer::SubmitStatic(Model* model, uint32_t meshIndex,
                                  const glm::mat4& worldTransform,
                                  const std::string& albedoPath,
                                  const std::string& normalPath) {
    if (!model) return;
    if (!model->HasMergedBuffers()) model->BuildMergedBuffers();
    if (!model->HasMergedBuffers()) return;

    const auto& merged = model->GetMergedBuffers();
    if (meshIndex >= merged.meshInfos.size()) return;

    uint64_t key = MakeBatchKey(model, albedoPath, normalPath);

    auto& batch = m_StaticBatches[key];
    if (!batch.model) {
        batch.model = model;
        batch.albedoPath = albedoPath;
        batch.normalPath = normalPath;
    }

    batch.drawData.push_back({worldTransform});

    const auto& info = merged.meshInfos[meshIndex];
    DrawIndirectCommand cmd;
    cmd.count = info.indexCount;
    cmd.instanceCount = 1;
    cmd.firstIndex = info.firstIndex;
    cmd.baseVertex = info.baseVertex;
    cmd.baseInstance = 0;
    batch.commands.push_back(cmd);

    const auto& mesh = model->GetMeshes()[meshIndex];
    glm::vec3 wMin, wMax;
    TransformAABB(mesh.GetBoundsMin(), mesh.GetBoundsMax(), worldTransform, wMin, wMax);
    batch.bounds.push_back({wMin, wMax});

    batch.totalTriangles += info.indexCount / 3;
}

void SceneRenderer::SubmitSkinned(AnimatedModel* model,
                                   const glm::mat4& worldTransform,
                                   const std::vector<glm::mat4>& boneMatrices,
                                   const std::string& albedoPath,
                                   const std::string& normalPath) {
    if (!model) return;
    if (!model->HasMergedBuffers()) model->BuildMergedBuffers();
    if (!model->HasMergedBuffers()) return;

    m_SkinnedQueue.push_back({model, worldTransform, boneMatrices, albedoPath, normalPath});
}

void SceneRenderer::RenderShadows(ShadowCasterCallback extraShadows) {
    if (!m_ShadowsEnabled || !m_CSM) return;

    m_CSM->Update(m_View, m_Projection, m_DirLight.direction,
                   0.1f, m_ShadowDistance, m_SplitLambda);
    RenderShadowPass(extraShadows);
}

void SceneRenderer::RenderBatches() {
    m_Stats = {};
    RenderStaticPass();
    RenderSkinnedPass();
}

void SceneRenderer::UploadLightUniforms(Shader* shader) const {
    shader->SetVec3("u_LightDir", m_DirLight.direction);
    shader->SetVec3("u_LightColor", m_DirLight.enabled ? m_DirLight.color : glm::vec3(0.0f));
    shader->SetVec3("u_ViewPos", m_CameraPos);
    shader->SetFloat("u_AmbientStrength", m_AmbientStrength);

    int pointCount = static_cast<int>(std::min(m_PointLights.size(), size_t(32)));
    shader->SetInt("u_PointLightCount", pointCount);
    for (int i = 0; i < pointCount; i++) {
        std::string idx = std::to_string(i);
        shader->SetVec3(("u_PointLightPos[" + idx + "]").c_str(), m_PointLights[i].position);
        shader->SetVec3(("u_PointLightColor[" + idx + "]").c_str(), m_PointLights[i].color);
        shader->SetFloat(("u_PointLightRange[" + idx + "]").c_str(), m_PointLights[i].range);
    }

    int spotCount = static_cast<int>(std::min(m_SpotLights.size(), size_t(8)));
    shader->SetInt("u_SpotLightCount", spotCount);
    for (int i = 0; i < spotCount; i++) {
        std::string idx = std::to_string(i);
        shader->SetVec3(("u_SpotLightPos[" + idx + "]").c_str(), m_SpotLights[i].position);
        shader->SetVec3(("u_SpotLightDir[" + idx + "]").c_str(), m_SpotLights[i].direction);
        shader->SetVec3(("u_SpotLightColor[" + idx + "]").c_str(), m_SpotLights[i].color);
        shader->SetFloat(("u_SpotLightRange[" + idx + "]").c_str(), m_SpotLights[i].range);
        shader->SetFloat(("u_SpotLightInnerCos[" + idx + "]").c_str(), m_SpotLights[i].innerCos);
        shader->SetFloat(("u_SpotLightOuterCos[" + idx + "]").c_str(), m_SpotLights[i].outerCos);
    }
}

void SceneRenderer::UploadShadowUniforms(Shader* shader, int shadowTextureSlot) const {
    shader->SetInt("u_EnableShadows", m_ShadowsEnabled ? 1 : 0);
    shader->SetFloat("u_ShadowBias", m_ShadowBias);
    shader->SetInt("u_ShowCascades", m_ShowCascades ? 1 : 0);

    if (m_ShadowsEnabled && m_CSM) {
        auto& matrices = m_CSM->GetLightSpaceMatrices();
        auto& splits = m_CSM->GetCascadeSplits();
        for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++) {
            shader->SetMat4(("u_LightSpaceMatrices[" + std::to_string(i) + "]").c_str(), matrices[i]);
            shader->SetFloat(("u_CascadeSplits[" + std::to_string(i) + "]").c_str(), splits[i]);
        }
        RenderCommand::BindTextureArray(shadowTextureSlot, m_CSM->GetDepthTextureArray());
    }
}

void SceneRenderer::RenderShadowPass(const ShadowCasterCallback& extraShadows) {
    m_CSM->ClearAll();
    RenderCommand::EnableDepthTest();
    RenderCommand::DisableCulling();
    RenderCommand::EnablePolygonOffset(4.0f, 4.0f);

    std::unordered_map<uint64_t, SkinnedBatch> skinnedBatches;
    if (!m_SkinnedQueue.empty()) {
        BuildSkinnedBatches(skinnedBatches);
    }

    // Reusable scratch vectors — avoid per-batch heap allocation
    std::vector<StaticDrawData> culledData;
    std::vector<DrawIndirectCommand> culledCmds;

    for (uint32_t cascade = 0; cascade < NUM_SHADOW_CASCADES; cascade++) {
        m_CSM->BindCascade(cascade);
        const glm::mat4& lightSpaceMat = m_CSM->GetLightSpaceMatrix(cascade);

        glm::vec4 planes[6];
        ExtractFrustumPlanes(lightSpaceMat, planes);

        // Pre-compute per-plane sign masks for branchless AABB test
        glm::vec3 planeN[6];
        float planeD[6];
        glm::vec3 planeSign[6]; // 1 where normal > 0, 0 otherwise
        for (int i = 0; i < 6; i++) {
            planeN[i] = glm::vec3(planes[i]);
            planeD[i] = planes[i].w;
            planeSign[i] = glm::vec3(
                planes[i].x > 0.0f ? 1.0f : 0.0f,
                planes[i].y > 0.0f ? 1.0f : 0.0f,
                planes[i].z > 0.0f ? 1.0f : 0.0f);
        }

        m_StaticShadowShader->Bind();
        m_StaticShadowShader->SetMat4("u_LightSpaceMatrix", lightSpaceMat);

        for (auto& [key, batch] : m_StaticBatches) {
            culledData.clear();
            culledCmds.clear();

            for (size_t i = 0; i < batch.bounds.size(); i++) {
                const glm::vec3& bMin = batch.bounds[i].worldMin;
                const glm::vec3& bMax = batch.bounds[i].worldMax;
                const glm::vec3 extent = bMax - bMin;

                bool visible = true;
                for (int p = 0; p < 6; p++) {
                    // p-vertex = bMin + extent * sign (branchless select of min/max per component)
                    glm::vec3 pv = bMin + extent * planeSign[p];
                    if (glm::dot(planeN[p], pv) + planeD[p] < 0.0f) {
                        visible = false;
                        break;
                    }
                }

                if (visible) {
                    culledData.push_back(batch.drawData[i]);
                    culledCmds.push_back(batch.commands[i]);
                }
            }

            if (culledCmds.empty()) continue;

            m_StaticSSBO->Upload(culledData.data(),
                culledData.size() * sizeof(StaticDrawData), 0);
            m_StaticCmdBO->Upload(culledCmds.data(),
                culledCmds.size() * sizeof(DrawIndirectCommand));
            RenderCommand::DrawBatched(*batch.model->GetMergedBuffers().vao,
                static_cast<uint32_t>(culledCmds.size()));
        }

        m_StaticCmdBO->UnBind();
        m_StaticSSBO->UnBind();

        if (!skinnedBatches.empty()) {
            m_SkinnedShadowShader->Bind();
            m_SkinnedShadowShader->SetMat4("u_LightSpaceMatrix", lightSpaceMat);

            for (auto& [key, batch] : skinnedBatches) {
                m_BoneSSBO->Upload(batch.packedBones.data(),
                    batch.packedBones.size() * sizeof(glm::mat4), 1);
                m_SkinnedSSBO->Upload(batch.drawData.data(),
                    batch.drawData.size() * sizeof(SkinnedDrawData), 0);
                m_SkinnedCmdBO->Upload(batch.commands.data(),
                    batch.commands.size() * sizeof(DrawIndirectCommand));
                RenderCommand::DrawBatched(*batch.model->GetMergedBuffers().vao,
                    static_cast<uint32_t>(batch.commands.size()));
            }

            m_SkinnedCmdBO->UnBind();
            m_SkinnedSSBO->UnBind();
            m_BoneSSBO->UnBind();
        }

        if (extraShadows) {
            extraShadows(cascade, lightSpaceMat);
        }
    }

    m_CSM->UnBind();
    RenderCommand::DisablePolygonOffset();
}

void SceneRenderer::RenderStaticPass() {
    if (m_StaticBatches.empty()) return;

    m_StaticBatchedShader->Bind();
    m_StaticBatchedShader->SetMat4("u_View", m_View);
    m_StaticBatchedShader->SetMat4("u_Projection", m_Projection);
    m_StaticBatchedShader->SetInt("u_AlbedoMap", 0);
    m_StaticBatchedShader->SetInt("u_NormalMap", 1);
    m_StaticBatchedShader->SetInt("u_ShadowMap", 2);

    UploadLightUniforms(m_StaticBatchedShader.get());
    UploadShadowUniforms(m_StaticBatchedShader.get());

    for (auto& [key, batch] : m_StaticBatches) {
        Texture* albedo = m_AssetManager->ResolveTexture(batch.albedoPath);
        Texture* normal = m_AssetManager->ResolveTexture(batch.normalPath);
        (albedo ? albedo : m_AssetManager->GetDefaultAlbedo())->Bind(0);

        if (normal) {
            normal->Bind(1);
            m_StaticBatchedShader->SetInt("u_UseNormalMap", 1);
        } else {
            m_AssetManager->GetDefaultNormal()->Bind(1);
            m_StaticBatchedShader->SetInt("u_UseNormalMap", 0);
        }

        uint32_t drawCount = static_cast<uint32_t>(batch.commands.size());

        m_StaticSSBO->Upload(batch.drawData.data(),
            batch.drawData.size() * sizeof(StaticDrawData), 0);
        m_StaticCmdBO->Upload(batch.commands.data(),
            batch.commands.size() * sizeof(DrawIndirectCommand));

        RenderCommand::DrawBatched(*batch.model->GetMergedBuffers().vao, drawCount);

        m_Stats.batchedMeshCount += drawCount;
        m_Stats.batchedDrawCalls++;
        m_Stats.triangles += batch.totalTriangles;
    }

    m_StaticCmdBO->UnBind();
    m_StaticSSBO->UnBind();
    m_Stats.drawCalls += m_Stats.batchedDrawCalls;
}

void SceneRenderer::RenderSkinnedPass() {
    if (m_SkinnedQueue.empty()) return;

    std::unordered_map<uint64_t, SkinnedBatch> skinnedBatches;
    BuildSkinnedBatches(skinnedBatches);

    m_SkinnedBatchedShader->Bind();
    m_SkinnedBatchedShader->SetMat4("u_View", m_View);
    m_SkinnedBatchedShader->SetMat4("u_Projection", m_Projection);
    m_SkinnedBatchedShader->SetInt("u_AlbedoMap", 0);
    m_SkinnedBatchedShader->SetInt("u_NormalMap", 1);
    m_SkinnedBatchedShader->SetInt("u_ShadowMap", 2);

    UploadLightUniforms(m_SkinnedBatchedShader.get());
    UploadShadowUniforms(m_SkinnedBatchedShader.get());

    for (auto& [key, batch] : skinnedBatches) {
        Texture* albedo = m_AssetManager->ResolveTexture(batch.albedoPath);
        Texture* normal = m_AssetManager->ResolveTexture(batch.normalPath);

        if (!albedo && !batch.model->GetMaterials().empty()) {
            auto& mat = batch.model->GetMaterials()[0];
            if (mat.diffuseTexture) albedo = mat.diffuseTexture.get();
        }

        (albedo ? albedo : m_AssetManager->GetDefaultAlbedo())->Bind(0);

        if (normal) {
            normal->Bind(1);
            m_SkinnedBatchedShader->SetInt("u_UseNormalMap", 1);
        } else {
            m_AssetManager->GetDefaultNormal()->Bind(1);
            m_SkinnedBatchedShader->SetInt("u_UseNormalMap", 0);
        }

        m_BoneSSBO->Upload(batch.packedBones.data(),
            batch.packedBones.size() * sizeof(glm::mat4), 1);
        m_SkinnedSSBO->Upload(batch.drawData.data(),
            batch.drawData.size() * sizeof(SkinnedDrawData), 0);
        m_SkinnedCmdBO->Upload(batch.commands.data(),
            batch.commands.size() * sizeof(DrawIndirectCommand));

        RenderCommand::DrawBatched(*batch.model->GetMergedBuffers().vao,
            static_cast<uint32_t>(batch.commands.size()));

        m_Stats.skinnedDrawCalls++;
        m_Stats.skinnedInstances += static_cast<uint32_t>(batch.drawData.size());
        m_Stats.triangles += batch.totalTriangles;
    }

    m_SkinnedCmdBO->UnBind();
    m_SkinnedSSBO->UnBind();
    m_BoneSSBO->UnBind();
    m_Stats.drawCalls += m_Stats.skinnedDrawCalls;
}

void SceneRenderer::BuildSkinnedBatches(std::unordered_map<uint64_t, SkinnedBatch>& out) const {
    for (const auto& sub : m_SkinnedQueue) {
        const auto& merged = sub.model->GetMergedBuffers();

        uint64_t key = MakeBatchKey(sub.model, sub.albedoPath, sub.normalPath);

        auto& batch = out[key];
        if (!batch.model) {
            batch.model = sub.model;
            batch.albedoPath = sub.albedoPath;
            batch.normalPath = sub.normalPath;
        }

        int32_t boneOffset = static_cast<int32_t>(batch.packedBones.size());
        int32_t boneCount = static_cast<int32_t>(sub.boneMatrices.size());

        batch.packedBones.insert(batch.packedBones.end(),
            sub.boneMatrices.begin(), sub.boneMatrices.end());

        for (uint32_t meshIdx = 0; meshIdx < merged.meshInfos.size(); meshIdx++) {
            const auto& info = merged.meshInfos[meshIdx];

            SkinnedDrawData dd;
            dd.model = sub.transform;
            dd.boneOffset = boneOffset;
            dd.boneCount = boneCount;
            dd._pad[0] = 0;
            dd._pad[1] = 0;
            batch.drawData.push_back(dd);

            DrawIndirectCommand cmd;
            cmd.count = info.indexCount;
            cmd.instanceCount = 1;
            cmd.firstIndex = info.firstIndex;
            cmd.baseVertex = info.baseVertex;
            cmd.baseInstance = 0;
            batch.commands.push_back(cmd);

            batch.totalTriangles += info.indexCount / 3;
        }
    }
}

void SceneRenderer::TransformAABB(const glm::vec3& localMin, const glm::vec3& localMax,
                                   const glm::mat4& transform,
                                   glm::vec3& worldMin, glm::vec3& worldMax) {
    // Arvo's method: compute transformed AABB from matrix columns directly
    // instead of transforming all 8 corners (8 mat*vec -> 3 col*scalar pairs)
    glm::vec3 t(transform[3]);
    worldMin = t;
    worldMax = t;

    for (int i = 0; i < 3; i++) {
        glm::vec3 col(transform[i][0], transform[i][1], transform[i][2]);
        glm::vec3 a = col * localMin[i];
        glm::vec3 b = col * localMax[i];
        worldMin += glm::min(a, b);
        worldMax += glm::max(a, b);
    }
}

void SceneRenderer::ExtractFrustumPlanes(const glm::mat4& vp, glm::vec4 planes[6]) {
    planes[0] = glm::vec4(vp[0][3]+vp[0][0], vp[1][3]+vp[1][0], vp[2][3]+vp[2][0], vp[3][3]+vp[3][0]);
    planes[1] = glm::vec4(vp[0][3]-vp[0][0], vp[1][3]-vp[1][0], vp[2][3]-vp[2][0], vp[3][3]-vp[3][0]);
    planes[2] = glm::vec4(vp[0][3]+vp[0][1], vp[1][3]+vp[1][1], vp[2][3]+vp[2][1], vp[3][3]+vp[3][1]);
    planes[3] = glm::vec4(vp[0][3]-vp[0][1], vp[1][3]-vp[1][1], vp[2][3]-vp[2][1], vp[3][3]-vp[3][1]);
    planes[4] = glm::vec4(vp[0][3]+vp[0][2], vp[1][3]+vp[1][2], vp[2][3]+vp[2][2], vp[3][3]+vp[3][2]);
    planes[5] = glm::vec4(vp[0][3]-vp[0][2], vp[1][3]-vp[1][2], vp[2][3]-vp[2][2], vp[3][3]-vp[3][2]);

    for (int i = 0; i < 6; i++) {
        float len = glm::length(glm::vec3(planes[i]));
        if (len > 0.0f) planes[i] /= len;
    }
}

bool SceneRenderer::AABBInFrustum(const glm::vec3& min, const glm::vec3& max,
                                   const glm::vec4 planes[6]) {
    for (int i = 0; i < 6; i++) {
        glm::vec3 p(
            planes[i].x > 0 ? max.x : min.x,
            planes[i].y > 0 ? max.y : min.y,
            planes[i].z > 0 ? max.z : min.z
        );
        if (glm::dot(glm::vec3(planes[i]), p) + planes[i].w < 0.0f)
            return false;
    }
    return true;
}

uint64_t SceneRenderer::MakeBatchKey(const void* ptr,
                                      const std::string& albedo,
                                      const std::string& normal) {
    uint64_t h = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ptr));
    // FNV-1a inspired combine — mix in string hashes without any allocation
    auto mix = [](uint64_t seed, uint64_t val) -> uint64_t {
        seed ^= val + 0x9e3779b97f4a7c15ULL + (seed << 12) + (seed >> 4);
        return seed;
    };
    std::hash<std::string> strHash;
    h = mix(h, strHash(albedo));
    h = mix(h, strHash(normal));
    return h;
}

} // namespace Onyx
