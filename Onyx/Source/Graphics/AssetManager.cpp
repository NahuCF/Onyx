#include "pch.h"
#include "AssetManager.h"
#include "Model.h"
#include "AnimatedModel.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace Onyx {

AssetManager::AssetManager() {
    m_DefaultAlbedo = Texture::CreateSolidColor(200, 200, 200);
    m_DefaultNormal = Texture::CreateSolidColor(128, 128, 255);

    InitBufferPool();
    m_LoadThread = std::thread(&AssetManager::LoaderThreadFunc, this);
    m_TextureLoadThread = std::thread(&AssetManager::TextureLoaderThreadFunc, this);
}

void AssetManager::InitBufferPool() {
    // Small pool for typical models (< 2MB vertex data)
    constexpr size_t POOL_SIZE = 32;
    constexpr size_t DEFAULT_VBO_BYTES = 2 * 1024 * 1024;  // 2MB (~35k MeshVertex)
    constexpr size_t DEFAULT_EBO_BYTES = 1 * 1024 * 1024;  // 1MB (~250k indices)

    for (size_t i = 0; i < POOL_SIZE; i++) {
        PooledBufferSet set;
        set.vbo = std::make_unique<VertexBuffer>(static_cast<uint32_t>(DEFAULT_VBO_BYTES));
        set.ebo = std::make_unique<IndexBuffer>(static_cast<uint32_t>(DEFAULT_EBO_BYTES));
        set.vboCapacity = DEFAULT_VBO_BYTES;
        set.eboCapacity = DEFAULT_EBO_BYTES;

        set.vao = std::make_unique<VertexArray>();

        auto layout = MeshVertex::GetLayout();
        set.vao->SetVertexBuffer(set.vbo.get());
        set.vao->SetIndexBuffer(set.ebo.get());
        set.vao->SetLayout(layout);

        m_StaticBufferPool.push_back(std::move(set));
    }

    // Large pool for big models (e.g., high-poly FBX files)
    // No VAO pre-configured — layout depends on static vs skinned, set at upload time
    constexpr size_t LARGE_POOL_SIZE = 4;
    constexpr size_t LARGE_VBO_BYTES = 32 * 1024 * 1024;  // 32MB (~571k MeshVertex or ~363k SkinnedVertex)
    constexpr size_t LARGE_EBO_BYTES = 16 * 1024 * 1024;  // 16MB (~4M indices)

    for (size_t i = 0; i < LARGE_POOL_SIZE; i++) {
        LargePoolEntry entry;
        entry.vbo = std::make_unique<VertexBuffer>(static_cast<uint32_t>(LARGE_VBO_BYTES));
        entry.ebo = std::make_unique<IndexBuffer>(static_cast<uint32_t>(LARGE_EBO_BYTES));
        entry.vboCapacity = LARGE_VBO_BYTES;
        entry.eboCapacity = LARGE_EBO_BYTES;
        m_LargeBufferPool.push_back(std::move(entry));
    }

    // Warm up all pool buffers — forces driver to commit physical VRAM now
    // rather than stalling on first glBufferSubData during model upload.
    uint8_t warmup[4] = {0};
    for (auto& set : m_StaticBufferPool) {
        set.vbo->SetSubData(warmup, 0, 1);
        set.ebo->SetSubData(warmup, 0, 1);
    }
    for (auto& entry : m_LargeBufferPool) {
        entry.vbo->SetSubData(warmup, 0, 1);
        entry.ebo->SetSubData(warmup, 0, 1);
    }
    glFinish();  // Block until all warmup writes are committed to GPU
}

