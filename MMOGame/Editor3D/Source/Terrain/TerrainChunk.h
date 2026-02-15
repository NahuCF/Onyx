#pragma once

#include <Onyx.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace Editor3D {

using namespace Onyx;

constexpr float CHUNK_SIZE = 64.0f;
constexpr int CHUNK_RESOLUTION = 65;
constexpr int CHUNK_HEIGHTMAP_SIZE = CHUNK_RESOLUTION * CHUNK_RESOLUTION;
constexpr int HOLE_GRID_SIZE = 8;
constexpr int MAX_TERRAIN_LAYERS = 8;
constexpr int SPLATMAP_RESOLUTION = 64;
constexpr int SPLATMAP_TEXELS = SPLATMAP_RESOLUTION * SPLATMAP_RESOLUTION;

enum class ChunkState {
    Unloaded,
    Loaded,
    Active
};

struct TerrainChunkData {
    int32_t chunkX = 0;
    int32_t chunkZ = 0;

    std::vector<float> heightmap;
    std::vector<uint8_t> splatmap;
    uint64_t holeMask = 0;

    std::string materialIds[MAX_TERRAIN_LAYERS];

    float minHeight = 0.0f;
    float maxHeight = 0.0f;

    void CalculateBounds() {
        if (heightmap.empty()) return;
        minHeight = heightmap[0];
        maxHeight = heightmap[0];
        for (float h : heightmap) {
            minHeight = std::min(minHeight, h);
            maxHeight = std::max(maxHeight, h);
        }
    }
};

class TerrainChunk {
public:
    TerrainChunk(int32_t chunkX, int32_t chunkZ);
    ~TerrainChunk();

    int32_t GetChunkX() const { return m_ChunkX; }
    int32_t GetChunkZ() const { return m_ChunkZ; }

    glm::vec3 GetWorldPosition() const {
        return glm::vec3(m_ChunkX * CHUNK_SIZE, 0.0f, m_ChunkZ * CHUNK_SIZE);
    }

    glm::vec3 GetBoundsMin() const {
        return glm::vec3(m_ChunkX * CHUNK_SIZE, m_Data.minHeight, m_ChunkZ * CHUNK_SIZE);
    }
    glm::vec3 GetBoundsMax() const {
        return glm::vec3((m_ChunkX + 1) * CHUNK_SIZE, m_Data.maxHeight, (m_ChunkZ + 1) * CHUNK_SIZE);
    }

    ChunkState GetState() const { return m_State; }
    bool IsReady() const { return m_State == ChunkState::Active; }

    void Load(const std::string& basePath);
    void LoadFromData(const TerrainChunkData& data);
    void Unload();
    void CreateGPUResources();
    void DestroyGPUResources();

    const TerrainChunkData& GetData() const { return m_Data; }
    TerrainChunkData& GetDataMutable() { m_Dirty = true; m_Modified = true; return m_Data; }
    TerrainChunkData& GetSplatmapMutable() { m_SplatmapDirty = true; m_Modified = true; return m_Data; }

    float GetHeightAt(float localX, float localZ) const;

    const std::string& GetLayerMaterial(int layer) const;
    void SetLayerMaterial(int layer, const std::string& materialId);
    int FindMaterialLayer(const std::string& materialId) const;
    int FindUnusedLayer() const;
    int FindLeastUsedLayer() const;

    void MarkSplatmapDirty() { m_SplatmapDirty = true; m_Modified = true; }

    void Draw(Shader* shader);

    bool IsDirty() const { return m_Dirty; }
    void ClearDirty() { m_Dirty = false; }

    bool IsModified() const { return m_Modified; }
    void ClearModified() { m_Modified = false; }

    void SetNormalMode(bool sobel, bool smooth);
    void SetDiamondGrid(bool enabled);
    void SetMeshResolution(int resolution);
    int GetMeshResolution() const { return m_MeshResolution; }

    using HeightSampler = std::function<float(int, int, int, int)>;
    void SetHeightSampler(HeightSampler sampler) { m_HeightSampler = std::move(sampler); }

private:
    int32_t m_ChunkX, m_ChunkZ;
    ChunkState m_State = ChunkState::Unloaded;
    bool m_Dirty = false;
    bool m_Modified = false;
    bool m_SplatmapDirty = false;
    bool m_SobelNormals = true;
    bool m_SmoothNormals = true;
    bool m_DiamondGrid = true;
    int m_MeshResolution = CHUNK_RESOLUTION;
    HeightSampler m_HeightSampler;

    TerrainChunkData m_Data;

    std::unique_ptr<VertexArray> m_VAO;
    std::unique_ptr<VertexBuffer> m_VBO;
    std::unique_ptr<IndexBuffer> m_EBO;
    std::unique_ptr<Texture> m_HeightmapTexture;
    std::unique_ptr<Texture> m_SplatmapTexture0;
    std::unique_ptr<Texture> m_SplatmapTexture1;

    uint32_t m_IndexCount = 0;

    void GenerateMesh();
    void UpdateSplatmapTexture();
};

} // namespace Editor3D
