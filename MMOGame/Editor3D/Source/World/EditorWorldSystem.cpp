#include "EditorWorldSystem.h"
#include "EditorWorld.h"
#include <World/StaticObject.h>
#include <World/WorldObjectData.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

using Shared::WorldToChunkX;
using Shared::WorldToChunkZ;

namespace Editor3D {

EditorWorldSystem::EditorWorldSystem() {
}

EditorWorldSystem::~EditorWorldSystem() {
    Shutdown();
}

void EditorWorldSystem::Init(const std::string& chunksDirectory) {
    m_ProjectPath = chunksDirectory.empty() ? "." : chunksDirectory;

    std::filesystem::create_directories(m_ProjectPath);

    if (std::filesystem::exists(m_ProjectPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(m_ProjectPath)) {
            if (entry.path().extension() != ".chunk") continue;

            std::string stem = entry.path().stem().string();
            if (stem.rfind("chunk_", 0) != 0) continue;

            std::string coords = stem.substr(6);
            size_t sep = coords.find('_');
            if (sep == std::string::npos) continue;

            int32_t cx = std::stoi(coords.substr(0, sep));
            int32_t cz = std::stoi(coords.substr(sep + 1));

            m_KnownChunkFiles.insert(MakeChunkKey(cx, cz));
        }
    }
}

void EditorWorldSystem::Shutdown() {
    SaveDirtyChunks();
    m_Chunks.clear();
    m_LoadQueue.clear();
    m_KnownChunkFiles.clear();
    m_MaterialLayerMap.clear();
    m_ObjectChunkMap.clear();
}

void EditorWorldSystem::Update(const glm::vec3& cameraPos, const glm::mat4& viewProj, float deltaTime) {
    m_CameraPosition = cameraPos;
    m_Frustum.Update(viewProj);

    m_LoadQueue.clear();

    int centerChunkX = WorldToChunkX(cameraPos.x);
    int centerChunkZ = WorldToChunkZ(cameraPos.z);
    int loadRadius = static_cast<int>(std::ceil(m_Settings.loadDistance / CHUNK_SIZE));

    for (int dz = -loadRadius; dz <= loadRadius; dz++) {
        for (int dx = -loadRadius; dx <= loadRadius; dx++) {
            int32_t chunkX = centerChunkX + dx;
            int32_t chunkZ = centerChunkZ + dz;

            float dist = CalculateChunkDistance(chunkX, chunkZ);
            if (dist > m_Settings.loadDistance) continue;

            int32_t key = MakeChunkKey(chunkX, chunkZ);
            if (m_Chunks.find(key) != m_Chunks.end()) continue;
            if (m_KnownChunkFiles.find(key) == m_KnownChunkFiles.end()) continue;

            ChunkLoadRequest req;
            req.chunkX = chunkX;
            req.chunkZ = chunkZ;
            req.priority = dist;
            m_LoadQueue.push_back(req);
        }
    }

    std::sort(m_LoadQueue.begin(), m_LoadQueue.end(),
        [](const ChunkLoadRequest& a, const ChunkLoadRequest& b) {
            return a.priority < b.priority;
        });

    ProcessLoadQueue();
    UnloadDistantChunks();

    for (auto& [key, chunk] : m_Chunks) {
        TerrainChunk* terrain = chunk->GetTerrain();
        if (terrain->GetState() == ChunkState::Loaded) {
            terrain->CreateGPUResources();
        }
    }

    m_Stats.totalChunks = (int)m_Chunks.size();
    m_Stats.loadedChunks = 0;
    m_Stats.visibleChunks = 0;
    m_Stats.dirtyChunks = 0;

    for (auto& [key, chunk] : m_Chunks) {
        if (chunk->GetTerrain()->GetState() == ChunkState::Active) {
            m_Stats.loadedChunks++;
            if (IsChunkVisible(chunk->GetChunkX(), chunk->GetChunkZ())) {
                m_Stats.visibleChunks++;
            }
        }
        if (chunk->IsModified()) {
            m_Stats.dirtyChunks++;
        }
    }
}

void EditorWorldSystem::RenderTerrain(Shader* terrainShader, const glm::mat4& viewProj,
                                       PerChunkCallback perChunkSetup) {
    if (!terrainShader) return;

    static const glm::mat4 identity(1.0f);
    terrainShader->SetMat4("u_Model", identity);

    for (auto& [key, chunk] : m_Chunks) {
        TerrainChunk* terrain = chunk->GetTerrain();
        if (terrain->GetState() != ChunkState::Active) continue;
        if (!m_Settings.enableFrustumCulling || IsChunkVisible(chunk->GetChunkX(), chunk->GetChunkZ())) {
            if (perChunkSetup) {
                perChunkSetup(terrain, terrainShader);
            }
            terrain->Draw(terrainShader);
        }
    }
}

std::vector<EditorLight> EditorWorldSystem::GatherVisibleLights(const glm::vec3& cameraPos, float maxDistance) const {
    std::vector<EditorLight> result;

    for (const auto& [key, chunk] : m_Chunks) {
        if (!chunk->IsReady()) continue;

        for (const auto& light : chunk->GetLights()) {
            float dist = glm::distance(light.position, cameraPos);
            if (dist <= light.range + maxDistance) {
                result.push_back(light);
            }
        }
    }

    // Sort by distance to camera
    std::sort(result.begin(), result.end(),
        [&cameraPos](const EditorLight& a, const EditorLight& b) {
            return glm::distance(a.position, cameraPos) < glm::distance(b.position, cameraPos);
        });

    return result;
}

WorldChunk* EditorWorldSystem::GetChunk(int32_t cx, int32_t cz) {
    int32_t key = MakeChunkKey(cx, cz);
    auto it = m_Chunks.find(key);
    return (it != m_Chunks.end()) ? it->second.get() : nullptr;
}

void EditorWorldSystem::SetDefaultMaterialIds(const std::string ids[MAX_TERRAIN_LAYERS]) {
    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
        m_DefaultMaterialIds[i] = ids[i];
        if (!ids[i].empty()) {
            m_MaterialLayerMap[ids[i]] = i;
        }
    }
    for (auto& [key, chunk] : m_Chunks) {
        ApplyDefaultMaterials(chunk.get());
    }
}