MergedBuffers AssetManager::BuildMergedFromPool(
    const void* vertexData, size_t vertexBytes,
    const void* indexData, size_t indexBytes,
    uint32_t totalVertices, uint32_t totalIndices,
    std::vector<MergedMeshInfo>&& meshInfos) {

    MergedBuffers merged;
    merged.totalVertices = totalVertices;
    merged.totalIndices = totalIndices;
    merged.meshInfos = std::move(meshInfos);

    if (!m_StaticBufferPool.empty()) {
        auto pooled = std::move(m_StaticBufferPool.front());
        m_StaticBufferPool.pop_front();

        if (vertexBytes <= pooled.vboCapacity) {
            pooled.vbo->SetSubData(vertexData, 0, static_cast<uint32_t>(vertexBytes));
        } else {
            pooled.vbo->SetData(vertexData, static_cast<uint32_t>(vertexBytes));
        }

        if (indexBytes <= pooled.eboCapacity) {
            pooled.ebo->SetSubData(indexData, 0, static_cast<uint32_t>(indexBytes));
            pooled.ebo->SetCount(totalIndices);
        } else {
            pooled.ebo->SetData(indexData, static_cast<uint32_t>(indexBytes));
        }

        merged.vao = std::move(pooled.vao);
        merged.vbo = std::move(pooled.vbo);
        merged.ebo = std::move(pooled.ebo);
    } else {
        merged.vbo = std::make_unique<VertexBuffer>(vertexData, static_cast<uint32_t>(vertexBytes));
        merged.ebo = std::make_unique<IndexBuffer>(indexData, static_cast<uint32_t>(indexBytes));
        merged.vao = std::make_unique<VertexArray>();

        auto layout = MeshVertex::GetLayout();
        merged.vao->SetVertexBuffer(merged.vbo.get());
        merged.vao->SetIndexBuffer(merged.ebo.get());
        merged.vao->SetLayout(layout);
    }

    return merged;
}

AssetManager::~AssetManager() {
    m_ShutdownLoader = true;
    m_LoadCV.notify_all();
    m_TextureLoadCV.notify_all();
    if (m_LoadThread.joinable())
        m_LoadThread.join();
    if (m_TextureLoadThread.joinable())
        m_TextureLoadThread.join();
}

ModelHandle AssetManager::LoadModel(const std::string& path, bool loadTextures) {
    auto it = m_ModelPaths.find(path);
    if (it != m_ModelPaths.end()) return it->second;

    ModelHandle handle;
    handle.id = m_NextId++;

    try {
        auto model = std::make_unique<Model>(path.c_str(), loadTextures);
        m_Models[handle.id] = std::move(model);
    } catch (...) {
        m_Models[handle.id] = nullptr;
    }

    m_ModelPaths[path] = handle;
    m_IdToPath[handle.id] = path;
    return handle;
}

Model* AssetManager::Get(ModelHandle handle) {
    auto it = m_Models.find(handle.id);
    return it != m_Models.end() ? it->second.get() : nullptr;
}

Model* AssetManager::FindModel(const std::string& path) {
    auto it = m_ModelPaths.find(path);
    if (it == m_ModelPaths.end()) return nullptr;
    auto modelIt = m_Models.find(it->second.id);
    return modelIt != m_Models.end() ? modelIt->second.get() : nullptr;
}

AnimatedModelHandle AssetManager::LoadAnimatedModel(const std::string& path) {
    auto it = m_AnimModelPaths.find(path);
    if (it != m_AnimModelPaths.end()) return it->second;

    AnimatedModelHandle handle;
    handle.id = m_NextId++;

    auto model = std::make_unique<AnimatedModel>();
    if (!model->Load(path)) {
        m_AnimModels[handle.id] = nullptr;
    } else {
        m_AnimModels[handle.id] = std::move(model);
    }

    m_AnimModelPaths[path] = handle;
    m_IdToPath[handle.id] = path;
    return handle;
}

AnimatedModel* AssetManager::Get(AnimatedModelHandle handle) {
    auto it = m_AnimModels.find(handle.id);
    return it != m_AnimModels.end() ? it->second.get() : nullptr;
}

AnimatedModel* AssetManager::FindAnimatedModel(const std::string& path) {
    auto it = m_AnimModelPaths.find(path);
    if (it == m_AnimModelPaths.end()) return nullptr;
    auto modelIt = m_AnimModels.find(it->second.id);
    return modelIt != m_AnimModels.end() ? modelIt->second.get() : nullptr;
}

