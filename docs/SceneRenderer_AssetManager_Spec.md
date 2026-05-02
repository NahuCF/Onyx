# SceneRenderer & AssetManager — Implementation Specification

> **Historical implementation plan from 2026-02-17.** Mostly implemented as described, but the public API has drifted. **For current public API and behavior, prefer [engine-rendering.md](engine-rendering.md).** Use this document for design intent and rationale only.
>
> **Known drifts vs. current code (verify before relying on):**
> 1. **No `Flush()` method.** The spec's combined `Flush(target, clearColor)` was split into separate `RenderShadows(callback)` and `RenderBatches()` calls. Framebuffer management lives in the caller (e.g., `ViewportPanel`, `GameRenderer`), not inside `SceneRenderer`.
> 2. **`SkinnedSubmission::boneMatrices` is a `std::vector<glm::mat4>` (copy), not a pointer.** Bone matrices are copied per submission rather than referenced.
> 3. **Async loading API is richer than documented.** `AssetManager` adds `RequestModelAsync()`, `ProcessGPUUploads()`, `GetModelStatus()`, plus staged GPU upload for >4 MB models. Not in the original spec.
> 4. **MMO Client did not migrate to `SceneRenderer`.** It uses its own `GameRenderer` with a `RuntimeModel` cache for `.omdl` static objects (see [mmogame-client.md](mmogame-client.md)). Editor3D does use `Onyx::SceneRenderer`.
> 5. Comments tagged `// future:` in this document are mostly already implemented.

This document describes the full implementation plan for extracting the indirect rendering
pipeline from `ViewportPanel` into a reusable `Onyx::SceneRenderer`, and introducing a
handle-based `Onyx::AssetManager` to replace scattered caches.

---

## Table of Contents