void EditorWorldSystem::ApplyDefaultMaterials(WorldChunk* chunk) {
    TerrainChunk* terrain = chunk->GetTerrain();
    const auto& data = terrain->GetData();
    bool allEmpty = true;
    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
        if (!data.materialIds[i].empty()) { allEmpty = false; break; }
    }
    if (!allEmpty) return;

    std::string materials[MAX_TERRAIN_LAYERS];
    for (auto& [matId, layer] : m_MaterialLayerMap) {
        if (layer >= 0 && layer < MAX_TERRAIN_LAYERS) {
            materials[layer] = matId;
        }
    }
    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
        if (!materials[i].empty()) {
            terrain->SetLayerMaterial(i, materials[i]);
        }
    }
}

int EditorWorldSystem::ResolveGlobalLayer(const std::string& materialId) {
    auto it = m_MaterialLayerMap.find(materialId);
    if (it != m_MaterialLayerMap.end()) {
        return it->second;
    }

    bool layerUsed[MAX_TERRAIN_LAYERS] = {};
    for (auto& [mat, layer] : m_MaterialLayerMap) {
        if (layer >= 0 && layer < MAX_TERRAIN_LAYERS) {
            layerUsed[layer] = true;
        }
    }
    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
        if (!layerUsed[i]) {
            m_MaterialLayerMap[materialId] = i;
            return i;
        }
    }

    float globalSums[MAX_TERRAIN_LAYERS] = {};
    for (auto& [key, chunk] : m_Chunks) {
        const auto& data = chunk->GetTerrain()->GetData();
        if (data.splatmap.empty()) continue;
        for (int i = 0; i < SPLATMAP_TEXELS; i++) {
            for (int c = 0; c < MAX_TERRAIN_LAYERS; c++) {
                globalSums[c] += data.splatmap[i * MAX_TERRAIN_LAYERS + c];
            }
        }
    }
    int minLayer = 0;
    float minSum = globalSums[0];
    for (int i = 1; i < MAX_TERRAIN_LAYERS; i++) {
        if (globalSums[i] < minSum) {
            minSum = globalSums[i];
            minLayer = i;
        }
    }

    for (auto mapIt = m_MaterialLayerMap.begin(); mapIt != m_MaterialLayerMap.end(); ) {
        if (mapIt->second == minLayer) {
            mapIt = m_MaterialLayerMap.erase(mapIt);
        } else {
            ++mapIt;
        }
    }

    m_MaterialLayerMap[materialId] = minLayer;
    return minLayer;
}

void EditorWorldSystem::SetNormalMode(bool sobel, bool smooth) {
    if (m_SobelNormals == sobel && m_SmoothNormals == smooth) return;
    m_SobelNormals = sobel;
    m_SmoothNormals = smooth;
    for (auto& [key, chunk] : m_Chunks) {
        chunk->GetTerrain()->SetNormalMode(sobel, smooth);
    }
}

void EditorWorldSystem::SetDiamondGrid(bool enabled) {
    if (m_DiamondGrid == enabled) return;
    m_DiamondGrid = enabled;
    for (auto& [key, chunk] : m_Chunks) {
        chunk->GetTerrain()->SetDiamondGrid(enabled);
    }
}

void EditorWorldSystem::SetMeshResolution(int resolution) {
    if (m_MeshResolution == resolution) return;
    m_MeshResolution = resolution;
    for (auto& [key, chunk] : m_Chunks) {
        chunk->GetTerrain()->SetMeshResolution(resolution);
    }
}

float EditorWorldSystem::GetHeightAt(float worldX, float worldZ) {
    float height = 0.0f;
    GetHeightAt(worldX, worldZ, height);
    return height;
}

bool EditorWorldSystem::GetHeightAt(float worldX, float worldZ, float& outHeight) {
    int32_t chunkX = WorldToChunkX(worldX);
    int32_t chunkZ = WorldToChunkZ(worldZ);

    int32_t key = MakeChunkKey(chunkX, chunkZ);
    auto it = m_Chunks.find(key);
    if (it == m_Chunks.end() || it->second->GetTerrain()->GetState() != ChunkState::Active) {
        outHeight = 0.0f;
        return false;
    }

    float localX = worldX - chunkX * CHUNK_SIZE;
    float localZ = worldZ - chunkZ * CHUNK_SIZE;

    outHeight = it->second->GetTerrain()->GetHeightAt(localX, localZ);
    return true;
}

void EditorWorldSystem::SetHeightAt(float worldX, float worldZ, float height) {
    int32_t chunkX = WorldToChunkX(worldX);
    int32_t chunkZ = WorldToChunkZ(worldZ);

    int32_t key = MakeChunkKey(chunkX, chunkZ);
    auto it = m_Chunks.find(key);
    if (it == m_Chunks.end() || it->second->GetTerrain()->GetState() != ChunkState::Active) return;
    TerrainChunk* terrain = it->second->GetTerrain();

    float localX = worldX - chunkX * CHUNK_SIZE;
    float localZ = worldZ - chunkZ * CHUNK_SIZE;

    int ix = static_cast<int>(localX / CHUNK_SIZE * (CHUNK_RESOLUTION - 1));
    int iz = static_cast<int>(localZ / CHUNK_SIZE * (CHUNK_RESOLUTION - 1));
    ix = std::clamp(ix, 0, CHUNK_RESOLUTION - 1);
    iz = std::clamp(iz, 0, CHUNK_RESOLUTION - 1);

    auto& data = terrain->GetDataMutable();
    if (data.heightmap.empty()) {
        data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
    }
    data.heightmap[iz * CHUNK_RESOLUTION + ix] = height;
    data.CalculateBounds();
}

