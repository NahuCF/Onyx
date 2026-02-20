#pragma once

#include "Buffers.h"
#include "Shader.h"
#include "CascadedShadowMap.h"
#include "Frustum.h"
#include "AssetManager.h"
#include "Model.h"
#include "AnimatedModel.h"
#include "Framebuffer.h"
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

namespace Onyx {

struct StaticDrawData {
    glm::mat4 model;
};

struct SkinnedDrawData {
    glm::mat4 model;
    int32_t boneOffset;
    int32_t boneCount;
    int32_t _pad[2];
};

struct DrawIndirectCommand {
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t  baseVertex;
    uint32_t baseInstance;
};

struct MeshBounds {
    glm::vec3 worldMin;
    glm::vec3 worldMax;
};

struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t triangles = 0;
    uint32_t batchedDrawCalls = 0;
    uint32_t batchedMeshCount = 0;
    uint32_t skinnedDrawCalls = 0;
    uint32_t skinnedInstances = 0;
    uint32_t meshesSubmitted = 0;
    uint32_t meshesCulled = 0;
};

struct DirectionalLight {
    glm::vec3 direction = glm::vec3(-0.5f, -1.0f, -0.3f);
    glm::vec3 color = glm::vec3(1.0f);
    bool enabled = true;
};

struct PointLightData {
    glm::vec3 position;
    glm::vec3 color;
    float range;
};

struct SpotLightData {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float range;
    float innerCos;
    float outerCos;
};

class SceneRenderer {
public:
    using ShadowCasterCallback = std::function<void(uint32_t cascade, const glm::mat4& lightSpaceMat)>;

    SceneRenderer();
    ~SceneRenderer();

    void Init(AssetManager* assetManager, const std::string& shaderBasePath = "");

    void Begin(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);

    void SetDirectionalLight(const DirectionalLight& light);
    void SetAmbientStrength(float strength);
    void AddPointLight(const PointLightData& light);
    void AddSpotLight(const SpotLightData& light);

    void SetShadowsEnabled(bool enabled);
    void SetShadowBias(float bias);
    void SetShadowDistance(float distance);
    void SetShadowMapSize(uint32_t resolution);
    void SetSplitLambda(float lambda);
    void SetShowCascades(bool show);

    void SubmitStatic(Model* model, uint32_t meshIndex,
                      const glm::mat4& worldTransform,
                      const std::string& albedoPath,
                      const std::string& normalPath);

    void SubmitSkinned(AnimatedModel* model,
                       const glm::mat4& worldTransform,
                       const std::vector<glm::mat4>& boneMatrices,
                       const std::string& albedoPath = "",
                       const std::string& normalPath = "");

    void RenderShadows(ShadowCasterCallback extraShadows = nullptr);
    void RenderBatches();

    const CascadedShadowMap* GetCSM() const { return m_CSM.get(); }
    void UploadShadowUniforms(Shader* shader, int shadowTextureSlot = 2) const;
    void UploadLightUniforms(Shader* shader) const;

    RenderStats GetStats() const { return m_Stats; }

private:
    struct StaticBatch {
        Model* model = nullptr;
        std::string albedoPath;
        std::string normalPath;
        std::vector<StaticDrawData> drawData;
        std::vector<DrawIndirectCommand> commands;
        std::vector<MeshBounds> bounds;
        uint32_t totalTriangles = 0;
    };

    struct SkinnedSubmission {
        AnimatedModel* model = nullptr;
        glm::mat4 transform;
        std::vector<glm::mat4> boneMatrices;
        std::string albedoPath;
        std::string normalPath;
    };

    struct SkinnedBatch {
        AnimatedModel* model = nullptr;
        std::string albedoPath;
        std::string normalPath;
        std::vector<SkinnedDrawData> drawData;
        std::vector<DrawIndirectCommand> commands;
        std::vector<glm::mat4> packedBones;
        uint32_t totalTriangles = 0;
    };

    void RenderShadowPass(const ShadowCasterCallback& extraShadows);
    void RenderStaticPass();
    void RenderSkinnedPass();

    void BuildSkinnedBatches(std::unordered_map<uint64_t, SkinnedBatch>& out) const;

    static void TransformAABB(const glm::vec3& localMin, const glm::vec3& localMax,
                              const glm::mat4& transform,
                              glm::vec3& worldMin, glm::vec3& worldMax);

    static uint64_t MakeBatchKey(const void* ptr,
                                 const std::string& albedo,
                                 const std::string& normal);

    AssetManager* m_AssetManager = nullptr;

    std::unique_ptr<Shader> m_StaticBatchedShader;
    std::unique_ptr<Shader> m_SkinnedBatchedShader;
    std::unique_ptr<Shader> m_StaticShadowShader;
    std::unique_ptr<Shader> m_SkinnedShadowShader;

    std::unique_ptr<ShaderStorageBuffer> m_StaticSSBO;
    std::unique_ptr<DrawCommandBuffer> m_StaticCmdBO;
    std::unique_ptr<ShaderStorageBuffer> m_SkinnedSSBO;
    std::unique_ptr<ShaderStorageBuffer> m_BoneSSBO;
    std::unique_ptr<DrawCommandBuffer> m_SkinnedCmdBO;

    std::unique_ptr<CascadedShadowMap> m_CSM;
    bool m_ShadowsEnabled = true;
    float m_ShadowBias = 0.005f;
    float m_ShadowDistance = 60.0f;
    float m_SplitLambda = 0.0f;
    bool m_ShowCascades = false;

    glm::mat4 m_View = glm::mat4(1.0f);
    glm::mat4 m_Projection = glm::mat4(1.0f);
    glm::vec3 m_CameraPos = glm::vec3(0.0f);

    DirectionalLight m_DirLight;
    float m_AmbientStrength = 0.3f;
    std::vector<PointLightData> m_PointLights;
    std::vector<SpotLightData> m_SpotLights;

    Frustum m_CameraFrustum;

    std::unordered_map<uint64_t, StaticBatch> m_StaticBatches;
    std::vector<SkinnedSubmission> m_SkinnedQueue;

    RenderStats m_Stats;
};

} // namespace Onyx
