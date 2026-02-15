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
    if (m_Data.heightmap.empty()) return 0.0f;

    float u = std::clamp(localX / CHUNK_SIZE, 0.0f, 1.0f);
    float v = std::clamp(localZ / CHUNK_SIZE, 0.0f, 1.0f);

    float fx = u * (CHUNK_RESOLUTION - 1);
    float fz = v * (CHUNK_RESOLUTION - 1);

    int x0 = static_cast<int>(fx);
    int z0 = static_cast<int>(fz);
    int x1 = std::min(x0 + 1, CHUNK_RESOLUTION - 1);
    int z1 = std::min(z0 + 1, CHUNK_RESOLUTION - 1);

    float fracX = fx - x0;
    float fracZ = fz - z0;

    float h00 = m_Data.heightmap[z0 * CHUNK_RESOLUTION + x0];
    float h10 = m_Data.heightmap[z0 * CHUNK_RESOLUTION + x1];
    float h01 = m_Data.heightmap[z1 * CHUNK_RESOLUTION + x0];
    float h11 = m_Data.heightmap[z1 * CHUNK_RESOLUTION + x1];

    float h0 = h00 + fracX * (h10 - h00);
    float h1 = h01 + fracX * (h11 - h01);

    return h0 + fracZ * (h1 - h0);
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