void EditorWorldSystem::RaiseTerrain(float worldX, float worldZ, float radius, float amount) {
    TerrainBrush brush;
    brush.radius = radius;
    brush.strength = amount;

    ApplyBrush(worldX, worldZ, radius, [&brush](TerrainChunk* chunk, int localX, int localZ, float weight) {
        auto& data = chunk->GetDataMutable();
        if (data.heightmap.empty()) {
            data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
        }
        int idx = localZ * CHUNK_RESOLUTION + localX;
        data.heightmap[idx] += brush.GetWeight(weight);
    });
}

void EditorWorldSystem::LowerTerrain(float worldX, float worldZ, float radius, float amount) {
    RaiseTerrain(worldX, worldZ, radius, -amount);
}

float EditorWorldSystem::GetChunkHeight(int cx, int cz, int lx, int lz) const {
    const int last = CHUNK_RESOLUTION - 1;
    if (lx < 0) { cx--; lx = last + lx; }
    else if (lx > last) { cx++; lx = lx - last; }
    if (lz < 0) { cz--; lz = last + lz; }
    else if (lz > last) { cz++; lz = lz - last; }

    int32_t key = MakeChunkKey(cx, cz);
    auto it = m_Chunks.find(key);
    if (it == m_Chunks.end()) return 0.0f;
    const auto& data = it->second->GetTerrain()->GetData();
    if (data.heightmap.empty()) return 0.0f;
    return data.heightmap[lz * CHUNK_RESOLUTION + lx];
}

void EditorWorldSystem::SmoothTerrain(float worldX, float worldZ, float radius, float strength) {
    TerrainBrush brush;
    brush.radius = radius;
    brush.strength = strength;
    brush.falloff = TerrainBrush::Falloff::Smooth;

    struct SmoothVertex {
        TerrainChunk* chunk;
        int lx, lz;
        float average;
        float weight;
    };
    std::vector<SmoothVertex> vertices;

    int minChunkX = WorldToChunkX(worldX - radius);
    int maxChunkX = WorldToChunkX(worldX + radius);
    int minChunkZ = WorldToChunkZ(worldZ - radius);
    int maxChunkZ = WorldToChunkZ(worldZ + radius);

    if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ)) return;

    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
        for (int cx = minChunkX; cx <= maxChunkX; cx++) {
            TerrainChunk* terrain = m_Chunks[MakeChunkKey(cx, cz)]->GetTerrain();
            auto& data = terrain->GetData();
            if (data.heightmap.empty()) continue;

            float chunkWorldX = cx * CHUNK_SIZE;
            float chunkWorldZ = cz * CHUNK_SIZE;

            for (int lz = 0; lz < CHUNK_RESOLUTION; lz++) {
                for (int lx = 0; lx < CHUNK_RESOLUTION; lx++) {
                    float vertWorldX = chunkWorldX + (lx / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;
                    float vertWorldZ = chunkWorldZ + (lz / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;

                    float dx = vertWorldX - worldX;
                    float dz = vertWorldZ - worldZ;
                    float dist = std::sqrt(dx * dx + dz * dz);
                    if (dist > radius) continue;

                    float sum = GetChunkHeight(cx, cz, lx - 1, lz)
                              + GetChunkHeight(cx, cz, lx + 1, lz)
                              + GetChunkHeight(cx, cz, lx, lz - 1)
                              + GetChunkHeight(cx, cz, lx, lz + 1);
                    float avg = sum * 0.25f;

                    SmoothVertex sv;
                    sv.chunk = terrain;
                    sv.lx = lx;
                    sv.lz = lz;
                    sv.average = avg;
                    sv.weight = brush.GetWeight(dist);
                    vertices.push_back(sv);
                }
            }
        }
    }

    for (auto& sv : vertices) {
        auto& data = sv.chunk->GetDataMutable();
        int idx = sv.lz * CHUNK_RESOLUTION + sv.lx;
        data.heightmap[idx] = glm::mix(data.heightmap[idx], sv.average, sv.weight);
    }

    StitchEdges(minChunkX, maxChunkX, minChunkZ, maxChunkZ);

    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
        for (int cx = minChunkX; cx <= maxChunkX; cx++) {
            int32_t key = MakeChunkKey(cx, cz);
            auto it = m_Chunks.find(key);
            if (it != m_Chunks.end()) {
                it->second->GetTerrain()->GetDataMutable().CalculateBounds();
            }
            DirtyNeighborChunks(cx, cz);
        }
    }
}

void EditorWorldSystem::FlattenTerrain(float worldX, float worldZ, float radius, float targetHeight, float hardness) {
    float innerRadius = radius * std::clamp(hardness, 0.0f, 0.99f);
    float transitionWidth = radius - innerRadius;

    ApplyBrush(worldX, worldZ, radius, [targetHeight, innerRadius, transitionWidth](TerrainChunk* chunk, int localX, int localZ, float dist) {
        auto& data = chunk->GetDataMutable();
        if (data.heightmap.empty()) {
            data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
        }
        int idx = localZ * CHUNK_RESOLUTION + localX;
        float currentHeight = data.heightmap[idx];

        if (dist <= innerRadius) {
            data.heightmap[idx] = targetHeight;
        } else {
            float t = (dist - innerRadius) / transitionWidth;
            t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
            float desired = glm::mix(targetHeight, currentHeight, t);
            if (std::abs(desired - targetHeight) < std::abs(currentHeight - targetHeight)) {
                data.heightmap[idx] = desired;
            }
        }
    });
}