1. [Goals](#1-goals)
2. [AssetManager](#2-assetmanager)
3. [SceneRenderer](#3-scenerenderer)
4. [Shader Changes](#4-shader-changes)
5. [AnimatedModel Changes](#5-animatedmodel-changes)
6. [Migration — Editor3D ViewportPanel](#6-migration--editor3d-viewportpanel)
7. [Migration — MMO Client](#7-migration--mmo-client)
8. [File Layout](#8-file-layout)
9. [Implementation Order](#9-implementation-order)

---

## 1. Goals

- **Reusability**: Editor3D and MMO Client (and future apps) share the same render pipeline.
- **Option A skinned rendering**: Animated models use bone SSBOs + `glMultiDrawElementsIndirect`,
  same pattern as static meshes — no per-instance uniform uploads.
- **Centralized asset ownership**: One `AssetManager` owns all Models, AnimatedModels, Textures,
  and Materials. Consumers hold lightweight handles.
- **Minimal engine API surface**: Submit renderables, call Flush, get pixels on screen.

---

## 2. AssetManager

### 2.1 Handle Type

```cpp
// Onyx/Source/Graphics/AssetHandle.h

namespace Onyx {

template<typename T>
struct AssetHandle {
    uint32_t id = 0;
    bool IsValid() const { return id != 0; }
    bool operator==(const AssetHandle& o) const { return id == o.id; }
    bool operator!=(const AssetHandle& o) const { return id != o.id; }
};

// Convenience typedefs
using ModelHandle         = AssetHandle<Model>;
using AnimatedModelHandle = AssetHandle<AnimatedModel>;
using TextureHandle       = AssetHandle<Texture>;

} // namespace Onyx

// Hash support for use as map key
namespace std {
template<typename T>
struct hash<Onyx::AssetHandle<T>> {
    size_t operator()(const Onyx::AssetHandle<T>& h) const { return std::hash<uint32_t>()(h.id); }
};
}
```

Handles are 4 bytes, trivially copyable, safe to store in scene objects and serialization.

### 2.2 AssetManager Class

```cpp
// Onyx/Source/Graphics/AssetManager.h

namespace Onyx {

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    // --- Models (static) ---
    ModelHandle LoadModel(const std::string& path, bool loadTextures = false);
    Model*      Get(ModelHandle handle);
    bool        IsLoaded(ModelHandle handle) const;

    // --- Animated Models ---
    AnimatedModelHandle LoadAnimatedModel(const std::string& path);
    AnimatedModel*      Get(AnimatedModelHandle handle);
    bool                IsLoaded(AnimatedModelHandle handle) const;

    // --- Textures ---
    TextureHandle LoadTexture(const std::string& path);
    Texture*      Get(TextureHandle handle);
    bool          IsLoaded(TextureHandle handle) const;

    // --- Materials (metadata, not GPU resources) ---
    Material&       CreateMaterial(const std::string& id, const std::string& name = "");
    Material*       GetMaterial(const std::string& id);
    const Material* GetMaterial(const std::string& id) const;
    bool            HasMaterial(const std::string& id) const;
    bool            RemoveMaterial(const std::string& id);
    std::vector<std::string> GetAllMaterialIds() const;

    // --- Defaults ---
    Texture* GetDefaultAlbedo() const;
    Texture* GetDefaultNormal() const;

    // --- Texture resolution (convenience, resolves path -> GPU texture) ---
    // Returns raw pointer for immediate binding. Uses internal cache.
    Texture* ResolveTexture(const std::string& path);

    // --- Lifecycle ---
    // Drop assets not referenced by any live handle (future: refcount-based)
    void UnloadUnused();

    // Force-reload an asset (hot-reload in editor). Handle stays valid, data replaced.
    void Reload(ModelHandle handle);
    void Reload(AnimatedModelHandle handle);
    void Reload(TextureHandle handle);

private:
    uint32_t m_NextId = 1;

    // Path -> handle deduplication
    std::unordered_map<std::string, ModelHandle>         m_ModelPaths;
    std::unordered_map<std::string, AnimatedModelHandle> m_AnimModelPaths;
    std::unordered_map<std::string, TextureHandle>       m_TexturePaths;

    // Handle -> asset storage
    std::unordered_map<uint32_t, std::unique_ptr<Model>>         m_Models;
    std::unordered_map<uint32_t, std::unique_ptr<AnimatedModel>> m_AnimModels;
    std::unordered_map<uint32_t, std::unique_ptr<Texture>>       m_Textures;

    // Materials (keyed by user-defined ID, not handles — they're just metadata structs)
    std::unordered_map<std::string, Material> m_Materials;

    // Defaults
    std::unique_ptr<Texture> m_DefaultAlbedo;   // 1x1 white
    std::unique_ptr<Texture> m_DefaultNormal;   // 1x1 flat (128,128,255)
};

} // namespace Onyx
```

### 2.3 Implementation Notes

**LoadModel**:
```cpp
ModelHandle AssetManager::LoadModel(const std::string& path, bool loadTextures) {
    auto it = m_ModelPaths.find(path);
    if (it != m_ModelPaths.end()) return it->second;

    auto model = std::make_unique<Model>(path.c_str(), loadTextures);
    // Model constructor loads via Assimp. If it fails, model has 0 meshes.

    ModelHandle handle;
    handle.id = m_NextId++;
    m_Models[handle.id] = std::move(model);
    m_ModelPaths[path] = handle;
    return handle;
}
```

- Same pattern for `LoadAnimatedModel` and `LoadTexture`.
- `ResolveTexture(path)` is a convenience that calls `LoadTexture` then `Get`, returning raw ptr.
  This replaces `MaterialLibrary::ResolveTexture`.
- `Reload(handle)` finds the path from handle, destroys the old asset, creates a new one at
  the same handle ID. All handles remain valid — `Get()` returns the new data.
- Failed loads store `nullptr` in the map (same as current behavior) to avoid retrying.

### 2.4 What Gets Replaced

| Current location                              | Replaced by                |
|-----------------------------------------------|----------------------------|
| `ViewportPanel::m_ModelCache`                 | `AssetManager::m_Models`   |
| `ViewportPanel::m_AnimatedModelCache`         | `AssetManager::m_AnimModels` |
| `MaterialLibrary::m_TextureCache`             | `AssetManager::m_Textures` |
| `MaterialLibrary::m_Materials`                | `AssetManager::m_Materials`|
| `MaterialLibrary::m_DefaultAlbedo/Normal`     | `AssetManager` defaults    |
| `ViewportPanel::GetOrLoadModel()`             | `AssetManager::LoadModel()`|
| `ViewportPanel::GetOrLoadAnimatedModel()`     | `AssetManager::LoadAnimatedModel()` |
| `ViewportPanel::GetOrLoadTexture()`           | `AssetManager::ResolveTexture()` |

`MaterialLibrary` can be deleted entirely — its functionality is fully absorbed.

### 2.5 What Stays Separate

- `Animator` instances (per-object runtime state) stay owned by whoever manages instances.
  In Editor3D: `ViewportPanel::m_AnimatorCache` (keyed by objectGuid).
  In Client: the entity/player system.
- `TerrainMaterialLibrary` stays separate — it manages terrain-specific texture arrays.

---

## 3. SceneRenderer

### 3.1 GPU Data Structures

```cpp
// Onyx/Source/Graphics/SceneRenderer.h

namespace Onyx {

// Per-draw data for STATIC models — stored in SSBO binding 0
struct StaticDrawData {
    glm::mat4 model;  // 64 bytes, std430 aligned
};

// Per-draw data for SKINNED models — stored in SSBO binding 0
struct SkinnedDrawData {
    glm::mat4 model;      // 64 bytes
    int32_t   boneOffset; // offset into bone SSBO (in # of mat4s)
    int32_t   boneCount;  // number of bones for this instance
    int32_t   _pad[2];    // pad to 16-byte alignment (std430)
};  // 80 bytes total

// Standard OpenGL indirect draw command
struct DrawIndirectCommand {
    uint32_t count;         // index count
    uint32_t instanceCount; // always 1
    uint32_t firstIndex;    // into merged EBO
    int32_t  baseVertex;    // added to indices
    uint32_t baseInstance;  // always 0
};

// AABB for frustum culling
struct MeshBounds {
    glm::vec3 worldMin;
    glm::vec3 worldMax;
};

// Render statistics returned by GetStats()
struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t triangles = 0;
    uint32_t batchedDrawCalls = 0;   // glMultiDrawElementsIndirect calls
    uint32_t batchedMeshCount = 0;   // individual meshes inside batched calls
    uint32_t skinnedDrawCalls = 0;
    uint32_t skinnedInstances = 0;
};

} // namespace Onyx
```

### 3.2 Internal Batch Types

```cpp
// Private to SceneRenderer implementation

// One batch = one merged VAO (one model) + one texture pair
struct StaticBatch {
    Model* model = nullptr;
    std::string albedoPath;
    std::string normalPath;
    std::vector<StaticDrawData> drawData;
    std::vector<DrawIndirectCommand> commands;
    std::vector<MeshBounds> bounds;
    uint32_t totalTriangles = 0;
};

// One skinned submission = one animated model instance
struct SkinnedSubmission {
    AnimatedModel* model = nullptr;
    glm::mat4 transform;
    const std::vector<glm::mat4>* boneMatrices;  // ptr to Animator output
    std::string albedoPath;
    std::string normalPath;
};
```

### 3.3 Light Data

```cpp
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
```

### 3.4 SceneRenderer Class

```cpp
// Onyx/Source/Graphics/SceneRenderer.h

namespace Onyx {

class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();

    // Must be called once after construction, loads shaders and creates GPU buffers.
    // shaderBasePath: directory containing the engine shaders
    //   e.g., "Onyx/Assets/Shaders/" or "MMOGame/Editor3D/assets/shaders/"
    //   (see Section 4 for which shaders are needed)
    void Init(AssetManager* assetManager, const std::string& shaderBasePath = "");

    // ========================
    //  Per-frame API
    // ========================

    // Begin a new frame. Clears all submission queues.
    void Begin(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);

    // --- Lighting ---
    void SetDirectionalLight(const DirectionalLight& light);
    void SetAmbientStrength(float strength);
    void AddPointLight(const PointLightData& light);
    void AddSpotLight(const SpotLightData& light);

    // --- Shadow configuration ---
    void SetShadowsEnabled(bool enabled);
    void SetShadowBias(float bias);
    void SetShadowDistance(float distance);
    void SetShadowMapSize(uint32_t resolution);
    void SetSplitLambda(float lambda);
    void SetShowCascades(bool show);

    // --- Static model submission ---
    // Submit a single mesh of a static model.
    void SubmitStatic(Model* model, uint32_t meshIndex,
                      const glm::mat4& worldTransform,
                      const std::string& albedoPath,
                      const std::string& normalPath);

    // Convenience: submit ALL meshes of a model with a single material.
    // Internally calls SubmitStatic per mesh using MergedMeshInfo.
    void SubmitStaticModel(Model* model, const glm::mat4& worldTransform,
                           const std::string& materialId = "");

    // --- Skinned model submission ---
    // Submit an animated model instance with its current bone pose.
    void SubmitSkinned(AnimatedModel* model,
                       const glm::mat4& worldTransform,
                       const std::vector<glm::mat4>& boneMatrices,
                       const std::string& albedoPath = "",
                       const std::string& normalPath = "");

    // ========================
    //  Flush (execute all passes)
    // ========================

    // target: Framebuffer to render into. nullptr = backbuffer (default FBO).
    // clearColor: if provided, clears the target before rendering.
    //
    // Internally executes:
    //   1. Shadow pass (CSM, all cascades)
    //   2. Opaque static pass (indirect batched)
    //   3. Opaque skinned pass (indirect batched)
    //
    // After Flush, submission queues are consumed. Call Begin() again next frame.
    void Flush(Framebuffer* target = nullptr,
               const glm::vec4& clearColor = glm::vec4(0.15f, 0.15f, 0.18f, 1.0f));

    // ========================
    //  Post-flush overlays
    // ========================

    // Called AFTER Flush, while the same framebuffer is still bound.
    // Useful for terrain, debug overlays, etc. that the consumer renders themselves.
    // SceneRenderer leaves depth buffer intact so overlays depth-test correctly.

    // Get shadow data for external shadow-receiving shaders (terrain, etc.)
    const CascadedShadowMap* GetCSM() const;
    void UploadShadowUniforms(Shader* shader) const;
    void UploadLightUniforms(Shader* shader) const;

    // ========================
    //  Stats & Config
    // ========================

    RenderStats GetStats() const;

private:
    AssetManager* m_AssetManager = nullptr;

    // --- Shaders ---
    std::unique_ptr<Shader> m_StaticBatchedShader;      // model_batched.vert + model.frag
    std::unique_ptr<Shader> m_SkinnedBatchedShader;     // skinned_batched.vert + model.frag
    std::unique_ptr<Shader> m_StaticShadowShader;       // shadow_depth_batched.vert + shadow_depth.frag
    std::unique_ptr<Shader> m_SkinnedShadowShader;      // shadow_depth_skinned_batched.vert + shadow_depth.frag

    // --- GPU Buffers ---
    std::unique_ptr<ShaderStorageBuffer> m_StaticSSBO;   // binding 0: StaticDrawData[]
    std::unique_ptr<DrawCommandBuffer>   m_StaticCmdBO;

    std::unique_ptr<ShaderStorageBuffer> m_SkinnedSSBO;  // binding 0: SkinnedDrawData[]
    std::unique_ptr<ShaderStorageBuffer> m_BoneSSBO;     // binding 1: mat4[] (packed bones)
    std::unique_ptr<DrawCommandBuffer>   m_SkinnedCmdBO;

    // --- Shadow ---
    std::unique_ptr<CascadedShadowMap> m_CSM;
    bool     m_ShadowsEnabled = true;
    float    m_ShadowBias = 0.0f;
    float    m_ShadowDistance = 60.0f;
    float    m_SplitLambda = 0.0f;
    bool     m_ShowCascades = false;

    // --- Per-frame state ---
    glm::mat4 m_View;
    glm::mat4 m_Projection;
    glm::vec3 m_CameraPos;

    DirectionalLight m_DirLight;
    float m_AmbientStrength = 0.3f;
    std::vector<PointLightData> m_PointLights;
    std::vector<SpotLightData> m_SpotLights;

    // --- Submission queues ---
    std::unordered_map<std::string, StaticBatch> m_StaticBatches;
    std::vector<SkinnedSubmission> m_SkinnedQueue;

    // --- Stats ---
    mutable RenderStats m_Stats;

    // --- Internal methods ---
    void BuildStaticBatches();     // (already done during Submit calls)
    void RenderShadowPass();
    void RenderStaticPass();
    void RenderSkinnedPass();

    // Shadow frustum culling
    void FrustumCullBatch(const glm::mat4& lightSpaceMat,
                          const StaticBatch& batch,
                          std::vector<StaticDrawData>& outData,
                          std::vector<DrawIndirectCommand>& outCmds) const;

    // Upload light uniforms to a shader
    void UploadLightUniformsInternal(Shader* shader) const;
    void UploadShadowUniformsInternal(Shader* shader) const;

    // Batch key generation
    static std::string MakeBatchKey(const std::string& modelPath,
                                    const std::string& albedo,
                                    const std::string& normal);
};

} // namespace Onyx
```

### 3.5 Submit Flow (Static)

```cpp
void SceneRenderer::SubmitStatic(Model* model, uint32_t meshIndex,
                                 const glm::mat4& worldTransform,
                                 const std::string& albedoPath,
                                 const std::string& normalPath) {
    // Ensure merged buffers exist
    if (!model->HasMergedBuffers()) {
        model->BuildMergedBuffers();
    }
    if (!model->HasMergedBuffers()) return;

    const auto& merged = model->GetMergedBuffers();
    if (meshIndex >= merged.meshInfos.size()) return;

    // Batch key = pointer-as-string + textures
    // Using model pointer ensures same model with same textures shares one batch
    std::string key = MakeBatchKey(
        std::to_string(reinterpret_cast<uintptr_t>(model)),
        albedoPath, normalPath);

    auto& batch = m_StaticBatches[key];
    if (!batch.model) {
        batch.model = model;
        batch.albedoPath = albedoPath;
        batch.normalPath = normalPath;
    }

    // Per-draw data
    batch.drawData.push_back({worldTransform});

    // Indirect command from merged mesh info
    const auto& info = merged.meshInfos[meshIndex];
    DrawIndirectCommand cmd;
    cmd.count = info.indexCount;
    cmd.instanceCount = 1;
    cmd.firstIndex = info.firstIndex;
    cmd.baseVertex = info.baseVertex;
    cmd.baseInstance = 0;
    batch.commands.push_back(cmd);

    // World AABB for shadow culling
    const auto& mesh = model->GetMeshes()[meshIndex];
    glm::vec3 wMin, wMax;
    TransformAABB(mesh.GetBoundsMin(), mesh.GetBoundsMax(), worldTransform, wMin, wMax);
    batch.bounds.push_back({wMin, wMax});

    batch.totalTriangles += info.indexCount / 3;
}
```

### 3.6 Submit Flow (Skinned)

```cpp
void SceneRenderer::SubmitSkinned(AnimatedModel* model,
                                  const glm::mat4& worldTransform,
                                  const std::vector<glm::mat4>& boneMatrices,
                                  const std::string& albedoPath,
                                  const std::string& normalPath) {
    // Ensure merged buffers (see Section 5)
    if (!model->HasMergedBuffers()) {
        model->BuildMergedBuffers();
    }
    if (!model->HasMergedBuffers()) return;

    m_SkinnedQueue.push_back({
        model, worldTransform, &boneMatrices, albedoPath, normalPath
    });
}
```

### 3.7 Flush — Render Order

```cpp
void SceneRenderer::Flush(Framebuffer* target, const glm::vec4& clearColor) {
    m_Stats = {};

    // 1. Shadow pass
    if (m_ShadowsEnabled && m_CSM) {
        m_CSM->Update(m_View, m_Projection, m_DirLight.direction,
                       0.1f, m_ShadowDistance, m_SplitLambda);
        RenderShadowPass();
    }

    // 2. Bind render target
    if (target) {
        target->Bind();
        target->Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    } else {
        // Bind default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    RenderCommand::EnableDepthTest();
    RenderCommand::SetDepthMask(true);
    RenderCommand::EnableCulling();

    // 3. Opaque static batches
    RenderStaticPass();

    // 4. Opaque skinned instances
    RenderSkinnedPass();

    // (Framebuffer left bound — consumer can render terrain, overlays, etc.)
    // (Depth buffer intact for correct depth testing)
}
```

### 3.8 RenderStaticPass

```cpp
void SceneRenderer::RenderStaticPass() {
    if (m_StaticBatches.empty()) return;

    m_StaticBatchedShader->Bind();
    m_StaticBatchedShader->SetMat4("u_View", m_View);
    m_StaticBatchedShader->SetMat4("u_Projection", m_Projection);
    m_StaticBatchedShader->SetVec3("u_ViewPos", m_CameraPos);
    m_StaticBatchedShader->SetInt("u_AlbedoMap", 0);
    m_StaticBatchedShader->SetInt("u_NormalMap", 1);
    m_StaticBatchedShader->SetInt("u_ShadowMap", 2);

    UploadLightUniformsInternal(m_StaticBatchedShader.get());
    UploadShadowUniformsInternal(m_StaticBatchedShader.get());

    if (m_ShadowsEnabled && m_CSM) {
        RenderCommand::BindTextureArray(2, m_CSM->GetDepthTextureArray());
    }

    for (auto& [key, batch] : m_StaticBatches) {
        // Bind textures
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

        // Upload SSBO + draw commands
        m_StaticSSBO->Upload(batch.drawData.data(),
            batch.drawData.size() * sizeof(StaticDrawData), 0);
        m_StaticCmdBO->Upload(batch.commands.data(),
            batch.commands.size() * sizeof(DrawIndirectCommand));

        // Draw
        RenderCommand::DrawBatched(*batch.model->GetMergedBuffers().vao,
            static_cast<uint32_t>(batch.commands.size()));

        m_Stats.batchedDrawCalls++;
        m_Stats.batchedMeshCount += batch.commands.size();
        m_Stats.triangles += batch.totalTriangles;
    }

    m_StaticCmdBO->UnBind();
    m_StaticSSBO->UnBind();
    m_Stats.drawCalls += m_Stats.batchedDrawCalls;
}
```

### 3.9 RenderSkinnedPass

This is the core of Option A — bone SSBO approach:

```cpp
void SceneRenderer::RenderSkinnedPass() {
    if (m_SkinnedQueue.empty()) return;

    // -----------------------------------------------
    // Step 1: Group skinned submissions by (model + texture)
    // -----------------------------------------------
    struct SkinnedBatch {
        AnimatedModel* model = nullptr;
        std::string albedoPath;
        std::string normalPath;
        std::vector<SkinnedDrawData> drawData;
        std::vector<DrawIndirectCommand> commands;
        std::vector<glm::mat4> packedBones;        // flat bone array for this batch
        uint32_t totalTriangles = 0;
    };

    std::unordered_map<std::string, SkinnedBatch> skinnedBatches;

    for (const auto& sub : m_SkinnedQueue) {
        const auto& merged = sub.model->GetMergedBuffers();

        // Determine texture paths (fallback to model's embedded materials if empty)
        std::string albedo = sub.albedoPath;
        std::string normal = sub.normalPath;
        if (albedo.empty() && !sub.model->GetMaterials().empty()) {
            auto& mat = sub.model->GetMaterials()[0];
            if (mat.diffuseTexture) albedo = "__embedded";
            // For embedded textures, batch key includes model pointer
        }

        std::string key = MakeBatchKey(
            std::to_string(reinterpret_cast<uintptr_t>(sub.model)),
            albedo, normal);

        auto& batch = skinnedBatches[key];
        if (!batch.model) {
            batch.model = sub.model;
            batch.albedoPath = albedo;
            batch.normalPath = normal;
        }

        // Record bone offset (in number of mat4s) into the packed array
        int32_t boneOffset = static_cast<int32_t>(batch.packedBones.size());
        int32_t boneCount  = static_cast<int32_t>(sub.boneMatrices->size());

        // Append this instance's bones to the flat array
        batch.packedBones.insert(batch.packedBones.end(),
            sub.boneMatrices->begin(), sub.boneMatrices->end());

        // One draw command per mesh in the animated model
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

    // -----------------------------------------------
    // Step 2: Render each skinned batch
    // -----------------------------------------------
    m_SkinnedBatchedShader->Bind();
    m_SkinnedBatchedShader->SetMat4("u_View", m_View);
    m_SkinnedBatchedShader->SetMat4("u_Projection", m_Projection);
    m_SkinnedBatchedShader->SetVec3("u_ViewPos", m_CameraPos);
    m_SkinnedBatchedShader->SetInt("u_AlbedoMap", 0);
    m_SkinnedBatchedShader->SetInt("u_NormalMap", 1);
    m_SkinnedBatchedShader->SetInt("u_ShadowMap", 2);

    UploadLightUniformsInternal(m_SkinnedBatchedShader.get());
    UploadShadowUniformsInternal(m_SkinnedBatchedShader.get());

    if (m_ShadowsEnabled && m_CSM) {
        RenderCommand::BindTextureArray(2, m_CSM->GetDepthTextureArray());
    }

    for (auto& [key, batch] : skinnedBatches) {
        // Bind textures
        Texture* albedo = m_AssetManager->ResolveTexture(batch.albedoPath);
        Texture* normal = m_AssetManager->ResolveTexture(batch.normalPath);

        // Handle embedded textures from AnimatedModel
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

        // Upload bone SSBO (binding 1)
        m_BoneSSBO->Upload(batch.packedBones.data(),
            batch.packedBones.size() * sizeof(glm::mat4), 1);

        // Upload draw data SSBO (binding 0)
        m_SkinnedSSBO->Upload(batch.drawData.data(),
            batch.drawData.size() * sizeof(SkinnedDrawData), 0);

        // Upload indirect commands
        m_SkinnedCmdBO->Upload(batch.commands.data(),
            batch.commands.size() * sizeof(DrawIndirectCommand));

        // Draw
        RenderCommand::DrawBatched(*batch.model->GetMergedBuffers().vao,
            static_cast<uint32_t>(batch.commands.size()));

        m_Stats.skinnedDrawCalls++;
        m_Stats.skinnedInstances += batch.drawData.size();
        m_Stats.triangles += batch.totalTriangles;
    }

    m_SkinnedCmdBO->UnBind();
    m_SkinnedSSBO->UnBind();
    m_BoneSSBO->UnBind();
    m_Stats.drawCalls += m_Stats.skinnedDrawCalls;
}
```

### 3.10 Shadow Pass

Same as current `ViewportPanel::RenderShadowPass()` but supports both static and skinned:

```cpp
void SceneRenderer::RenderShadowPass() {
    m_CSM->ClearAll();
    RenderCommand::EnablePolygonOffset(4.0f, 4.0f);
    RenderCommand::SetCullFace(true);  // front-face culling for shadows

    for (uint32_t cascade = 0; cascade < NUM_SHADOW_CASCADES; cascade++) {
        m_CSM->BindCascade(cascade);
        const glm::mat4& lightSpaceMat = m_CSM->GetLightSpaceMatrix(cascade);

        // --- Static shadow draws ---
        m_StaticShadowShader->Bind();
        m_StaticShadowShader->SetMat4("u_LightSpaceMatrix", lightSpaceMat);

        for (auto& [key, batch] : m_StaticBatches) {
            // Frustum cull against this cascade
            std::vector<StaticDrawData> culledData;
            std::vector<DrawIndirectCommand> culledCmds;
            FrustumCullBatch(lightSpaceMat, batch, culledData, culledCmds);

            if (culledCmds.empty()) continue;

            m_StaticSSBO->Upload(culledData.data(),
                culledData.size() * sizeof(StaticDrawData), 0);
            m_StaticCmdBO->Upload(culledCmds.data(),
                culledCmds.size() * sizeof(DrawIndirectCommand));
            RenderCommand::DrawBatched(*batch.model->GetMergedBuffers().vao,
                static_cast<uint32_t>(culledCmds.size()));
        }

        // --- Skinned shadow draws ---
        // (rebuild skinned batches as in RenderSkinnedPass, using shadow shader)
        m_SkinnedShadowShader->Bind();
        m_SkinnedShadowShader->SetMat4("u_LightSpaceMatrix", lightSpaceMat);
        // ... same bone SSBO upload pattern, just with shadow shader ...
    }

    m_CSM->UnBind();
    RenderCommand::DisablePolygonOffset();
    RenderCommand::SetCullFace(false);  // restore back-face culling
}
```

### 3.11 UploadLightUniforms / UploadShadowUniforms (Public)

These are exposed so consumers can upload the same lighting to their own shaders
(e.g., terrain shader):

```cpp
void SceneRenderer::UploadLightUniforms(Shader* shader) const {
    UploadLightUniformsInternal(shader);
}

void SceneRenderer::UploadShadowUniforms(Shader* shader) const {
    UploadShadowUniformsInternal(shader);
}
```

The internal implementations set the same uniforms as current `ViewportPanel::UploadLightUniforms`:
- `u_LightDir`, `u_LightColor`, `u_AmbientStrength`
- `u_PointLightCount`, `u_PointLightPos[i]`, etc.
- `u_EnableShadows`, `u_ShadowBias`, `u_LightSpaceMatrices[i]`, `u_CascadeSplits[i]`

---

## 4. Shader Changes

### 4.1 New Shaders to Create

All shaders go into `Onyx/Assets/Shaders/` (engine-level, shared by all apps).

#### `skinned_batched.vert` (NEW — #version 460 core)

```glsl
#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
layout (location = 5) in ivec4 a_BoneIds;
layout (location = 6) in vec4 a_BoneWeights;

out vec3 v_FragPos;
out vec2 v_TexCoord;
out mat3 v_TBN;

struct DrawData {
    mat4 model;
    int boneOffset;
    int boneCount;
    int _pad0;
    int _pad1;
};

layout(std430, binding = 0) readonly buffer DrawDataBuffer {
    DrawData draws[];
};

layout(std430, binding = 1) readonly buffer BoneBuffer {
    mat4 bones[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    DrawData d = draws[gl_DrawID];

    // Skeletal animation: blend bone transforms
    mat4 boneTransform = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; i++) {
        int boneId = a_BoneIds[i];
        float weight = a_BoneWeights[i];
        if (boneId >= 0 && boneId < d.boneCount && weight > 0.0) {
            boneTransform += bones[d.boneOffset + boneId] * weight;
            totalWeight += weight;
        }
    }

    // Fallback to identity if no bones affect this vertex
    if (totalWeight < 0.01) {
        boneTransform = mat4(1.0);
    }

    // Apply bone transform, then world transform
    vec4 skinnedPos = boneTransform * vec4(a_Position, 1.0);
    vec4 worldPos = d.model * skinnedPos;
    v_FragPos = worldPos.xyz;
    v_TexCoord = a_TexCoord;

    // TBN matrix for normal mapping
    mat3 skinMatrix = mat3(d.model * boneTransform);
    vec3 T = normalize(skinMatrix * a_Tangent);
    vec3 B = normalize(skinMatrix * a_Bitangent);
    vec3 N = normalize(skinMatrix * a_Normal);
    v_TBN = mat3(T, B, N);

    gl_Position = u_Projection * u_View * worldPos;
}
```

#### `shadow_depth_skinned_batched.vert` (NEW — #version 460 core)

```glsl
#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 5) in ivec4 a_BoneIds;
layout (location = 6) in vec4 a_BoneWeights;

struct DrawData {
    mat4 model;
    int boneOffset;
    int boneCount;
    int _pad0;
    int _pad1;
};

layout(std430, binding = 0) readonly buffer DrawDataBuffer {
    DrawData draws[];
};

layout(std430, binding = 1) readonly buffer BoneBuffer {
    mat4 bones[];
};

uniform mat4 u_LightSpaceMatrix;

void main() {
    DrawData d = draws[gl_DrawID];

    mat4 boneTransform = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; i++) {
        int boneId = a_BoneIds[i];
        float weight = a_BoneWeights[i];
        if (boneId >= 0 && boneId < d.boneCount && weight > 0.0) {
            boneTransform += bones[d.boneOffset + boneId] * weight;
            totalWeight += weight;
        }
    }
    if (totalWeight < 0.01) boneTransform = mat4(1.0);

    vec4 skinnedPos = boneTransform * vec4(a_Position, 1.0);
    gl_Position = u_LightSpaceMatrix * d.model * skinnedPos;
}
```

### 4.2 Existing Shaders to Move

Move from `MMOGame/Editor3D/assets/shaders/` to `Onyx/Assets/Shaders/`:

| File | Notes |
|------|-------|
| `model_batched.vert` | No changes needed |
| `shadow_depth_batched.vert` | No changes needed |
| `model.frag` | Shared by all vertex shaders. No changes needed. |
| `shadow_depth.frag` | Empty fragment shader. No changes needed. |

Editor3D shaders that stay in `Editor3D/assets/shaders/` (editor-specific):
- `basic3d.vert/frag` (cube/primitive shader)
- `model.vert` (non-batched legacy, still used for selection wireframe)
- `skinned.vert` (legacy non-batched, can be deleted after migration)
- `infinite_grid.vert/frag`
- `outline.vert/frag`
- `picking*.vert/frag`
- `billboard.vert/frag`
- `terrain.vert/frag`
- `shadow_depth.vert` (non-batched, for terrain shadows)

### 4.3 Engine Shader Directory

Create `Onyx/Assets/Shaders/` (new directory):
```
Onyx/Assets/Shaders/
├── model_batched.vert
├── skinned_batched.vert
├── model.frag
├── shadow_depth_batched.vert
├── shadow_depth_skinned_batched.vert
└── shadow_depth.frag
```

SceneRenderer loads these at `Init()` time:
```cpp
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

    // GPU buffers
    m_StaticSSBO = std::make_unique<ShaderStorageBuffer>();
    m_StaticCmdBO = std::make_unique<DrawCommandBuffer>();
    m_SkinnedSSBO = std::make_unique<ShaderStorageBuffer>();
    m_BoneSSBO = std::make_unique<ShaderStorageBuffer>();
    m_SkinnedCmdBO = std::make_unique<DrawCommandBuffer>();

    // CSM
    m_CSM = std::make_unique<CascadedShadowMap>();
    m_CSM->Create(2048);
}
```

---

## 5. AnimatedModel Changes

### 5.1 Add BuildMergedBuffers()

`AnimatedModel` needs the same merged buffer support as `Model`.

Add to `AnimatedModel.h`:
```cpp
struct SkinnedMergedBuffers {
    std::unique_ptr<VertexArray> vao;
    std::unique_ptr<VertexBuffer> vbo;
    std::unique_ptr<IndexBuffer> ebo;
    uint32_t totalVertices = 0;
    uint32_t totalIndices = 0;
    std::vector<MergedMeshInfo> meshInfos;
};

class AnimatedModel {
public:
    // ... existing API ...

    void BuildMergedBuffers();
    const SkinnedMergedBuffers& GetMergedBuffers() const { return m_Merged; }
    bool HasMergedBuffers() const { return m_Merged.vao != nullptr; }

private:
    SkinnedMergedBuffers m_Merged;
};
```

### 5.2 BuildMergedBuffers Implementation

```cpp
void AnimatedModel::BuildMergedBuffers() {
    if (m_Merged.vao) return;  // already built
    if (m_Meshes.empty()) return;

    // Count total vertices and indices across all meshes
    // Note: unlike Model, AnimatedModel doesn't store raw vertex vectors after upload.
    // We need to keep the raw data OR re-read from GPU.
    //
    // CHANGE REQUIRED: Store raw vertices/indices in SkinnedMesh (see 5.3)

    std::vector<SkinnedVertex> allVertices;
    std::vector<uint32_t> allIndices;

    for (auto& mesh : m_Meshes) {
        MergedMeshInfo info;
        info.indexCount = static_cast<uint32_t>(mesh.indices.size());
        info.firstIndex = static_cast<uint32_t>(allIndices.size());
        info.baseVertex = static_cast<int32_t>(allVertices.size());

        allVertices.insert(allVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
        allIndices.insert(allIndices.end(), mesh.indices.begin(), mesh.indices.end());

        m_Merged.meshInfos.push_back(info);
    }

    m_Merged.totalVertices = static_cast<uint32_t>(allVertices.size());
    m_Merged.totalIndices = static_cast<uint32_t>(allIndices.size());

    // Create merged GPU buffers
    m_Merged.vbo = std::make_unique<VertexBuffer>(
        allVertices.data(), allVertices.size() * sizeof(SkinnedVertex));

    m_Merged.ebo = std::make_unique<IndexBuffer>(
        allIndices.data(), allIndices.size() * sizeof(uint32_t));

    m_Merged.vao = std::make_unique<VertexArray>();

    // Skinned vertex layout: pos(3) + normal(3) + uv(2) + tangent(3) + bitangent(3) + boneIds(4i) + boneWeights(4)
    VertexLayout layout;
    layout.PushFloat(3);  // position
    layout.PushFloat(3);  // normal
    layout.PushFloat(2);  // texCoords
    layout.PushFloat(3);  // tangent
    layout.PushFloat(3);  // bitangent
    layout.PushInt(4);    // boneIds (ivec4)
    layout.PushFloat(4);  // boneWeights (vec4)

    m_Merged.vao->SetVertexBuffer(m_Merged.vbo.get());
    m_Merged.vao->SetIndexBuffer(m_Merged.ebo.get());
    m_Merged.vao->SetLayout(layout);
}
```

### 5.3 Store Raw Vertex/Index Data in SkinnedMesh

Currently `SkinnedMesh` doesn't keep CPU-side vertex/index data after upload.
We need to either:

**(a) Keep the data** — add fields to `SkinnedMesh`:
```cpp
struct SkinnedMesh {
    // ... existing GPU resources ...

    // CPU-side data (kept for BuildMergedBuffers)
    std::vector<SkinnedVertex> vertices;
    std::vector<uint32_t> indices;
};
```

This mirrors how `Mesh` stores `m_Vertices` and `m_Indices`.

**(b)** Alternative: build merged buffers during `ProcessMeshImpl` before creating
per-mesh VAOs. This avoids storing duplicates but requires reordering the load pipeline.

**Recommendation: (a)** — straightforward, matches the existing `Mesh` pattern.
Memory cost is minimal (skinned models are typically <100k vertices).

---

## 6. Migration — Editor3D ViewportPanel

### 6.1 What ViewportPanel Keeps

- Camera controls, viewport interaction, ImGui rendering
- Gizmo system (`TransformGizmo`)
- GPU picking (`m_PickingFramebuffer`, picking shaders)
- Selection wireframe overlay (uses `m_ModelShader` non-batched)
- Grid rendering (`m_InfiniteGridShader`)
- Billboard icons for lights/spawns/triggers
- Primitive rendering (cubes for objects without models)
- Terrain system (`EditorWorldSystem`, terrain shader)
- `m_AnimatorCache` (per-object animation state)

### 6.2 What ViewportPanel Delegates to SceneRenderer

- Shadow pass for models
- Opaque static model rendering (batched)
- Opaque skinned model rendering (batched)
- Light uniform management (reuse via `UploadLightUniforms`)

### 6.3 What ViewportPanel Delegates to AssetManager

- Model loading/caching (`GetOrLoadModel` → `AssetManager::LoadModel`)
- AnimatedModel loading/caching (`GetOrLoadAnimatedModel` → `AssetManager::LoadAnimatedModel`)
- Texture loading/caching (`GetOrLoadTexture` → `AssetManager::ResolveTexture`)
- Material storage (`m_MaterialLibrary` → `AssetManager::GetMaterial`)

### 6.4 Refactored RenderScene

```cpp
void ViewportPanel::RenderScene() {
    m_TriangleCount = 0;
    m_DrawCalls = 0;

    CollectLights();

    // --- Submit to SceneRenderer ---
    m_SceneRenderer->Begin(m_ViewMatrix, m_ProjectionMatrix, m_CameraPosition);

    // Directional light
    DirectionalLight dirLight;
    dirLight.direction = m_LightDir;
    dirLight.color = m_LightColor;
    dirLight.enabled = m_EnableDirectionalLight;
    m_SceneRenderer->SetDirectionalLight(dirLight);
    m_SceneRenderer->SetAmbientStrength(m_AmbientStrength);

    // Point/spot lights
    for (auto& pl : m_CollectedPointLights) {
        m_SceneRenderer->AddPointLight({pl.position, pl.color, pl.range});
    }
    for (auto& sl : m_CollectedSpotLights) {
        m_SceneRenderer->AddSpotLight({sl.position, sl.direction, sl.color,
                                        sl.range, sl.innerCos, sl.outerCos});
    }

    // Shadow config
    m_SceneRenderer->SetShadowsEnabled(m_EnableShadows);
    m_SceneRenderer->SetShadowBias(m_ShadowBias);
    m_SceneRenderer->SetShadowDistance(m_ShadowDistance);
    m_SceneRenderer->SetSplitLambda(m_SplitLambda);
    m_SceneRenderer->SetShowCascades(m_ShowCascades);

    // Submit static objects
    for (const auto& obj : m_World->GetStaticObjects()) {
        if (!obj->IsVisible()) continue;
        const std::string& modelPath = obj->GetModelPath();
        if (modelPath.empty() || modelPath[0] == '#') continue;

        if (IsAnimatedModel(modelPath)) {
            // Submit as skinned
            auto* animModel = m_AssetManager->Get(
                m_AssetManager->LoadAnimatedModel(modelPath));
            if (!animModel) continue;

            auto* animator = GetAnimator(obj->GetGuid());
            if (animator->IsPlaying()) {
                animator->Update(deltaTime);
            }

            glm::mat4 worldMat = m_World->GetWorldMatrix(obj.get());
            std::string albedo, normal;
            ResolveMaterial(obj.get(), albedo, normal);  // helper

            m_SceneRenderer->SubmitSkinned(animModel, worldMat,
                animator->GetBoneMatrices(), albedo, normal);
        } else {
            // Submit as static (per-mesh, with MeshMaterial transforms)
            auto* model = m_AssetManager->Get(m_AssetManager->LoadModel(modelPath));
            if (!model) continue;

            glm::mat4 objectMatrix = m_World->GetWorldMatrix(obj.get());
            for (uint32_t meshIdx = 0; meshIdx < model->GetMeshes().size(); meshIdx++) {
                auto& mesh = model->GetMeshes()[meshIdx];
                const MeshMaterial* meshMat = mesh.m_Name.empty() ?
                    nullptr : obj->GetMeshMaterial(mesh.m_Name);

                glm::mat4 meshMatrix = ComputeMeshMatrix(objectMatrix, meshMat, mesh.GetCenter());
                std::string albedo, normal;
                ResolveMeshMaterial(obj.get(), meshMat, albedo, normal);

                m_SceneRenderer->SubmitStatic(model, meshIdx, meshMatrix, albedo, normal);
            }
        }
    }

    // Submit spawn point models too
    for (const auto& spawn : m_World->GetSpawnPoints()) {
        if (!spawn->IsVisible() || spawn->GetModelPath().empty()) continue;
        auto* model = m_AssetManager->Get(m_AssetManager->LoadModel(spawn->GetModelPath()));
        if (!model) continue;

        glm::mat4 worldMat = m_World->GetWorldMatrix(spawn.get());
        std::string albedo, normal;
        ResolveMaterial(spawn.get(), albedo, normal);

        for (uint32_t i = 0; i < model->GetMeshes().size(); i++) {
            m_SceneRenderer->SubmitStatic(model, i, worldMat, albedo, normal);
        }
    }

    // --- Flush: shadow + opaque passes ---
    m_SceneRenderer->Flush(m_Framebuffer.get());

    // --- Post-flush: things rendered on top with shared depth buffer ---

    // Terrain (uses its own shader, but needs shadow/light uniforms from renderer)
    if (m_TerrainEnabled) {
        RenderTerrain();  // can call m_SceneRenderer->UploadShadowUniforms(m_TerrainShader)
    }

    // Cubes, icons, gizmos, selection wireframes, grid (editor-only)
    RenderPrimitives();       // cubes for objects without models
    RenderGizmoIcons();       // light/spawn/trigger billboard icons
    RenderSelectionOutline(); // wireframe highlights
    RenderGizmo();            // transform gizmo

    if (m_ShowGrid) RenderGrid();

    // Resolve MSAA
    m_Framebuffer->Resolve();

    // Stats
    auto stats = m_SceneRenderer->GetStats();
    m_TriangleCount = stats.triangles;
    m_DrawCalls = stats.drawCalls;
    m_BatchedDrawCalls = stats.batchedDrawCalls;
    m_BatchedMeshCount = stats.batchedMeshCount;
}
```

### 6.5 What Gets Deleted from ViewportPanel

```
REMOVE:
  - struct DrawDataGPU, DrawCommand, MeshBoundsEntry, ModelBatch
  - m_ModelBatches map
  - m_DrawDataSSBO, m_DrawCmdBO
  - m_BatchedModelShader, m_BatchedShadowShader
  - m_CSM (owned by SceneRenderer now)
  - m_ShadowDepthShader
  - BuildModelBatches()
  - RenderShadowPass()
  - The "Batched model meshes" section of RenderWorldObjects()
  - m_ModelCache (use AssetManager)
  - m_AnimatedModelCache (use AssetManager)
  - m_MaterialLibrary (use AssetManager)
  - GetOrLoadModel(), GetOrLoadTexture(), GetOrLoadAnimatedModel()

KEEP:
  - m_ModelShader (still needed for selection wireframe overlay)
  - m_ObjectShader (still needed for cubes/primitives)
  - All editor-specific rendering (grid, gizmos, picking, billboards)
  - m_AnimatorCache (runtime animation state, not an asset)
  - Camera, gizmo, picking logic
```

---

## 7. Migration — MMO Client

The MMO client can use the same `SceneRenderer` for rendering player models,
NPC models, and world objects.

```cpp
// In client rendering code:

void GameRenderer::Render(float dt) {
    m_SceneRenderer->Begin(m_Camera.GetView(), m_Camera.GetProjection(), m_Camera.GetPosition());

    // Zone lighting
    m_SceneRenderer->SetDirectionalLight(m_ZoneLighting.sun);
    m_SceneRenderer->SetAmbientStrength(m_ZoneLighting.ambient);

    // Static world objects (buildings, props)
    for (auto& obj : m_VisibleStaticObjects) {
        auto* model = m_AssetManager->Get(obj.modelHandle);
        for (uint32_t i = 0; i < model->GetMeshes().size(); i++) {
            m_SceneRenderer->SubmitStatic(model, i, obj.worldMatrix, obj.albedo, obj.normal);
        }
    }

    // Animated entities (players, NPCs)
    for (auto& entity : m_VisibleEntities) {
        auto* animModel = m_AssetManager->Get(entity.animModelHandle);
        entity.animator.Update(dt);
        m_SceneRenderer->SubmitSkinned(animModel, entity.worldMatrix,
            entity.animator.GetBoneMatrices(), entity.albedo, entity.normal);
    }

    m_SceneRenderer->Flush(nullptr);  // render to backbuffer

    // Terrain, UI, particles, etc.
    RenderTerrain();
    RenderParticles();
    RenderUI();
}
```

---

## 8. File Layout

### New files to create:

```
Onyx/Source/Graphics/
├── AssetHandle.h          (handle template + typedefs)
├── AssetManager.h         (class declaration)
├── AssetManager.cpp        (implementation)
├── SceneRenderer.h         (class declaration + GPU structs)
└── SceneRenderer.cpp       (implementation)

Onyx/Assets/Shaders/       (new directory)
├── model_batched.vert      (moved from Editor3D, unchanged)
├── model.frag              (moved from Editor3D, unchanged)
├── shadow_depth_batched.vert (moved from Editor3D, unchanged)
├── shadow_depth.frag       (moved from Editor3D, unchanged)
├── skinned_batched.vert    (NEW)
└── shadow_depth_skinned_batched.vert (NEW)
```

### Files to modify:

```
Onyx/Source/Graphics/AnimatedModel.h     (add BuildMergedBuffers, SkinnedMergedBuffers, keep raw data)
Onyx/Source/Graphics/AnimatedModel.cpp   (implement BuildMergedBuffers, store vertices in SkinnedMesh)

MMOGame/Editor3D/Source/Panels/ViewportPanel.h   (remove batch structs, add SceneRenderer*)
MMOGame/Editor3D/Source/Panels/ViewportPanel.cpp  (refactor RenderScene, delete batch code)
MMOGame/Editor3D/Source/Editor3DLayer.cpp         (create AssetManager + SceneRenderer, pass to panels)

Onyx/CMakeLists.txt         (new .cpp files if not GLOB_RECURSE picking them up)
MMOGame/Editor3D/CMakeLists.txt  (remove MaterialLibrary dependency if fully absorbed)
```

### Files to delete:

```
Onyx/Source/Graphics/MaterialLibrary.h    (absorbed into AssetManager)
Onyx/Source/Graphics/MaterialLibrary.cpp  (absorbed into AssetManager)
```

(Or keep MaterialLibrary as a thin compatibility wrapper if other code depends on it.)

---

## 9. Implementation Order

### Step 1: AssetManager (no breaking changes)

1. Create `AssetHandle.h`, `AssetManager.h`, `AssetManager.cpp`
2. Implement `LoadModel`, `LoadAnimatedModel`, `LoadTexture`, `Get`, `ResolveTexture`
3. Absorb material storage from `MaterialLibrary`
4. Add to Onyx CMakeLists
5. **Test**: Create `AssetManager` in `Editor3DLayer`, verify it loads the same models

### Step 2: AnimatedModel::BuildMergedBuffers

1. Add `vertices`/`indices` fields to `SkinnedMesh`
2. Store raw data during `ProcessMeshImpl`
3. Implement `BuildMergedBuffers()` with skinned vertex layout
4. **Test**: Call `BuildMergedBuffers()` on a loaded AnimatedModel, verify VAO is valid

### Step 3: SceneRenderer — Static Only

1. Create `SceneRenderer.h`, `SceneRenderer.cpp`
2. Move `model_batched.vert`, `shadow_depth_batched.vert`, `model.frag`, `shadow_depth.frag`
   to `Onyx/Assets/Shaders/`
3. Implement `Init`, `Begin`, `SubmitStatic`, `Flush` (static path only)
4. Move shadow pass logic from ViewportPanel
5. Wire into ViewportPanel alongside existing code (dual path for validation)
6. **Test**: Verify identical rendering output with SceneRenderer vs old code

### Step 4: SceneRenderer — Skinned

1. Create `skinned_batched.vert`, `shadow_depth_skinned_batched.vert`
2. Implement `SubmitSkinned` and `RenderSkinnedPass`
3. Add skinned shadow pass
4. **Test**: Load an animated model, verify it renders with bone animation

### Step 5: ViewportPanel Cleanup

1. Remove old batch structs, SSBOs, shaders from ViewportPanel
2. Remove `m_ModelCache`, `m_AnimatedModelCache`, `m_MaterialLibrary`
3. Use `AssetManager` everywhere
4. Delete or deprecate `MaterialLibrary`
5. **Test**: Full editor functionality — models, shadows, terrain, picking, gizmos

### Step 6: MMO Client Integration

1. Create a `GameRenderer` in the client that uses `SceneRenderer`
2. Wire entity rendering through `SubmitStatic` / `SubmitSkinned`
3. **Test**: Client renders world with the same visual quality as editor

---

## Appendix A: TransformAABB Helper

This utility already exists in ViewportPanel. Move to engine:

```cpp
// Onyx/Source/Graphics/SceneRenderer.cpp (or a MathUtils header)

static void TransformAABB(const glm::vec3& localMin, const glm::vec3& localMax,
                           const glm::mat4& transform,
                           glm::vec3& worldMin, glm::vec3& worldMax) {
    glm::vec3 corners[8] = {
        {localMin.x, localMin.y, localMin.z},
        {localMax.x, localMin.y, localMin.z},
        {localMin.x, localMax.y, localMin.z},
        {localMax.x, localMax.y, localMin.z},
        {localMin.x, localMin.y, localMax.z},
        {localMax.x, localMin.y, localMax.z},
        {localMin.x, localMax.y, localMax.z},
        {localMax.x, localMax.y, localMax.z},
    };

    worldMin = glm::vec3(std::numeric_limits<float>::max());
    worldMax = glm::vec3(std::numeric_limits<float>::lowest());

    for (auto& c : corners) {
        glm::vec3 w = glm::vec3(transform * glm::vec4(c, 1.0f));
        worldMin = glm::min(worldMin, w);
        worldMax = glm::max(worldMax, w);
    }
}
```

## Appendix B: Frustum Culling Helper

Extract from `Frustum.h`/`.cpp` (already exists in engine). The shadow pass uses
6 frustum planes extracted from the light-space matrix:

```cpp
static void ExtractFrustumPlanes(const glm::mat4& vp, glm::vec4 planes[6]) {
    // Left, Right, Bottom, Top, Near, Far
    planes[0] = glm::vec4(vp[0][3]+vp[0][0], vp[1][3]+vp[1][0], vp[2][3]+vp[2][0], vp[3][3]+vp[3][0]);
    planes[1] = glm::vec4(vp[0][3]-vp[0][0], vp[1][3]-vp[1][0], vp[2][3]-vp[2][0], vp[3][3]-vp[3][0]);
    planes[2] = glm::vec4(vp[0][3]+vp[0][1], vp[1][3]+vp[1][1], vp[2][3]+vp[2][1], vp[3][3]+vp[3][1]);
    planes[3] = glm::vec4(vp[0][3]-vp[0][1], vp[1][3]-vp[1][1], vp[2][3]-vp[2][1], vp[3][3]-vp[3][1]);
    planes[4] = glm::vec4(vp[0][3]+vp[0][2], vp[1][3]+vp[1][2], vp[2][3]+vp[2][2], vp[3][3]+vp[3][2]);
    planes[5] = glm::vec4(vp[0][3]-vp[0][2], vp[1][3]-vp[1][2], vp[2][3]-vp[2][2], vp[3][3]-vp[3][2]);

    for (int i = 0; i < 6; i++) {
        float len = glm::length(glm::vec3(planes[i]));
        planes[i] /= len;
    }
}

static bool AABBInFrustum(const glm::vec3& min, const glm::vec3& max, const glm::vec4 planes[6]) {
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
```

## Appendix C: SSBO Binding Points

| Binding | Static Pass | Skinned Pass | Contents |
|---------|-------------|--------------|----------|
| 0 | `StaticDrawData[]` | `SkinnedDrawData[]` | Per-draw transforms |
| 1 | (unused) | `mat4[]` | Packed bone matrices |
| 2 | (reserved for future UBO lights) | (same) | — |

The `GL_DRAW_INDIRECT_BUFFER` is bound implicitly (no binding point number).

The shadow texture array is bound to texture slot 2 via `glActiveTexture(GL_TEXTURE2)`.