void TerrainChunk::Draw(Shader* shader) {
    if (!m_VAO || m_State != ChunkState::Active) return;

    m_VAO->Bind();

    if (m_Dirty) {
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

    std::vector<uint8_t> rgba0(SPLATMAP_TEXELS * 4);
    std::vector<uint8_t> rgba1(SPLATMAP_TEXELS * 4);

    for (int i = 0; i < SPLATMAP_TEXELS; i++) {
        int src = i * MAX_TERRAIN_LAYERS;
        rgba0[i * 4 + 0] = m_Data.splatmap[src + 0];
        rgba0[i * 4 + 1] = m_Data.splatmap[src + 1];
        rgba0[i * 4 + 2] = m_Data.splatmap[src + 2];
        rgba0[i * 4 + 3] = m_Data.splatmap[src + 3];

        rgba1[i * 4 + 0] = m_Data.splatmap[src + 4];
        rgba1[i * 4 + 1] = m_Data.splatmap[src + 5];
        rgba1[i * 4 + 2] = m_Data.splatmap[src + 6];
        rgba1[i * 4 + 3] = m_Data.splatmap[src + 7];
    }

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

    const int dataRes = CHUNK_RESOLUTION;
    const int dataQuads = dataRes - 1;
    const int meshRes = m_MeshResolution;
    const int meshQuads = meshRes - 1;

    int totalCornerVerts = meshRes * meshRes;
    int totalCenterVerts = m_DiamondGrid ? (meshQuads * meshQuads) : 0;
    int totalVerts = totalCornerVerts + totalCenterVerts;
    int trisPerQuad = m_DiamondGrid ? 4 : 2;

    std::vector<float> vertices;
    vertices.reserve(totalVerts * 8);

    std::vector<uint32_t> indices;
    indices.reserve(meshQuads * meshQuads * trisPerQuad * 3);

    float meshStep = CHUNK_SIZE / (float)meshQuads;
    float dataStep = CHUNK_SIZE / (float)dataQuads;

    auto sampleHeightInt = [&](int sx, int sz) -> float {
        if (sx >= 0 && sx < dataRes && sz >= 0 && sz < dataRes) {
            return m_Data.heightmap[sz * dataRes + sx];
        }
        if (m_HeightSampler) {
            return m_HeightSampler(m_ChunkX, m_ChunkZ, sx, sz);
        }
        sx = std::clamp(sx, 0, dataRes - 1);
        sz = std::clamp(sz, 0, dataRes - 1);
        return m_Data.heightmap[sz * dataRes + sx];
    };

    auto sampleHeightBilinear = [&](float fx, float fz) -> float {
        fx = std::clamp(fx, 0.0f, (float)(dataRes - 1));
        fz = std::clamp(fz, 0.0f, (float)(dataRes - 1));
        int x0 = std::min((int)fx, dataRes - 2);
        int z0 = std::min((int)fz, dataRes - 2);
        float tx = fx - x0;
        float tz = fz - z0;
        float h00 = m_Data.heightmap[z0 * dataRes + x0];
        float h10 = m_Data.heightmap[z0 * dataRes + x0 + 1];
        float h01 = m_Data.heightmap[(z0 + 1) * dataRes + x0];
        float h11 = m_Data.heightmap[(z0 + 1) * dataRes + x0 + 1];
        return h00 * (1 - tx) * (1 - tz) + h10 * tx * (1 - tz) +
               h01 * (1 - tx) * tz + h11 * tx * tz;
    };

    const int dataTotalVerts = dataRes * dataRes;
    std::vector<glm::vec3> dataNormals(dataTotalVerts);

    for (int z = 0; z < dataRes; z++) {
        for (int x = 0; x < dataRes; x++) {
            glm::vec3 normal;

            if (m_SobelNormals) {
                float h00 = sampleHeightInt(x - 1, z - 1);
                float h10 = sampleHeightInt(x,     z - 1);
                float h20 = sampleHeightInt(x + 1, z - 1);
                float h01 = sampleHeightInt(x - 1, z    );
                float h21 = sampleHeightInt(x + 1, z    );
                float h02 = sampleHeightInt(x - 1, z + 1);
                float h12 = sampleHeightInt(x,     z + 1);
                float h22 = sampleHeightInt(x + 1, z + 1);

                float gx = -h00 + h20 - 2.0f * h01 + 2.0f * h21 - h02 + h22;
                float gz =  h00 + 2.0f * h10 + h20 - h02 - 2.0f * h12 - h22;

                normal = glm::normalize(glm::vec3(-gx, 8.0f * dataStep, -gz));
            } else {
                float hL = sampleHeightInt(x - 1, z);
                float hR = sampleHeightInt(x + 1, z);
                float hD = sampleHeightInt(x, z - 1);
                float hU = sampleHeightInt(x, z + 1);

                normal = glm::normalize(glm::vec3(hL - hR, 2.0f * dataStep, hD - hU));
            }

            dataNormals[z * dataRes + x] = normal;
        }
    }

    if (m_SmoothNormals) {
        auto computeSobelAt = [&](int sx, int sz) -> glm::vec3 {
            float h00 = sampleHeightInt(sx - 1, sz - 1);
            float h10 = sampleHeightInt(sx,     sz - 1);
            float h20 = sampleHeightInt(sx + 1, sz - 1);
            float h01 = sampleHeightInt(sx - 1, sz    );
            float h21 = sampleHeightInt(sx + 1, sz    );
            float h02 = sampleHeightInt(sx - 1, sz + 1);
            float h12 = sampleHeightInt(sx,     sz + 1);
            float h22 = sampleHeightInt(sx + 1, sz + 1);
            float gx = -h00 + h20 - 2.0f * h01 + 2.0f * h21 - h02 + h22;
            float gz =  h00 + 2.0f * h10 + h20 - h02 - 2.0f * h12 - h22;
            return glm::normalize(glm::vec3(-gx, 8.0f * dataStep, -gz));
        };

        auto getNormal = [&](int nx, int nz) -> glm::vec3 {
            if (nx >= 0 && nx < dataRes && nz >= 0 && nz < dataRes)
                return dataNormals[nz * dataRes + nx];
            return computeSobelAt(nx, nz);
        };

        std::vector<glm::vec3> smoothed(dataTotalVerts);

        for (int z = 0; z < dataRes; z++) {
            for (int x = 0; x < dataRes; x++) {
                glm::vec3 sum = dataNormals[z * dataRes + x];
                sum += getNormal(x - 1, z);
                sum += getNormal(x + 1, z);
                sum += getNormal(x, z - 1);
                sum += getNormal(x, z + 1);
                smoothed[z * dataRes + x] = glm::normalize(sum / 5.0f);
            }
        }

        dataNormals = std::move(smoothed);
    }

    auto sampleNormalBilinear = [&](float fx, float fz) -> glm::vec3 {
        fx = std::clamp(fx, 0.0f, (float)(dataRes - 1));
        fz = std::clamp(fz, 0.0f, (float)(dataRes - 1));
        int x0 = std::min((int)fx, dataRes - 2);
        int z0 = std::min((int)fz, dataRes - 2);
        float tx = fx - x0;
        float tz = fz - z0;
        glm::vec3 n00 = dataNormals[z0 * dataRes + x0];
        glm::vec3 n10 = dataNormals[z0 * dataRes + x0 + 1];
        glm::vec3 n01 = dataNormals[(z0 + 1) * dataRes + x0];
        glm::vec3 n11 = dataNormals[(z0 + 1) * dataRes + x0 + 1];
        return glm::normalize(n00 * (1 - tx) * (1 - tz) + n10 * tx * (1 - tz) +
                              n01 * (1 - tx) * tz + n11 * tx * tz);
    };

    float worldOriginX = m_ChunkX * CHUNK_SIZE;
    float worldOriginZ = m_ChunkZ * CHUNK_SIZE;

    for (int z = 0; z < meshRes; z++) {
        for (int x = 0; x < meshRes; x++) {
            float hx = x * (float)dataQuads / meshQuads;
            float hz = z * (float)dataQuads / meshQuads;

            float px = worldOriginX + x * meshStep;
            float pz = worldOriginZ + z * meshStep;
            float py = sampleHeightBilinear(hx, hz);

            vertices.push_back(px);
            vertices.push_back(py);
            vertices.push_back(pz);

            glm::vec3 n = sampleNormalBilinear(hx, hz);
            vertices.push_back(n.x);
            vertices.push_back(n.y);
            vertices.push_back(n.z);

            vertices.push_back(static_cast<float>(x) / meshQuads);
            vertices.push_back(static_cast<float>(z) / meshQuads);
        }
    }

    if (m_DiamondGrid) {
        for (int z = 0; z < meshQuads; z++) {
            for (int x = 0; x < meshQuads; x++) {
                float hx = (x + 0.5f) * (float)dataQuads / meshQuads;
                float hz = (z + 0.5f) * (float)dataQuads / meshQuads;

                float cx = worldOriginX + (x + 0.5f) * meshStep;
                float cz = worldOriginZ + (z + 0.5f) * meshStep;
                float cy = sampleHeightBilinear(hx, hz);

                vertices.push_back(cx);
                vertices.push_back(cy);
                vertices.push_back(cz);

                glm::vec3 cn = sampleNormalBilinear(hx, hz);
                vertices.push_back(cn.x);
                vertices.push_back(cn.y);
                vertices.push_back(cn.z);

                vertices.push_back((x + 0.5f) / meshQuads);
                vertices.push_back((z + 0.5f) / meshQuads);
            }
        }
    }

    for (int z = 0; z < meshQuads; z++) {
        for (int x = 0; x < meshQuads; x++) {
            int holeX = std::min(x * HOLE_GRID_SIZE / meshQuads, HOLE_GRID_SIZE - 1);
            int holeZ = std::min(z * HOLE_GRID_SIZE / meshQuads, HOLE_GRID_SIZE - 1);
            uint64_t holeBit = 1ULL << (holeZ * HOLE_GRID_SIZE + holeX);
            if (m_Data.holeMask & holeBit) continue;

            uint32_t TL = z * meshRes + x;
            uint32_t TR = TL + 1;
            uint32_t BL = (z + 1) * meshRes + x;
            uint32_t BR = BL + 1;

            if (m_DiamondGrid) {
                uint32_t C = totalCornerVerts + z * meshQuads + x;

                indices.push_back(TL); indices.push_back(C);  indices.push_back(TR);
                indices.push_back(TR); indices.push_back(C);  indices.push_back(BR);
                indices.push_back(BR); indices.push_back(C);  indices.push_back(BL);
                indices.push_back(BL); indices.push_back(C);  indices.push_back(TL);
            } else {
                indices.push_back(TL); indices.push_back(BL); indices.push_back(TR);
                indices.push_back(TR); indices.push_back(BL); indices.push_back(BR);
            }
        }
    }

    m_IndexCount = static_cast<uint32_t>(indices.size());

    if (!m_VAO) {
        RenderCommand::ResetState();

        m_VAO = std::make_unique<VertexArray>();
        m_VBO = std::make_unique<VertexBuffer>(vertices.data(), vertices.size() * sizeof(float));
        m_EBO = std::make_unique<IndexBuffer>(indices.data(), indices.size() * sizeof(uint32_t));

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
        m_VBO->SetData(vertices.data(), vertices.size() * sizeof(float));
        m_EBO->SetData(indices.data(), indices.size() * sizeof(uint32_t));
    }

    if (!m_Data.heightmap.empty()) {
        const int paddedRes = CHUNK_RESOLUTION + 2;
        std::vector<float> padded(paddedRes * paddedRes);

        for (int z = 0; z < CHUNK_RESOLUTION; z++)
            for (int x = 0; x < CHUNK_RESOLUTION; x++)
                padded[(z + 1) * paddedRes + (x + 1)] = m_Data.heightmap[z * CHUNK_RESOLUTION + x];

        for (int z = -1; z <= CHUNK_RESOLUTION; z++) {
            padded[(z + 1) * paddedRes + 0] = sampleHeightInt(-1, z);
            padded[(z + 1) * paddedRes + (paddedRes - 1)] = sampleHeightInt(CHUNK_RESOLUTION, z);
        }
        for (int x = 0; x < CHUNK_RESOLUTION; x++) {
            padded[0 * paddedRes + (x + 1)] = sampleHeightInt(x, -1);
            padded[(paddedRes - 1) * paddedRes + (x + 1)] = sampleHeightInt(x, CHUNK_RESOLUTION);
        }

        if (!m_HeightmapTexture) {
            m_HeightmapTexture = Texture::CreateFloatTexture(padded.data(), paddedRes, paddedRes);
        } else {
            m_HeightmapTexture->SetFloatData(padded.data());
        }
    }

    UpdateSplatmapTexture();
}

} // namespace Editor3D