void EditorWorldSystem::RampTerrain(float startX, float startZ, float startHeight,
                                     float endX, float endZ, float endHeight, float width) {
    float dirX = endX - startX;
    float dirZ = endZ - startZ;
    float length = std::sqrt(dirX * dirX + dirZ * dirZ);
    if (length < 0.001f) return;

    float ndX = dirX / length;
    float ndZ = dirZ / length;
    float perpX = -ndZ;
    float perpZ = ndX;

    float minWX = std::min(startX, endX) - width;
    float maxWX = std::max(startX, endX) + width;
    float minWZ = std::min(startZ, endZ) - width;
    float maxWZ = std::max(startZ, endZ) + width;

    int minChunkX = WorldToChunkX(minWX);
    int maxChunkX = WorldToChunkX(maxWX);
    int minChunkZ = WorldToChunkZ(minWZ);
    int maxChunkZ = WorldToChunkZ(maxWZ);

    if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ)) return;

    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
        for (int cx = minChunkX; cx <= maxChunkX; cx++) {
            TerrainChunk* terrain = m_Chunks[MakeChunkKey(cx, cz)]->GetTerrain();

            float chunkWorldX = cx * CHUNK_SIZE;
            float chunkWorldZ = cz * CHUNK_SIZE;

            for (int lz = 0; lz < CHUNK_RESOLUTION; lz++) {
                for (int lx = 0; lx < CHUNK_RESOLUTION; lx++) {
                    float vertX = chunkWorldX + (lx / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;
                    float vertZ = chunkWorldZ + (lz / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;

                    float toVertX = vertX - startX;
                    float toVertZ = vertZ - startZ;

                    float t = (toVertX * ndX + toVertZ * ndZ) / length;
                    float d = std::abs(toVertX * perpX + toVertZ * perpZ);

                    if (t < 0.0f || t > 1.0f || d > width) continue;

                    float targetH = startHeight + t * (endHeight - startHeight);

                    float edgeFade = 1.0f;
                    float fadeZone = width * 0.3f;
                    if (d > width - fadeZone) {
                        edgeFade = 1.0f - (d - (width - fadeZone)) / fadeZone;
                    }

                    auto& data = terrain->GetDataMutable();
                    if (data.heightmap.empty()) {
                        data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
                    }
                    int idx = lz * CHUNK_RESOLUTION + lx;
                    data.heightmap[idx] = glm::mix(data.heightmap[idx], targetH, edgeFade);
                }
            }

            terrain->GetDataMutable().CalculateBounds();
        }
    }

    StitchEdges(minChunkX, maxChunkX, minChunkZ, maxChunkZ);

    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
        for (int cx = minChunkX; cx <= maxChunkX; cx++) {
            DirtyNeighborChunks(cx, cz);
        }
    }
}

void EditorWorldSystem::PaintTexture(float worldX, float worldZ, float radius, int layer, float strength) {
    if (layer < 0 || layer >= MAX_TERRAIN_LAYERS) return;
    PaintSplatmapLayer(worldX, worldZ, radius, layer, strength);
}

void EditorWorldSystem::PaintMaterial(float worldX, float worldZ, float radius,
                                       const std::string& materialId, float strength) {
    if (materialId.empty()) return;

    int globalLayer = ResolveGlobalLayer(materialId);

    int minChunkX = WorldToChunkX(worldX - radius);
    int maxChunkX = WorldToChunkX(worldX + radius);
    int minChunkZ = WorldToChunkZ(worldZ - radius);
    int maxChunkZ = WorldToChunkZ(worldZ + radius);

    if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ)) return;

    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
        for (int cx = minChunkX; cx <= maxChunkX; cx++) {
            TerrainChunk* terrain = m_Chunks[MakeChunkKey(cx, cz)]->GetTerrain();
            if (terrain->FindMaterialLayer(materialId) != globalLayer) {
                terrain->SetLayerMaterial(globalLayer, materialId);
            }
        }
    }

    PaintSplatmapLayer(worldX, worldZ, radius, globalLayer, strength);
}

void EditorWorldSystem::PaintSplatmapLayer(float worldX, float worldZ, float radius, int layer, float strength) {
    TerrainBrush brush;
    brush.radius = radius;
    brush.strength = strength;

    int minChunkX = WorldToChunkX(worldX - radius);
    int maxChunkX = WorldToChunkX(worldX + radius);
    int minChunkZ = WorldToChunkZ(worldZ - radius);
    int maxChunkZ = WorldToChunkZ(worldZ + radius);

    if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ)) return;

    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
        for (int cx = minChunkX; cx <= maxChunkX; cx++) {
            TerrainChunk* terrain = m_Chunks[MakeChunkKey(cx, cz)]->GetTerrain();

            float chunkWorldX = cx * CHUNK_SIZE;
            float chunkWorldZ = cz * CHUNK_SIZE;

            auto& data = terrain->GetSplatmapMutable();
            if (data.splatmap.empty()) {
                data.splatmap.resize(SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS, 0);
                for (int i = 0; i < SPLATMAP_TEXELS; i++) {
                    data.splatmap[i * MAX_TERRAIN_LAYERS + 0] = 255;
                }
            }

            for (int sz = 0; sz < SPLATMAP_RESOLUTION; sz++) {
                for (int sx = 0; sx < SPLATMAP_RESOLUTION; sx++) {
                    float texelWorldX = chunkWorldX + (sx + 0.5f) / (float)SPLATMAP_RESOLUTION * CHUNK_SIZE;
                    float texelWorldZ = chunkWorldZ + (sz + 0.5f) / (float)SPLATMAP_RESOLUTION * CHUNK_SIZE;

                    float dx = texelWorldX - worldX;
                    float dz = texelWorldZ - worldZ;
                    float dist = std::sqrt(dx * dx + dz * dz);
                    if (dist > radius) continue;

                    int idx = (sz * SPLATMAP_RESOLUTION + sx) * MAX_TERRAIN_LAYERS;
                    float weight = brush.GetWeight(dist);

                    float layerWeights[MAX_TERRAIN_LAYERS];
                    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
                        layerWeights[i] = data.splatmap[idx + i] / 255.0f;
                    }

                    layerWeights[layer] += weight;

                    float total = 0.0f;
                    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) total += layerWeights[i];
                    if (total > 0.0f) {
                        for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
                            data.splatmap[idx + i] = static_cast<uint8_t>((layerWeights[i] / total) * 255.0f);
                        }
                    }
                }
            }
        }
    }
}

