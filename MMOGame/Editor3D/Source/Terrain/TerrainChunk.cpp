#include "TerrainChunk.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace Editor3D {

TerrainChunk::TerrainChunk(int32_t chunkX, int32_t chunkZ)
    : m_ChunkX(chunkX), m_ChunkZ(chunkZ) {
}

TerrainChunk::~TerrainChunk() {
    DestroyGPUResources();
}

void TerrainChunk::Load(const std::string& basePath) {
    std::ifstream file(basePath, std::ios::binary);
    if (!file.is_open()) {
        m_Data.chunkX = m_ChunkX;
        m_Data.chunkZ = m_ChunkZ;
        m_Data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
        m_Data.splatmap.resize(SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS, 0);
        for (int i = 0; i < SPLATMAP_TEXELS; i++) {
            m_Data.splatmap[i * MAX_TERRAIN_LAYERS] = 255;
        }
        m_Data.holeMask = 0;
        m_Data.minHeight = 0.0f;
        m_Data.maxHeight = 0.0f;
        m_State = ChunkState::Loaded;
        return;
    }

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x54455252) {
        file.close();
        return;
    }

    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 3) {
        file.close();
        return;
    }

    file.read(reinterpret_cast<char*>(&m_Data.chunkX), sizeof(m_Data.chunkX));
    file.read(reinterpret_cast<char*>(&m_Data.chunkZ), sizeof(m_Data.chunkZ));

    m_Data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE);
    file.read(reinterpret_cast<char*>(m_Data.heightmap.data()),
              CHUNK_HEIGHTMAP_SIZE * sizeof(float));

    m_Data.splatmap.resize(SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS);
    file.read(reinterpret_cast<char*>(m_Data.splatmap.data()),
              SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS);

    file.read(reinterpret_cast<char*>(&m_Data.holeMask), sizeof(m_Data.holeMask));

    file.read(reinterpret_cast<char*>(&m_Data.minHeight), sizeof(m_Data.minHeight));
    file.read(reinterpret_cast<char*>(&m_Data.maxHeight), sizeof(m_Data.maxHeight));

    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
        uint16_t len = 0;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (len > 0 && len < 256) {
            m_Data.materialIds[i].resize(len);
            file.read(m_Data.materialIds[i].data(), len);
        } else {
            m_Data.materialIds[i].clear();
        }
    }

    file.close();
    m_State = ChunkState::Loaded;
}

void TerrainChunk::LoadFromData(const TerrainChunkData& data) {
    m_Data = data;
    m_State = ChunkState::Loaded;
    m_Dirty = true;
}

void TerrainChunk::Unload() {
    DestroyGPUResources();
    m_Data.heightmap.clear();
    m_Data.splatmap.clear();
    m_State = ChunkState::Unloaded;
}

void TerrainChunk::CreateGPUResources() {
    if (m_State != ChunkState::Loaded) return;

    GenerateMesh();
    m_State = ChunkState::Active;
}

void TerrainChunk::DestroyGPUResources() {
    m_VAO.reset();
    m_VBO.reset();
    m_EBO.reset();
    m_HeightmapTexture.reset();
    m_SplatmapTexture0.reset();
    m_SplatmapTexture1.reset();

    if (m_State == ChunkState::Active) {
        m_State = ChunkState::Loaded;
    }
}

float TerrainChunk::GetHeightAt(float localX, float localZ) const {
    return MMO::GetTerrainHeight(m_Data, localX, localZ);
}

const std::string& TerrainChunk::GetLayerMaterial(int layer) const {
    static const std::string empty;
    if (layer < 0 || layer >= MAX_TERRAIN_LAYERS) return empty;
    return m_Data.materialIds[layer];
}

void TerrainChunk::SetLayerMaterial(int layer, const std::string& materialId) {
    if (layer < 0 || layer >= MAX_TERRAIN_LAYERS) return;
    m_Data.materialIds[layer] = materialId;
    m_Modified = true;
}

int TerrainChunk::FindMaterialLayer(const std::string& materialId) const {
    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
        if (m_Data.materialIds[i] == materialId) return i;
    }
    return -1;
}

int TerrainChunk::FindUnusedLayer() const {
    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
        if (m_Data.materialIds[i].empty()) return i;
    }
    return -1;
}