TextureHandle AssetManager::LoadTexture(const std::string& path) {
    if (path.empty()) return {};

    auto it = m_TexturePaths.find(path);
    if (it != m_TexturePaths.end()) return it->second;

    TextureHandle handle;
    handle.id = m_NextId++;

    try {
        auto texture = std::make_unique<Texture>(path.c_str());
        m_Textures[handle.id] = std::move(texture);
    } catch (...) {
        m_Textures[handle.id] = nullptr;
    }

    m_TexturePaths[path] = handle;
    return handle;
}

Texture* AssetManager::Get(TextureHandle handle) {
    auto it = m_Textures.find(handle.id);
    return it != m_Textures.end() ? it->second.get() : nullptr;
}

Texture* AssetManager::ResolveTexture(const std::string& path) {
    if (path.empty()) return nullptr;

    // Already loaded?
    auto it = m_TexturePaths.find(path);
    if (it != m_TexturePaths.end()) return Get(it->second);

    // Already pending async load?
    if (m_PendingTextures.find(path) != m_PendingTextures.end()) return nullptr;

    // Queue for async loading (stbi_load on background thread)
    auto pending = std::make_shared<PendingTextureLoad>();
    pending->path = path;
    m_PendingTextures[path] = pending;
    {
        std::lock_guard<std::mutex> lock(m_TextureLoadMutex);
        m_TextureParseQueue.push_back(pending);
    }
    m_TextureLoadCV.notify_one();
    return nullptr;  // Caller falls back to default texture
}

Material& AssetManager::CreateMaterial(const std::string& id, const std::string& name) {
    auto& mat = m_Materials[id];
    mat.id = id;
    mat.name = name.empty() ? id : name;
    return mat;
}

void AssetManager::RegisterMaterial(const Material& mat) {
    m_Materials[mat.id] = mat;
}

std::string AssetManager::GenerateUniqueMaterialId(const std::string& baseName) const {
    std::string id = baseName;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    std::replace(id.begin(), id.end(), ' ', '_');

    id.erase(std::remove_if(id.begin(), id.end(), [](char c) {
        return !std::isalnum(c) && c != '_';
    }), id.end());

    if (id.empty()) id = "material";

    if (m_Materials.find(id) == m_Materials.end()) return id;

    for (int i = 1; ; i++) {
        std::string candidate = id + "_" + std::to_string(i);
        if (m_Materials.find(candidate) == m_Materials.end()) return candidate;
    }
}

Material* AssetManager::GetMaterial(const std::string& id) {
    auto it = m_Materials.find(id);
    return it != m_Materials.end() ? &it->second : nullptr;
}

const Material* AssetManager::GetMaterial(const std::string& id) const {
    auto it = m_Materials.find(id);
    return it != m_Materials.end() ? &it->second : nullptr;
}

bool AssetManager::HasMaterial(const std::string& id) const {
    return m_Materials.find(id) != m_Materials.end();
}

bool AssetManager::RemoveMaterial(const std::string& id) {
    return m_Materials.erase(id) > 0;
}

std::vector<std::string> AssetManager::GetAllMaterialIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_Materials.size());
    for (const auto& [id, mat] : m_Materials) {
        ids.push_back(id);
    }
    return ids;
}

void AssetManager::Reload(ModelHandle handle) {
    auto it = m_IdToPath.find(handle.id);
    if (it == m_IdToPath.end()) return;
    const std::string& path = it->second;

    try {
        m_Models[handle.id] = std::make_unique<Model>(path.c_str(), false);
    } catch (...) {
        m_Models[handle.id] = nullptr;
    }
}

void AssetManager::Reload(AnimatedModelHandle handle) {
    auto it = m_IdToPath.find(handle.id);
    if (it == m_IdToPath.end()) return;
    const std::string& path = it->second;

    auto model = std::make_unique<AnimatedModel>();
    if (model->Load(path)) {
        m_AnimModels[handle.id] = std::move(model);
    } else {
        m_AnimModels[handle.id] = nullptr;
    }
}

// --- Async model loading ---

ModelLoadStatus AssetManager::GetModelStatus(const std::string& path) const {
    // Already loaded synchronously?
    if (m_ModelPaths.find(path) != m_ModelPaths.end() ||
        m_AnimModelPaths.find(path) != m_AnimModelPaths.end()) {
        return ModelLoadStatus::Ready;
    }

    std::lock_guard<std::mutex> lock(m_LoadMutex);
    auto it = m_PendingLoads.find(path);
    if (it == m_PendingLoads.end()) return ModelLoadStatus::NotRequested;
    return it->second->status.load();
}