void EditorWorldSystem::SetHole(float worldX, float worldZ, bool isHole) {
    int32_t chunkX = WorldToChunkX(worldX);
    int32_t chunkZ = WorldToChunkZ(worldZ);

    int32_t key = MakeChunkKey(chunkX, chunkZ);
    auto it = m_Chunks.find(key);
    if (it == m_Chunks.end() || it->second->GetTerrain()->GetState() != ChunkState::Active) return;
    TerrainChunk* terrain = it->second->GetTerrain();

    float localX = worldX - chunkX * CHUNK_SIZE;
    float localZ = worldZ - chunkZ * CHUNK_SIZE;

    int holeX = static_cast<int>(localX / CHUNK_SIZE * HOLE_GRID_SIZE);
    int holeZ = static_cast<int>(localZ / CHUNK_SIZE * HOLE_GRID_SIZE);
    holeX = std::clamp(holeX, 0, HOLE_GRID_SIZE - 1);
    holeZ = std::clamp(holeZ, 0, HOLE_GRID_SIZE - 1);

    auto& data = terrain->GetDataMutable();
    uint64_t bit = 1ULL << (holeZ * HOLE_GRID_SIZE + holeX);

    if (isHole) {
        data.holeMask |= bit;
    } else {
        data.holeMask &= ~bit;
    }
}

void EditorWorldSystem::EnsureChunkLoaded(int32_t chunkX, int32_t chunkZ) {
    WorldChunk* chunk = GetOrCreateChunk(chunkX, chunkZ);
    if (chunk && chunk->GetTerrain()->GetState() != ChunkState::Active) {
        LoadChunkFromDisk(chunk);
        chunk->GetTerrain()->CreateGPUResources();
    }
}

WorldChunk* EditorWorldSystem::CreateChunk(int32_t chunkX, int32_t chunkZ) {
    return GetOrCreateChunk(chunkX, chunkZ);
}

void EditorWorldSystem::DeleteChunk(int32_t chunkX, int32_t chunkZ) {
    int32_t key = MakeChunkKey(chunkX, chunkZ);
    auto it = m_Chunks.find(key);
    if (it != m_Chunks.end()) {
        std::string path = GetChunkFilePath(chunkX, chunkZ);
        std::remove(path.c_str());
        m_Chunks.erase(it);
        m_KnownChunkFiles.erase(key);
    }
}

void EditorWorldSystem::SaveDirtyChunks() {
    // Gather objects from EditorWorld into chunks before checking dirty flags.
    // This ensures chunks that gained/lost objects get marked dirty.
    if (m_EditorWorld) {
        // Find all chunks that currently contain objects
        std::unordered_set<int32_t> affectedChunks;
        for (const auto& obj : m_EditorWorld->GetStaticObjects()) {
            int32_t cx = WorldToChunkX(obj->GetPosition().x);
            int32_t cz = WorldToChunkZ(obj->GetPosition().z);
            affectedChunks.insert(MakeChunkKey(cx, cz));
        }
        // Also include chunks that previously had objects (object may have moved away)
        for (const auto& [guid, chunkKey] : m_ObjectChunkMap) {
            affectedChunks.insert(chunkKey);
        }
        // Gather objects for affected chunks, auto-creating if needed
        for (int32_t key : affectedChunks) {
            auto it = m_Chunks.find(key);
            if (it == m_Chunks.end()) {
                // Object in the void — create a default chunk to hold it
                int32_t cx = static_cast<int16_t>(static_cast<uint32_t>(key) >> 16);
                int32_t cz = static_cast<int16_t>(static_cast<uint32_t>(key) & 0xFFFF);
                GetOrCreateChunk(cx, cz);
                it = m_Chunks.find(key);
            }
            if (it != m_Chunks.end()) {
                GatherObjectsForChunk(it->second.get());
            }
        }
    }

    for (auto& [key, chunk] : m_Chunks) {
        if (chunk->IsModified()) {
            SaveChunkToDisk(chunk.get());
            chunk->ClearModified();
        }
    }
}

void EditorWorldSystem::SaveChunk(int32_t chunkX, int32_t chunkZ) {
    int32_t key = MakeChunkKey(chunkX, chunkZ);
    auto it = m_Chunks.find(key);
    if (it != m_Chunks.end()) {
        SaveChunkToDisk(it->second.get());
        it->second->ClearModified();
    }
}

void EditorWorldSystem::BeginEdit(float worldX, float worldZ, float radius) {
    m_CurrentEditSnapshot = std::make_unique<EditSnapshot>();

    int minChunkX = WorldToChunkX(worldX - radius);
    int maxChunkX = WorldToChunkX(worldX + radius);
    int minChunkZ = WorldToChunkZ(worldZ - radius);
    int maxChunkZ = WorldToChunkZ(worldZ + radius);

    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
        for (int cx = minChunkX; cx <= maxChunkX; cx++) {
            int32_t key = MakeChunkKey(cx, cz);
            auto it = m_Chunks.find(key);
            if (it != m_Chunks.end()) {
                m_CurrentEditSnapshot->chunkSnapshots.emplace_back(key, it->second->GetTerrain()->GetData());
            }
        }
    }
}

void EditorWorldSystem::EndEdit() {
    m_CurrentEditSnapshot.reset();
}

void EditorWorldSystem::CancelEdit() {
    if (!m_CurrentEditSnapshot) return;

    for (auto& [key, data] : m_CurrentEditSnapshot->chunkSnapshots) {
        auto it = m_Chunks.find(key);
        if (it != m_Chunks.end()) {
            it->second->GetTerrain()->LoadFromData(data);
        }
    }

    m_CurrentEditSnapshot.reset();
}

float EditorWorldSystem::CalculateChunkDistance(int32_t chunkX, int32_t chunkZ) const {
    float centerX = (chunkX + 0.5f) * CHUNK_SIZE;
    float centerZ = (chunkZ + 0.5f) * CHUNK_SIZE;
    float dx = centerX - m_CameraPosition.x;
    float dz = centerZ - m_CameraPosition.z;
    return std::sqrt(dx * dx + dz * dz);
}

bool EditorWorldSystem::IsChunkVisible(int32_t chunkX, int32_t chunkZ) const {
    int32_t key = MakeChunkKey(chunkX, chunkZ);
    auto it = m_Chunks.find(key);
    if (it == m_Chunks.end()) return false;

    TerrainChunk* terrain = it->second->GetTerrain();
    return m_Frustum.IsBoxVisible(terrain->GetBoundsMin(), terrain->GetBoundsMax());
}