int TerrainChunk::FindLeastUsedLayer() const {
    if (m_Data.splatmap.empty()) return 0;

    float channelSums[MAX_TERRAIN_LAYERS] = {};
    for (int i = 0; i < SPLATMAP_TEXELS; i++) {
        for (int c = 0; c < MAX_TERRAIN_LAYERS; c++) {
            channelSums[c] += m_Data.splatmap[i * MAX_TERRAIN_LAYERS + c];
        }
    }

    int minLayer = 0;
    float minSum = channelSums[0];
    for (int i = 1; i < MAX_TERRAIN_LAYERS; i++) {
        if (channelSums[i] < minSum) {
            minSum = channelSums[i];
            minLayer = i;
        }
    }
    return minLayer;
}

void TerrainChunk::Draw(Shader* shader, bool allowRegenerate) {
    if (!m_VAO || m_State != ChunkState::Active) return;

    m_VAO->Bind();

    if (m_Dirty && allowRegenerate) {
        GenerateMesh();
        m_Dirty = false;
        m_SplatmapDirty = false;
    } else if (m_SplatmapDirty) {
        UpdateSplatmapTexture();
        m_SplatmapDirty = false;
    }

    if (m_HeightmapTexture) {
        m_HeightmapTexture->Bind(0);
        shader->SetInt("u_Heightmap", 0);
    }
    if (m_SplatmapTexture0) {
        m_SplatmapTexture0->Bind(1);
        shader->SetInt("u_Splatmap0", 1);
    }
    if (m_SplatmapTexture1) {
        m_SplatmapTexture1->Bind(2);
        shader->SetInt("u_Splatmap1", 2);
    }

    RenderCommand::DrawIndexed(*m_VAO, m_IndexCount);
    m_VAO->UnBind();
}

void TerrainChunk::SetNormalMode(bool sobel, bool smooth) {
    if (m_SobelNormals != sobel || m_SmoothNormals != smooth) {
        m_SobelNormals = sobel;
        m_SmoothNormals = smooth;
        m_Dirty = true;
    }
}

void TerrainChunk::SetDiamondGrid(bool enabled) {
    if (m_DiamondGrid != enabled) {
        m_DiamondGrid = enabled;
        m_Dirty = true;
    }
}

void TerrainChunk::SetMeshResolution(int resolution) {
    resolution = std::clamp(resolution, 3, 513);
    if (m_MeshResolution != resolution) {
        m_MeshResolution = resolution;
        m_Dirty = true;
    }
}

void TerrainChunk::UpdateSplatmapTexture() {
    if (m_Data.splatmap.empty()) return;

    std::vector<uint8_t> rgba0, rgba1;
    MMO::SplitSplatmapToRGBA(m_Data.splatmap, rgba0, rgba1);

    if (!m_SplatmapTexture0) {
        m_SplatmapTexture0 = Texture::CreateFromData(rgba0.data(), SPLATMAP_RESOLUTION, SPLATMAP_RESOLUTION, 4);
    } else {
        m_SplatmapTexture0->SetData(rgba0.data());
    }

    if (!m_SplatmapTexture1) {
        m_SplatmapTexture1 = Texture::CreateFromData(rgba1.data(), SPLATMAP_RESOLUTION, SPLATMAP_RESOLUTION, 4);
    } else {
        m_SplatmapTexture1->SetData(rgba1.data());
    }
}

void TerrainChunk::GenerateMesh() {
    if (m_Data.heightmap.empty()) return;

    // Use shared generator for CPU work
    MMO::TerrainMeshOptions opts;
    opts.meshResolution = m_MeshResolution;
    opts.sobelNormals = m_SobelNormals;
    opts.smoothNormals = m_SmoothNormals;
    opts.diamondGrid = m_DiamondGrid;
    opts.generatePaddedHeightmap = true;
    opts.heightSampler = m_HeightSampler;

    PreparedMeshData meshData;
    MMO::GenerateTerrainMesh(m_Data, opts, meshData);

    m_IndexCount = meshData.indexCount;

    // GPU upload
    if (!m_VAO) {
        RenderCommand::ResetState();

        m_VAO = std::make_unique<VertexArray>();
        m_VBO = std::make_unique<VertexBuffer>(meshData.vertices.data(), meshData.vertices.size() * sizeof(float));
        m_EBO = std::make_unique<IndexBuffer>(meshData.indices.data(), meshData.indices.size() * sizeof(uint32_t));

        VertexLayout layout({
            { VertexAttributeType::Float3 },
            { VertexAttributeType::Float3 },
            { VertexAttributeType::Float2 }
        });

        m_VAO->SetVertexBuffer(m_VBO.get());
        m_VAO->SetLayout(layout);
        m_VAO->SetIndexBuffer(m_EBO.get());
        m_VAO->UnBind();
    } else {
        m_VBO->Bind();
        m_VBO->SetData(meshData.vertices.data(), meshData.vertices.size() * sizeof(float));
        m_EBO->SetData(meshData.indices.data(), meshData.indices.size() * sizeof(uint32_t));
    }

    if (!meshData.paddedHeightmap.empty()) {
        int paddedRes = meshData.paddedHeightmapResolution;
        if (!m_HeightmapTexture) {
            m_HeightmapTexture = Texture::CreateFloatTexture(meshData.paddedHeightmap.data(), paddedRes, paddedRes);
        } else {
            m_HeightmapTexture->SetFloatData(meshData.paddedHeightmap.data());
        }
    }

    UpdateSplatmapTexture();
}