void AssetManager::RequestModelAsync(const std::string& path, bool checkAnimated) {
    if (path.empty()) return;

    // Already loaded?
    if (m_ModelPaths.find(path) != m_ModelPaths.end() ||
        m_AnimModelPaths.find(path) != m_AnimModelPaths.end()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_LoadMutex);

    // Already pending?
    if (m_PendingLoads.find(path) != m_PendingLoads.end()) return;

    auto pending = std::make_shared<PendingLoad>();
    pending->path = path;
    pending->checkAnimated = checkAnimated;
    pending->status = ModelLoadStatus::Queued;

    m_PendingLoads[path] = pending;
    m_ParseQueue.push_back(pending);
    m_LoadCV.notify_one();
}

void AssetManager::ProcessGPUUploads(int maxPerFrame) {
    // Process pending async texture uploads
    for (int texUploaded = 0; texUploaded < maxPerFrame; texUploaded++) {
        std::shared_ptr<PendingTextureLoad> readyTex;
        {
            std::lock_guard<std::mutex> lock(m_TextureLoadMutex);
            if (!m_TextureReadyQueue.empty()) {
                readyTex = m_TextureReadyQueue.front();
                m_TextureReadyQueue.pop_front();
            }
        }
        if (!readyTex) break;

        if (readyTex->preloaded.Valid()) {
            double texStart = glfwGetTime();
            auto texture = Texture::CreateWithMipmaps(
                readyTex->preloaded.pixels.data(),
                readyTex->preloaded.width, readyTex->preloaded.height,
                readyTex->preloaded.channels);
            float texMs = static_cast<float>((glfwGetTime() - texStart) * 1000.0);

            TextureHandle handle;
            handle.id = m_NextId++;
            m_Textures[handle.id] = std::move(texture);
            m_TexturePaths[readyTex->path] = handle;
            std::cout << "[TEXTURE] Uploaded: " << readyTex->path
                      << " (" << readyTex->preloaded.width << "x" << readyTex->preloaded.height
                      << " ch=" << readyTex->preloaded.channels << ") " << texMs << " ms" << std::endl;
        }
        m_PendingTextures.erase(readyTex->path);
    }

    // Continue active staged upload first (streams one chunk per call)
    if (m_ActiveUpload) {
        ContinueStagedUpload();
        return;
    }

    int processed = 0;

    while (processed < maxPerFrame) {
        std::shared_ptr<PendingLoad> pending;

        {
            std::lock_guard<std::mutex> lock(m_LoadMutex);
            if (m_UploadQueue.empty()) break;
            pending = m_UploadQueue.front();
            m_UploadQueue.pop_front();
        }

        if (pending->status.load() == ModelLoadStatus::Failed) {
            std::lock_guard<std::mutex> lock(m_LoadMutex);
            m_PendingLoads.erase(pending->path);
            continue;
        }

        // Large model — begin staged upload (streamed across multiple frames)
        size_t totalBytes = pending->mergedVertexData.size() + pending->mergedIndexData.size();
        std::cout << "[UPLOAD] Model: " << pending->path
                  << " verts=" << pending->mergedTotalVertices
                  << " indices=" << pending->mergedTotalIndices
                  << " bytes=" << totalBytes
                  << (totalBytes > UPLOAD_CHUNK_BYTES ? " STAGED" : " IMMEDIATE")
                  << (pending->isAnimated ? " ANIMATED" : " STATIC") << std::endl;
        if (totalBytes > UPLOAD_CHUNK_BYTES) {
            BeginStagedUpload(pending);
            return;
        }

        // Small model — upload immediately (existing fast path)
        if (pending->isAnimated && pending->parsedAnimModel) {
            pending->parsedAnimModel->UploadTextures();
            pending->parsedAnimModel->BuildMergedBuffers();

            AnimatedModelHandle handle;
            handle.id = m_NextId++;
            m_AnimModels[handle.id] = std::move(pending->parsedAnimModel);
            m_AnimModelPaths[pending->path] = handle;
            m_IdToPath[handle.id] = pending->path;
        } else if (!pending->mergedVertexData.empty()) {
            auto model = std::make_unique<Model>(pending->directory, pending->meshBounds);
            model->SetMergedBuffers(BuildMergedFromPool(
                pending->mergedVertexData.data(), pending->mergedVertexData.size(),
                pending->mergedIndexData.data(), pending->mergedIndexData.size(),
                pending->mergedTotalVertices, pending->mergedTotalIndices,
                std::move(pending->mergedMeshInfos)));

            ModelHandle handle;
            handle.id = m_NextId++;
            m_Models[handle.id] = std::move(model);
            m_ModelPaths[pending->path] = handle;
            m_IdToPath[handle.id] = pending->path;
        } else {
            std::cerr << "AssetManager::ProcessGPUUploads: No data for model " << pending->path << std::endl;
            pending->status = ModelLoadStatus::Failed;
            std::lock_guard<std::mutex> lock(m_LoadMutex);
            m_PendingLoads.erase(pending->path);
            processed++;
            continue;
        }

        pending->status = ModelLoadStatus::Ready;

        {
            std::lock_guard<std::mutex> lock(m_LoadMutex);
            m_PendingLoads.erase(pending->path);
        }

        processed++;
    }
}