void EditorWorldSystem::DirtyNeighborChunks(int32_t cx, int32_t cz) {
    static const int offsets[][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    for (auto& off : offsets) {
        auto it = m_Chunks.find(MakeChunkKey(cx + off[0], cz + off[1]));
        if (it != m_Chunks.end() && it->second->GetTerrain()->GetState() == ChunkState::Active) {
            it->second->GetTerrain()->MarkMeshDirty();
        }
    }
}

void EditorWorldSystem::ProcessLoadQueue() {
    int loaded = 0;
    while (!m_LoadQueue.empty() && loaded < m_Settings.maxChunksPerFrame) {
        auto req = m_LoadQueue.front();
        m_LoadQueue.pop_front();

        int32_t key = MakeChunkKey(req.chunkX, req.chunkZ);
        if (m_Chunks.find(key) != m_Chunks.end()) continue;

        auto chunk = std::make_unique<WorldChunk>(req.chunkX, req.chunkZ);
        TerrainChunk* terrain = chunk->GetTerrain();
        terrain->SetNormalMode(m_SobelNormals, m_SmoothNormals);
        terrain->SetDiamondGrid(m_DiamondGrid);
        terrain->SetMeshResolution(m_MeshResolution);
        terrain->SetHeightSampler([this](int cx, int cz, int lx, int lz) {
            return GetChunkHeight(cx, cz, lx, lz);
        });
        LoadChunkFromDisk(chunk.get());
        ApplyDefaultMaterials(chunk.get());
        m_Chunks[key] = std::move(chunk);
        CopyBoundaryFromNeighbors(m_Chunks[key].get());
        m_Chunks[key]->GetTerrain()->CreateGPUResources();
        m_Chunks[key]->ClearModified();
        DirtyNeighborChunks(req.chunkX, req.chunkZ);
        loaded++;
    }
}

void EditorWorldSystem::UnloadDistantChunks() {
    std::vector<int32_t> toUnload;

    for (auto& [key, chunk] : m_Chunks) {
        float dist = CalculateChunkDistance(chunk->GetChunkX(), chunk->GetChunkZ());
        if (dist > m_Settings.unloadDistance) {
            toUnload.push_back(key);
        }
    }

    for (int32_t key : toUnload) {
        auto& chunk = m_Chunks[key];
        if (chunk->IsModified()) {
            SaveChunkToDisk(chunk.get());
            chunk->ClearModified();
        }
        RemoveChunkObjects(key);
        m_Chunks.erase(key);
    }
}

std::string EditorWorldSystem::GetChunkFilePath(int32_t chunkX, int32_t chunkZ) const {
    return m_ProjectPath + "/chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".chunk";
}

void EditorWorldSystem::LoadChunkFromDisk(WorldChunk* chunk) {
    std::string path = GetChunkFilePath(chunk->GetChunkX(), chunk->GetChunkZ());
    chunk->Load(path);
    SpawnObjectsFromChunk(chunk);

    TerrainChunk* terrain = chunk->GetTerrain();
    auto& data = terrain->GetDataMutable();

    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
        if (!data.materialIds[i].empty()) {
            if (m_MaterialLayerMap.find(data.materialIds[i]) == m_MaterialLayerMap.end()) {
                m_MaterialLayerMap[data.materialIds[i]] = i;
            }
        }
    }

    if (!data.splatmap.empty()) {
        int remap[MAX_TERRAIN_LAYERS];
        for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) remap[i] = i;
        bool needsRemap = false;

        for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
            if (!data.materialIds[i].empty()) {
                auto it = m_MaterialLayerMap.find(data.materialIds[i]);
                if (it != m_MaterialLayerMap.end() && it->second != i) {
                    remap[i] = it->second;
                    needsRemap = true;
                }
            }
        }

        if (needsRemap) {
            std::vector<uint8_t> newSplatmap(data.splatmap.size(), 0);
            for (int i = 0; i < SPLATMAP_TEXELS; i++) {
                for (int c = 0; c < MAX_TERRAIN_LAYERS; c++) {
                    newSplatmap[i * MAX_TERRAIN_LAYERS + remap[c]] = data.splatmap[i * MAX_TERRAIN_LAYERS + c];
                }
            }
            data.splatmap = std::move(newSplatmap);

            std::string newIds[MAX_TERRAIN_LAYERS];
            for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
                newIds[remap[i]] = data.materialIds[i];
            }
            for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
                data.materialIds[i] = newIds[i];
            }

            terrain->ClearModified();
        }
    }
}

void EditorWorldSystem::SaveChunkToDisk(WorldChunk* chunk) {
    GatherObjectsForChunk(chunk);
    std::string path = GetChunkFilePath(chunk->GetChunkX(), chunk->GetChunkZ());
    chunk->Save(path);
    m_KnownChunkFiles.insert(MakeChunkKey(chunk->GetChunkX(), chunk->GetChunkZ()));
}

WorldChunk* EditorWorldSystem::GetOrCreateChunk(int32_t chunkX, int32_t chunkZ) {
    int32_t key = MakeChunkKey(chunkX, chunkZ);
    auto it = m_Chunks.find(key);
    if (it != m_Chunks.end()) {
        return it->second.get();
    }

    auto chunk = std::make_unique<WorldChunk>(chunkX, chunkZ);
    TerrainChunk* terrain = chunk->GetTerrain();
    terrain->SetNormalMode(m_SobelNormals, m_SmoothNormals);
    terrain->SetDiamondGrid(m_DiamondGrid);
    terrain->SetMeshResolution(m_MeshResolution);
    terrain->SetHeightSampler([this](int cx, int cz, int lx, int lz) {
        return GetChunkHeight(cx, cz, lx, lz);
    });
    LoadChunkFromDisk(chunk.get());
    ApplyDefaultMaterials(chunk.get());
    CopyBoundaryFromNeighbors(chunk.get());
    terrain->CreateGPUResources();
    chunk->ClearModified();

    m_KnownChunkFiles.insert(key);

    WorldChunk* ptr = chunk.get();
    m_Chunks[key] = std::move(chunk);
    DirtyNeighborChunks(chunkX, chunkZ);
    return ptr;
}

