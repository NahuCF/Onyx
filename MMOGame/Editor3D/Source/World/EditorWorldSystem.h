#pragma once

#include "WorldChunk.h"
#include <Onyx.h>
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <memory>
#include <functional>
#include <unordered_set>

namespace Editor3D {

using namespace Onyx;

struct ChunkLoadRequest {
    int32_t chunkX;
    int32_t chunkZ;
    float priority;
};

class EditorWorldSystem {
public:
    EditorWorldSystem();
    ~EditorWorldSystem();

    using PerChunkCallback = std::function<void(TerrainChunk*, Shader*)>;

    void Init(const std::string& chunksDirectory);
    void Shutdown();
    void Update(const glm::vec3& cameraPos, const glm::mat4& viewProj, float deltaTime);
    void RenderTerrain(Shader* terrainShader, const glm::mat4& viewProj,
                       PerChunkCallback perChunkSetup = nullptr);

    // Light gathering from all loaded chunks
    std::vector<EditorLight> GatherVisibleLights(const glm::vec3& cameraPos, float maxDistance = 200.0f) const;

    // Terrain queries
    float GetHeightAt(float worldX, float worldZ);
    bool GetHeightAt(float worldX, float worldZ, float& outHeight);

    // Terrain editing
    void SetHeightAt(float worldX, float worldZ, float height);
    void RaiseTerrain(float worldX, float worldZ, float radius, float amount);
    void LowerTerrain(float worldX, float worldZ, float radius, float amount);
    void SmoothTerrain(float worldX, float worldZ, float radius, float strength);
    void FlattenTerrain(float worldX, float worldZ, float radius, float targetHeight, float hardness = 0.5f);
    void RampTerrain(float startX, float startZ, float startHeight,
                     float endX, float endZ, float endHeight, float width);
    void PaintTexture(float worldX, float worldZ, float radius, int layer, float strength);
    void PaintMaterial(float worldX, float worldZ, float radius,
                       const std::string& materialId, float strength);
    void SetHole(float worldX, float worldZ, bool isHole);

    // Chunk management
    void EnsureChunkLoaded(int32_t chunkX, int32_t chunkZ);
    WorldChunk* CreateChunk(int32_t chunkX, int32_t chunkZ);
    void DeleteChunk(int32_t chunkX, int32_t chunkZ);
    void SaveDirtyChunks();
    void SaveChunk(int32_t chunkX, int32_t chunkZ);

    // Edit undo/redo
    void BeginEdit(float worldX, float worldZ, float radius);
    void EndEdit();
    void CancelEdit();

    // Settings
    void SetDefaultMaterialIds(const std::string ids[MAX_TERRAIN_LAYERS]);
    void SetNormalMode(bool sobel, bool smooth);
    void SetDiamondGrid(bool enabled);
    void SetMeshResolution(int resolution);

    struct StreamingSettings {
        float loadDistance = 192.0f;
        float unloadDistance = 256.0f;
        int maxChunksPerFrame = 4;
        bool enableFrustumCulling = true;
    };

    StreamingSettings& GetSettings() { return m_Settings; }

    struct Stats {
        int totalChunks = 0;
        int loadedChunks = 0;
        int visibleChunks = 0;
        int dirtyChunks = 0;
    };

    const Stats& GetStats() const { return m_Stats; }
    const std::unordered_map<int32_t, std::unique_ptr<WorldChunk>>& GetChunks() const { return m_Chunks; }
    std::unordered_map<int32_t, std::unique_ptr<WorldChunk>>& GetChunksMutable() { return m_Chunks; }

    WorldChunk* GetChunk(int32_t cx, int32_t cz);

    static int32_t MakeChunkKeyPublic(int32_t x, int32_t z) {
        return static_cast<int32_t>((static_cast<uint32_t>(x) << 16) | (static_cast<uint32_t>(z) & 0xFFFF));
    }

private:
    std::string m_ProjectPath;
    StreamingSettings m_Settings;
    Stats m_Stats;

    std::unordered_map<int32_t, std::unique_ptr<WorldChunk>> m_Chunks;
    std::unordered_set<int32_t> m_KnownChunkFiles;
    std::deque<ChunkLoadRequest> m_LoadQueue;

    glm::vec3 m_CameraPosition;
    Frustum m_Frustum;

    bool m_SobelNormals = true;
    bool m_SmoothNormals = true;
    bool m_DiamondGrid = true;
    int m_MeshResolution = 65;

    std::string m_DefaultMaterialIds[MAX_TERRAIN_LAYERS];
    std::unordered_map<std::string, int> m_MaterialLayerMap;

    struct EditSnapshot {
        std::vector<std::pair<int32_t, TerrainChunkData>> chunkSnapshots;
    };
    std::unique_ptr<EditSnapshot> m_CurrentEditSnapshot;

    static int32_t MakeChunkKey(int32_t x, int32_t z) {
        return static_cast<int32_t>((static_cast<uint32_t>(x) << 16) | (static_cast<uint32_t>(z) & 0xFFFF));
    }

    void ApplyDefaultMaterials(WorldChunk* chunk);
    int ResolveGlobalLayer(const std::string& materialId);

    float CalculateChunkDistance(int32_t chunkX, int32_t chunkZ) const;
    bool IsChunkVisible(int32_t chunkX, int32_t chunkZ) const;

    void ProcessLoadQueue();
    void UnloadDistantChunks();

    std::string GetChunkFilePath(int32_t chunkX, int32_t chunkZ) const;
    void LoadChunkFromDisk(WorldChunk* chunk);
    void SaveChunkToDisk(WorldChunk* chunk);

    WorldChunk* GetOrCreateChunk(int32_t chunkX, int32_t chunkZ);
    bool EnsureChunksReady(int minCX, int maxCX, int minCZ, int maxCZ);

    void ApplyBrush(float worldX, float worldZ, float radius,
                    std::function<void(TerrainChunk*, int localX, int localZ, float weight)> operation);

    void StitchEdges(int minCX, int maxCX, int minCZ, int maxCZ);
    void CopyBoundaryFromNeighbors(WorldChunk* chunk);
    float GetChunkHeight(int cx, int cz, int lx, int lz) const;
    void DirtyNeighborChunks(int32_t cx, int32_t cz);
    void PaintSplatmapLayer(float worldX, float worldZ, float radius, int layer, float strength);
};

struct TerrainBrush {
    enum class Falloff { Linear, Smooth, Constant };

    Falloff falloff = Falloff::Smooth;
    float radius = 5.0f;
    float strength = 1.0f;

    float GetWeight(float distance) const {
        if (distance >= radius) return 0.0f;

        float t = distance / radius;

        switch (falloff) {
        case Falloff::Constant:
            return strength;
        case Falloff::Linear:
            return strength * (1.0f - t);
        case Falloff::Smooth:
            t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
            return strength * (1.0f - t);
        }
        return 0.0f;
    }
};

} // namespace Editor3D