void TerrainChunk::PrepareMeshCPU(PreparedMeshData& out, const HeightSampler& heightSampler) const {
    MMO::TerrainMeshOptions opts;
    opts.meshResolution = m_MeshResolution;
    opts.sobelNormals = m_SobelNormals;
    opts.smoothNormals = m_SmoothNormals;
    opts.diamondGrid = m_DiamondGrid;
    opts.generatePaddedHeightmap = true;
    opts.heightSampler = heightSampler;
    MMO::GenerateTerrainMesh(m_Data, opts, out);
}

void TerrainChunk::UploadPreparedMesh(PreparedMeshData& data) {
    if (data.vertices.empty()) return;

    m_IndexCount = data.indexCount;

    if (!m_VAO) {
        RenderCommand::ResetState();

        m_VAO = std::make_unique<VertexArray>();
        m_VBO = std::make_unique<VertexBuffer>(data.vertices.data(), data.vertices.size() * sizeof(float));
        m_EBO = std::make_unique<IndexBuffer>(data.indices.data(), data.indices.size() * sizeof(uint32_t));

        VertexLayout layout({
            { VertexAttributeType::Float3 },
            { VertexAttributeType::Float3 },
            { VertexAttributeType::Float2 }
        });

        m_VAO->SetVertexBuffer(m_VBO.get());
        m_VAO->SetLayout(layout);
        m_VAO->SetIndexBuffer(m_EBO.get());
        m_VAO->UnBind();
    } else {
        m_VBO->Bind();
        m_VBO->SetData(data.vertices.data(), data.vertices.size() * sizeof(float));
        m_EBO->SetData(data.indices.data(), data.indices.size() * sizeof(uint32_t));
    }

    if (!data.paddedHeightmap.empty()) {
        int paddedRes = data.paddedHeightmapResolution;
        if (!m_HeightmapTexture) {
            m_HeightmapTexture = Texture::CreateFloatTexture(data.paddedHeightmap.data(), paddedRes, paddedRes);
        } else {
            m_HeightmapTexture->SetFloatData(data.paddedHeightmap.data());
        }
    }

    if (!data.splatmapRGBA0.empty()) {
        if (!m_SplatmapTexture0) {
            m_SplatmapTexture0 = Texture::CreateFromData(data.splatmapRGBA0.data(), SPLATMAP_RESOLUTION, SPLATMAP_RESOLUTION, 4);
        } else {
            m_SplatmapTexture0->SetData(data.splatmapRGBA0.data());
        }
    }
    if (!data.splatmapRGBA1.empty()) {
        if (!m_SplatmapTexture1) {
            m_SplatmapTexture1 = Texture::CreateFromData(data.splatmapRGBA1.data(), SPLATMAP_RESOLUTION, SPLATMAP_RESOLUTION, 4);
        } else {
            m_SplatmapTexture1->SetData(data.splatmapRGBA1.data());
        }
    }

    m_State = ChunkState::Active;
    m_Dirty = false;
    m_SplatmapDirty = false;

    // Free the CPU data now that it's on the GPU
    data.vertices.clear();
    data.vertices.shrink_to_fit();
    data.indices.clear();
    data.indices.shrink_to_fit();
    data.paddedHeightmap.clear();
    data.paddedHeightmap.shrink_to_fit();
    data.splatmapRGBA0.clear();
    data.splatmapRGBA0.shrink_to_fit();
    data.splatmapRGBA1.clear();
    data.splatmapRGBA1.shrink_to_fit();
}

} // namespace Editor3D