bool EditorWorldSystem::EnsureChunksReady(int minCX, int maxCX, int minCZ, int maxCZ) {
    for (int cz = minCZ; cz <= maxCZ; cz++) {
        for (int cx = minCX; cx <= maxCX; cx++) {
            WorldChunk* chunk = GetOrCreateChunk(cx, cz);
            if (!chunk || chunk->GetTerrain()->GetState() != ChunkState::Active) {
                return false;
            }
        }
    }
    return true;
}

void EditorWorldSystem::StitchEdges(int minCX, int maxCX, int minCZ, int maxCZ) {
    for (int cz = minCZ; cz <= maxCZ; cz++) {
        for (int cx = minCX; cx <= maxCX; cx++) {
            int32_t key = MakeChunkKey(cx, cz);
            auto it = m_Chunks.find(key);
            if (it == m_Chunks.end()) continue;
            auto& data = it->second->GetTerrain()->GetDataMutable();
            if (data.heightmap.empty()) continue;

            if (cx + 1 <= maxCX) {
                auto rightIt = m_Chunks.find(MakeChunkKey(cx + 1, cz));
                if (rightIt != m_Chunks.end() && !rightIt->second->GetTerrain()->GetData().heightmap.empty()) {
                    auto& rdata = rightIt->second->GetTerrain()->GetDataMutable();
                    for (int lz = 0; lz < CHUNK_RESOLUTION; lz++) {
                        int thisIdx = lz * CHUNK_RESOLUTION + (CHUNK_RESOLUTION - 1);
                        int rightIdx = lz * CHUNK_RESOLUTION;
                        float avg = (data.heightmap[thisIdx] + rdata.heightmap[rightIdx]) * 0.5f;
                        data.heightmap[thisIdx] = avg;
                        rdata.heightmap[rightIdx] = avg;
                    }
                }
            }

            if (cz + 1 <= maxCZ) {
                auto bottomIt = m_Chunks.find(MakeChunkKey(cx, cz + 1));
                if (bottomIt != m_Chunks.end() && !bottomIt->second->GetTerrain()->GetData().heightmap.empty()) {
                    auto& bdata = bottomIt->second->GetTerrain()->GetDataMutable();
                    for (int lx = 0; lx < CHUNK_RESOLUTION; lx++) {
                        int thisIdx = (CHUNK_RESOLUTION - 1) * CHUNK_RESOLUTION + lx;
                        int bottomIdx = lx;
                        float avg = (data.heightmap[thisIdx] + bdata.heightmap[bottomIdx]) * 0.5f;
                        data.heightmap[thisIdx] = avg;
                        bdata.heightmap[bottomIdx] = avg;
                    }
                }
            }
        }
    }
}

void EditorWorldSystem::CopyBoundaryFromNeighbors(WorldChunk* chunk) {
    TerrainChunk* terrain = chunk->GetTerrain();
    auto& data = terrain->GetDataMutable();
    if (data.heightmap.empty()) {
        data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
    }

    int32_t cx = chunk->GetChunkX();
    int32_t cz = chunk->GetChunkZ();
    const int last = CHUNK_RESOLUTION - 1;

    auto leftIt = m_Chunks.find(MakeChunkKey(cx - 1, cz));
    if (leftIt != m_Chunks.end()) {
        const auto& nd = leftIt->second->GetTerrain()->GetData();
        if (!nd.heightmap.empty()) {
            for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
                data.heightmap[lz * CHUNK_RESOLUTION + 0] = nd.heightmap[lz * CHUNK_RESOLUTION + last];
        }
    }

    auto rightIt = m_Chunks.find(MakeChunkKey(cx + 1, cz));
    if (rightIt != m_Chunks.end()) {
        const auto& nd = rightIt->second->GetTerrain()->GetData();
        if (!nd.heightmap.empty()) {
            for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
                data.heightmap[lz * CHUNK_RESOLUTION + last] = nd.heightmap[lz * CHUNK_RESOLUTION + 0];
        }
    }

    auto topIt = m_Chunks.find(MakeChunkKey(cx, cz - 1));
    if (topIt != m_Chunks.end()) {
        const auto& nd = topIt->second->GetTerrain()->GetData();
        if (!nd.heightmap.empty()) {
            for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
                data.heightmap[0 * CHUNK_RESOLUTION + lx] = nd.heightmap[last * CHUNK_RESOLUTION + lx];
        }
    }

    auto bottomIt = m_Chunks.find(MakeChunkKey(cx, cz + 1));
    if (bottomIt != m_Chunks.end()) {
        const auto& nd = bottomIt->second->GetTerrain()->GetData();
        if (!nd.heightmap.empty()) {
            for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
                data.heightmap[last * CHUNK_RESOLUTION + lx] = nd.heightmap[0 * CHUNK_RESOLUTION + lx];
        }
    }

    data.CalculateBounds();
}

// ---- Object ↔ Chunk Bridge ----

static ChunkObject StaticObjectToChunkObject(const MMO::StaticObject* obj) {
    ChunkObject co;
    co.guid = obj->GetGuid();
    co.name = obj->GetName();
    co.position = obj->GetPosition();
    co.rotation = obj->GetRotation();
    co.scale = obj->GetScale();
    co.parentGuid = obj->GetParentGuid();
    co.modelPath = obj->GetModelPath();
    co.materialId = obj->GetMaterialId();

    // Collider
    const auto& col = obj->GetCollider();
    co.colliderType = static_cast<uint8_t>(col.type);
    co.colliderCenter = col.center;
    co.colliderHalfExtents = col.halfExtents;
    co.colliderRadius = col.radius;
    co.colliderHeight = col.height;

    // Rendering flags
    co.castsShadow = obj->CastsShadow();
    co.receivesLightmap = obj->ReceivesLightmap();
    co.lightmapIndex = obj->GetLightmapIndex();
    co.lightmapScaleOffset = obj->GetLightmapScaleOffset();

    // Mesh materials
    for (const auto& [meshName, mat] : obj->GetAllMeshMaterials()) {
        ChunkObject::MeshMaterialEntry entry;
        entry.meshName = meshName;
        entry.materialId = mat.materialId;
        entry.positionOffset = mat.positionOffset;
        entry.rotationOffset = mat.rotationOffset;
        entry.scaleMultiplier = mat.scaleMultiplier;
        entry.visible = mat.visible;
        co.meshMaterials.push_back(std::move(entry));
    }

    // Animations
    co.animationPaths = obj->GetAnimationPaths();
    co.currentAnimation = obj->GetCurrentAnimation();
    co.animLoop = obj->GetAnimationLoop();
    co.animSpeed = obj->GetAnimationSpeed();

    return co;
}