void AssetManager::BeginStagedUpload(std::shared_ptr<PendingLoad> pending) {
    m_ActiveUpload = std::make_unique<StagedUpload>();
    m_ActiveUpload->pending = pending;
    m_ActiveUpload->isAnimated = pending->isAnimated;

    size_t vboBytes = pending->mergedVertexData.size();
    size_t eboBytes = pending->mergedIndexData.size();

    if (!pending->isAnimated) {
        // Lightweight Model: bounds-only Mesh objects, no vertex data iteration
        m_ActiveUpload->staticModel = std::make_unique<Model>(
            pending->directory, pending->meshBounds);
    } else {
        m_ActiveUpload->animModel = std::move(pending->parsedAnimModel);
    }

    // Try pre-allocated large pool first (zero VRAM allocation, only SubData)
    if (!m_LargeBufferPool.empty() &&
        vboBytes <= m_LargeBufferPool.front().vboCapacity &&
        eboBytes <= m_LargeBufferPool.front().eboCapacity) {
        auto pooled = std::move(m_LargeBufferPool.front());
        m_LargeBufferPool.pop_front();
        m_ActiveUpload->vbo = std::move(pooled.vbo);
        m_ActiveUpload->ebo = std::move(pooled.ebo);
    } else {
        // Allocate exact-size empty buffers for very large models
        m_ActiveUpload->vbo = std::make_unique<VertexBuffer>(static_cast<uint32_t>(vboBytes));
        m_ActiveUpload->ebo = std::make_unique<IndexBuffer>(static_cast<uint32_t>(eboBytes));
    }

    // VAO always created fresh — cheap (just glGenVertexArrays), layout varies by model type
    m_ActiveUpload->vao = std::make_unique<VertexArray>();

    auto layout = pending->isAnimated ? SkinnedVertex::GetLayout() : MeshVertex::GetLayout();
    m_ActiveUpload->vao->SetVertexBuffer(m_ActiveUpload->vbo.get());
    m_ActiveUpload->vao->SetIndexBuffer(m_ActiveUpload->ebo.get());
    m_ActiveUpload->vao->SetLayout(layout);

    m_ActiveUpload->vboUploaded = 0;
    m_ActiveUpload->eboUploaded = 0;

    pending->status = ModelLoadStatus::Uploading;

    // Upload first chunk immediately
    ContinueStagedUpload();
}

void AssetManager::ContinueStagedUpload() {
    if (!m_ActiveUpload) return;

    auto& upload = *m_ActiveUpload;
    auto& pending = *upload.pending;

    size_t budget = UPLOAD_CHUNK_BYTES;

    // Upload VBO chunk
    size_t vboRemaining = pending.mergedVertexData.size() - upload.vboUploaded;
    if (vboRemaining > 0) {
        size_t chunk = std::min(vboRemaining, budget);
        upload.vbo->SetSubData(
            pending.mergedVertexData.data() + upload.vboUploaded,
            static_cast<uint32_t>(upload.vboUploaded),
            static_cast<uint32_t>(chunk));
        upload.vboUploaded += chunk;
        budget -= chunk;
    }

    // Upload EBO chunk (with remaining budget)
    size_t eboRemaining = pending.mergedIndexData.size() - upload.eboUploaded;
    if (eboRemaining > 0 && budget > 0) {
        size_t chunk = std::min(eboRemaining, budget);
        upload.ebo->SetSubData(
            pending.mergedIndexData.data() + upload.eboUploaded,
            static_cast<uint32_t>(upload.eboUploaded),
            static_cast<uint32_t>(chunk));
        upload.eboUploaded += chunk;
    }

    // Check if upload is complete
    bool vboDone = upload.vboUploaded >= pending.mergedVertexData.size();
    bool eboDone = upload.eboUploaded >= pending.mergedIndexData.size();

    if (vboDone && eboDone) {
        std::cout << "[UPLOAD] Staged upload finalized: " << upload.pending->path << std::endl;
        FinalizeStagedUpload();
    }
}

void AssetManager::FinalizeStagedUpload() {
    auto& upload = *m_ActiveUpload;
    auto& pending = *upload.pending;

    upload.ebo->SetCount(pending.mergedTotalIndices);

    MergedBuffers merged;
    merged.vao = std::move(upload.vao);
    merged.vbo = std::move(upload.vbo);
    merged.ebo = std::move(upload.ebo);
    merged.totalVertices = pending.mergedTotalVertices;
    merged.totalIndices = pending.mergedTotalIndices;
    merged.meshInfos = std::move(pending.mergedMeshInfos);

    if (upload.isAnimated && upload.animModel) {
        upload.animModel->UploadTextures();
        upload.animModel->SetMergedBuffers(std::move(merged));

        AnimatedModelHandle handle;
        handle.id = m_NextId++;
        m_AnimModels[handle.id] = std::move(upload.animModel);
        m_AnimModelPaths[pending.path] = handle;
        m_IdToPath[handle.id] = pending.path;
    } else if (upload.staticModel) {
        upload.staticModel->SetMergedBuffers(std::move(merged));

        ModelHandle handle;
        handle.id = m_NextId++;
        m_Models[handle.id] = std::move(upload.staticModel);
        m_ModelPaths[pending.path] = handle;
        m_IdToPath[handle.id] = pending.path;
    } else {
        std::cerr << "AssetManager::FinalizeStagedUpload: No model for " << pending.path << std::endl;
        pending.status = ModelLoadStatus::Failed;
        {
            std::lock_guard<std::mutex> lock(m_LoadMutex);
            m_PendingLoads.erase(pending.path);
        }
        m_ActiveUpload.reset();
        return;
    }

    pending.status = ModelLoadStatus::Ready;

    {
        std::lock_guard<std::mutex> lock(m_LoadMutex);
        m_PendingLoads.erase(pending.path);
    }

    m_ActiveUpload.reset();
}