static std::unique_ptr<MMO::StaticObject> ChunkObjectToStaticObject(const ChunkObject& co) {
    auto obj = std::make_unique<MMO::StaticObject>(co.guid, co.name);
    obj->SetPosition(co.position);
    obj->SetRotation(co.rotation);
    obj->SetScale(co.scale);
    obj->SetParent(co.parentGuid);
    obj->SetModelPath(co.modelPath);
    obj->SetMaterialId(co.materialId);

    // Collider
    MMO::ColliderData col;
    col.type = static_cast<MMO::ColliderType>(co.colliderType);
    col.center = co.colliderCenter;
    col.halfExtents = co.colliderHalfExtents;
    col.radius = co.colliderRadius;
    col.height = co.colliderHeight;
    obj->SetCollider(col);

    // Rendering flags
    obj->SetCastsShadow(co.castsShadow);
    obj->SetReceivesLightmap(co.receivesLightmap);
    obj->SetLightmapIndex(co.lightmapIndex);
    obj->SetLightmapScaleOffset(co.lightmapScaleOffset);

    // Mesh materials
    for (const auto& entry : co.meshMaterials) {
        MMO::MeshMaterial mat;
        mat.materialId = entry.materialId;
        mat.positionOffset = entry.positionOffset;
        mat.rotationOffset = entry.rotationOffset;
        mat.scaleMultiplier = entry.scaleMultiplier;
        mat.visible = entry.visible;
        obj->SetMeshMaterial(entry.meshName, mat);
    }

    // Animations
    for (const auto& path : co.animationPaths) {
        obj->AddAnimationPath(path);
    }
    obj->SetCurrentAnimation(co.currentAnimation);
    obj->SetAnimationLoop(co.animLoop);
    obj->SetAnimationSpeed(co.animSpeed);

    return obj;
}

void EditorWorldSystem::GatherObjectsForChunk(WorldChunk* chunk) {
    if (!m_EditorWorld) return;

    int32_t cx = chunk->GetChunkX();
    int32_t cz = chunk->GetChunkZ();
    chunk->GetObjects().clear();

    for (const auto& obj : m_EditorWorld->GetStaticObjects()) {
        int32_t objCX = WorldToChunkX(obj->GetPosition().x);
        int32_t objCZ = WorldToChunkZ(obj->GetPosition().z);
        if (objCX == cx && objCZ == cz) {
            chunk->GetObjects().push_back(StaticObjectToChunkObject(obj.get()));
        }
    }
}

void EditorWorldSystem::SpawnObjectsFromChunk(WorldChunk* chunk) {
    if (!m_EditorWorld) return;

    int32_t key = MakeChunkKey(chunk->GetChunkX(), chunk->GetChunkZ());

    for (const auto& co : chunk->GetObjects()) {
        // Skip if this GUID already exists in the world (e.g., re-loading same chunk)
        if (m_EditorWorld->GetObject(co.guid)) continue;

        auto obj = ChunkObjectToStaticObject(co);
        m_EditorWorld->EnsureGuidAbove(co.guid);
        m_EditorWorld->AddObject(std::move(obj));
        m_ObjectChunkMap[co.guid] = key;
    }
}

void EditorWorldSystem::RemoveChunkObjects(int32_t chunkKey) {
    if (!m_EditorWorld) return;

    std::vector<uint64_t> toRemove;
    for (auto& [guid, key] : m_ObjectChunkMap) {
        if (key == chunkKey) {
            toRemove.push_back(guid);
        }
    }

    for (uint64_t guid : toRemove) {
        m_EditorWorld->DeleteObject(guid);
        m_ObjectChunkMap.erase(guid);
    }
}

void EditorWorldSystem::ApplyBrush(float worldX, float worldZ, float radius,
    std::function<void(TerrainChunk*, int localX, int localZ, float weight)> operation) {

    int minChunkX = WorldToChunkX(worldX - radius);
    int maxChunkX = WorldToChunkX(worldX + radius);
    int minChunkZ = WorldToChunkZ(worldZ - radius);
    int maxChunkZ = WorldToChunkZ(worldZ + radius);

    if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ)) return;

    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
        for (int cx = minChunkX; cx <= maxChunkX; cx++) {
            int32_t key = MakeChunkKey(cx, cz);
            TerrainChunk* terrain = m_Chunks[key]->GetTerrain();

            float chunkWorldX = cx * CHUNK_SIZE;
            float chunkWorldZ = cz * CHUNK_SIZE;

            for (int lz = 0; lz < CHUNK_RESOLUTION; lz++) {
                for (int lx = 0; lx < CHUNK_RESOLUTION; lx++) {
                    float vertWorldX = chunkWorldX + (lx / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;
                    float vertWorldZ = chunkWorldZ + (lz / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;

                    float dx = vertWorldX - worldX;
                    float dz = vertWorldZ - worldZ;
                    float dist = std::sqrt(dx * dx + dz * dz);

                    operation(terrain, lx, lz, dist);
                }
            }

            terrain->GetDataMutable().CalculateBounds();
        }
    }

    StitchEdges(minChunkX, maxChunkX, minChunkZ, maxChunkZ);

    for (int cz = minChunkZ; cz <= maxChunkZ; cz++) {
        for (int cx = minChunkX; cx <= maxChunkX; cx++) {
            DirtyNeighborChunks(cx, cz);
        }
    }
}

} // namespace Editor3D