template<typename VertexT, typename MeshRange>
void AssetManager::MergeMeshesToPending(const MeshRange& meshes, PendingLoad& out,
                                        std::vector<MeshBoundsInfo>* boundsOut) {
    uint32_t totalVerts = 0, totalIndices = 0;
    for (const auto& mesh : meshes) {
        totalVerts += static_cast<uint32_t>(mesh.vertices.size());
        totalIndices += static_cast<uint32_t>(mesh.indices.size());
    }
    out.mergedTotalVertices = totalVerts;
    out.mergedTotalIndices = totalIndices;
    out.mergedVertexData.resize(totalVerts * sizeof(VertexT));
    out.mergedIndexData.resize(totalIndices * sizeof(uint32_t));

    size_t vOff = 0, iOff = 0;
    uint32_t vertexOffset = 0, indexOffset = 0;
    for (const auto& mesh : meshes) {
        MergedMeshInfo info;
        info.indexCount = static_cast<uint32_t>(mesh.indices.size());
        info.firstIndex = indexOffset;
        info.baseVertex = static_cast<int32_t>(vertexOffset);
        out.mergedMeshInfos.push_back(info);

        if (boundsOut) {
            MeshBoundsInfo bounds;
            bounds.name = mesh.name;
            bounds.boundsMin = glm::vec3(std::numeric_limits<float>::max());
            bounds.boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
            for (const auto& v : mesh.vertices) {
                bounds.boundsMin = glm::min(bounds.boundsMin, v.position);
                bounds.boundsMax = glm::max(bounds.boundsMax, v.position);
            }
            boundsOut->push_back(bounds);
        }

        size_t vBytes = mesh.vertices.size() * sizeof(VertexT);
        std::memcpy(out.mergedVertexData.data() + vOff, mesh.vertices.data(), vBytes);
        vOff += vBytes;

        size_t iBytes = mesh.indices.size() * sizeof(uint32_t);
        std::memcpy(out.mergedIndexData.data() + iOff, mesh.indices.data(), iBytes);
        iOff += iBytes;

        vertexOffset += static_cast<uint32_t>(mesh.vertices.size());
        indexOffset += static_cast<uint32_t>(mesh.indices.size());
    }
}

void AssetManager::LoaderThreadFunc() {
    while (true) {
        std::shared_ptr<PendingLoad> pending;

        {
            std::unique_lock<std::mutex> lock(m_LoadMutex);
            m_LoadCV.wait(lock, [this] { return m_ShutdownLoader || !m_ParseQueue.empty(); });

            if (m_ShutdownLoader) return;

            pending = m_ParseQueue.front();
            m_ParseQueue.pop_front();
        }

        pending->status = ModelLoadStatus::Parsing;

        bool loaded = false;

        // Try animated first if requested
        if (pending->checkAnimated) {
            auto animModel = AnimatedModel::ParseFromFile(pending->path);
            if (animModel && animModel->GetAnimationCount() > 0) {
                animModel->PreloadTextureData();  // stbi_load on background thread
                pending->parsedAnimModel = std::move(animModel);
                pending->isAnimated = true;
                loaded = true;
            }
        }

        // Fall back to static model
        std::vector<CpuMeshData> staticMeshData;
        if (!loaded) {
            staticMeshData = Model::ParseFromFile(pending->path, pending->directory, false);
            if (!staticMeshData.empty()) {
                pending->isAnimated = false;
                loaded = true;
            }
        }

        if (loaded) {
            // Pre-concatenate mesh data on background thread (CPU-only, no GL calls)
            // This avoids large CPU allocations on the main/render thread.
            if (!pending->isAnimated) {
                MergeMeshesToPending<MeshVertex>(staticMeshData, *pending, &pending->meshBounds);
            } else {
                MergeMeshesToPending<SkinnedVertex>(pending->parsedAnimModel->GetMeshes(), *pending, nullptr);
            }

            pending->status = ModelLoadStatus::ReadyForGPU;

            {
                std::lock_guard<std::mutex> lock(m_LoadMutex);
                m_UploadQueue.push_back(pending);
            }
        } else {
            pending->status = ModelLoadStatus::Failed;

            {
                std::lock_guard<std::mutex> lock(m_LoadMutex);
                m_PendingLoads.erase(pending->path);
            }
        }
    }
}

void AssetManager::TextureLoaderThreadFunc() {
    while (true) {
        std::shared_ptr<PendingTextureLoad> pending;
        {
            std::unique_lock<std::mutex> lock(m_TextureLoadMutex);
            m_TextureLoadCV.wait(lock, [this] {
                return m_ShutdownLoader.load() || !m_TextureParseQueue.empty();
            });
            if (m_ShutdownLoader.load()) return;

            pending = m_TextureParseQueue.front();
            m_TextureParseQueue.pop_front();
        }

        pending->preloaded = Texture::PreloadFromFile(pending->path.c_str());

        {
            std::lock_guard<std::mutex> lock(m_TextureLoadMutex);
            m_TextureReadyQueue.push_back(pending);
        }
    }
}

} // namespace Onyx
