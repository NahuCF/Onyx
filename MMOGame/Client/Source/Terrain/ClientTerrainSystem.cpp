#include "ClientTerrainSystem.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace MMO {

void ClientTerrainSystem::LoadZone(uint32_t mapId, const std::string& basePath) {
    UnloadZone();
    m_MapId = mapId;

    std::string chunksDir = basePath + "/chunks";

    if (!std::filesystem::exists(chunksDir)) {
        std::cout << "[ClientTerrain] No chunks directory: " << chunksDir << std::endl;
        return;
    }

    int loadedCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(chunksDir)) {
        if (entry.path().extension() != ".chunk") continue;

        ChunkFileData fileData;
        if (!LoadChunkFile(entry.path().string(), fileData)) {
            std::cout << "[ClientTerrain] Failed to load: " << entry.path() << std::endl;
            continue;
        }

        auto chunk = std::make_unique<ClientTerrainChunk>();
        chunk->data = std::move(fileData.terrain);
        chunk->data.CalculateBounds();

        CreateChunkGPU(*chunk);

        int64_t key = PackKey(chunk->data.chunkX, chunk->data.chunkZ);
        m_Chunks[key] = std::move(chunk);
        loadedCount++;
    }

    std::cout << "[ClientTerrain] Loaded " << loadedCount << " chunks for map " << mapId << std::endl;
}

void ClientTerrainSystem::UnloadZone() {
    m_Chunks.clear();
    m_MapId = 0;
}

void ClientTerrainSystem::CreateChunkGPU(ClientTerrainChunk& chunk) {
    GenerateMesh(chunk);
    UploadSplatmapTextures(chunk);
}

void ClientTerrainSystem::GenerateMesh(ClientTerrainChunk& chunk) {
    const auto& data = chunk.data;
    if (data.heightmap.empty()) return;

    const int res = TERRAIN_CHUNK_RESOLUTION;
    const int quads = res - 1;

    std::vector<float> vertices;
    vertices.reserve(res * res * 8);

    std::vector<uint32_t> indices;
    indices.reserve(quads * quads * 6);

    float worldOriginX = data.chunkX * TERRAIN_CHUNK_SIZE;
    float worldOriginZ = data.chunkZ * TERRAIN_CHUNK_SIZE;
    float step = TERRAIN_CHUNK_SIZE / static_cast<float>(quads);

    // Generate vertices with Sobel normals
    for (int z = 0; z < res; z++) {
        for (int x = 0; x < res; x++) {
            float px = worldOriginX + x * step;
            float pz = worldOriginZ + z * step;
            float py = data.heightmap[z * res + x];

            // Sobel normal
            auto sampleH = [&](int sx, int sz) -> float {
                sx = std::clamp(sx, 0, res - 1);
                sz = std::clamp(sz, 0, res - 1);
                return data.heightmap[sz * res + sx];
            };

            float h00 = sampleH(x - 1, z - 1);
            float h10 = sampleH(x,     z - 1);
            float h20 = sampleH(x + 1, z - 1);
            float h01 = sampleH(x - 1, z    );
            float h21 = sampleH(x + 1, z    );
            float h02 = sampleH(x - 1, z + 1);
            float h12 = sampleH(x,     z + 1);
            float h22 = sampleH(x + 1, z + 1);

            float gx = -h00 + h20 - 2.0f * h01 + 2.0f * h21 - h02 + h22;
            float gz =  h00 + 2.0f * h10 + h20 - h02 - 2.0f * h12 - h22;
            glm::vec3 normal = glm::normalize(glm::vec3(-gx, 8.0f * step, -gz));

            // Position
            vertices.push_back(px);
            vertices.push_back(py);
            vertices.push_back(pz);
            // Normal
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            // UV
            vertices.push_back(static_cast<float>(x) / quads);
            vertices.push_back(static_cast<float>(z) / quads);
        }
    }

    // Generate indices (skip holes)
    for (int z = 0; z < quads; z++) {
        for (int x = 0; x < quads; x++) {
            int holeX = std::min(x * TERRAIN_HOLE_GRID_SIZE / quads, TERRAIN_HOLE_GRID_SIZE - 1);
            int holeZ = std::min(z * TERRAIN_HOLE_GRID_SIZE / quads, TERRAIN_HOLE_GRID_SIZE - 1);
            uint64_t holeBit = 1ULL << (holeZ * TERRAIN_HOLE_GRID_SIZE + holeX);
            if (data.holeMask & holeBit) continue;

            uint32_t TL = z * res + x;
            uint32_t TR = TL + 1;
            uint32_t BL = (z + 1) * res + x;
            uint32_t BR = BL + 1;

            indices.push_back(TL);
            indices.push_back(BL);
            indices.push_back(TR);
            indices.push_back(TR);
            indices.push_back(BL);
            indices.push_back(BR);
        }
    }

    chunk.indexCount = static_cast<uint32_t>(indices.size());

    Onyx::RenderCommand::ResetState();

    chunk.vao = std::make_unique<Onyx::VertexArray>();
    chunk.vbo = std::make_unique<Onyx::VertexBuffer>(vertices.data(), vertices.size() * sizeof(float));
    chunk.ebo = std::make_unique<Onyx::IndexBuffer>(indices.data(), indices.size() * sizeof(uint32_t));

    Onyx::VertexLayout layout({
        { Onyx::VertexAttributeType::Float3 },  // position
        { Onyx::VertexAttributeType::Float3 },  // normal
        { Onyx::VertexAttributeType::Float2 }   // texcoord
    });

    chunk.vao->SetVertexBuffer(chunk.vbo.get());
    chunk.vao->SetLayout(layout);
    chunk.vao->SetIndexBuffer(chunk.ebo.get());
    chunk.vao->UnBind();

    // Model matrix is identity (vertices are already in world space)
    chunk.modelMatrix = glm::mat4(1.0f);
}

void ClientTerrainSystem::UploadSplatmapTextures(ClientTerrainChunk& chunk) {
    const auto& data = chunk.data;
    if (data.splatmap.empty()) return;

    const int texels = TERRAIN_SPLATMAP_TEXELS;
    std::vector<uint8_t> rgba0(texels * 4);
    std::vector<uint8_t> rgba1(texels * 4);

    for (int i = 0; i < texels; i++) {
        int src = i * TERRAIN_MAX_LAYERS;
        rgba0[i * 4 + 0] = data.splatmap[src + 0];
        rgba0[i * 4 + 1] = data.splatmap[src + 1];
        rgba0[i * 4 + 2] = data.splatmap[src + 2];
        rgba0[i * 4 + 3] = data.splatmap[src + 3];

        rgba1[i * 4 + 0] = data.splatmap[src + 4];
        rgba1[i * 4 + 1] = data.splatmap[src + 5];
        rgba1[i * 4 + 2] = data.splatmap[src + 6];
        rgba1[i * 4 + 3] = data.splatmap[src + 7];
    }

    chunk.splatmapTexture0 = Onyx::Texture::CreateFromData(
        rgba0.data(), TERRAIN_SPLATMAP_RESOLUTION, TERRAIN_SPLATMAP_RESOLUTION, 4);
    chunk.splatmapTexture1 = Onyx::Texture::CreateFromData(
        rgba1.data(), TERRAIN_SPLATMAP_RESOLUTION, TERRAIN_SPLATMAP_RESOLUTION, 4);
}

void ClientTerrainSystem::Render(Onyx::Shader* shader) {
    for (auto& [key, chunk] : m_Chunks) {
        if (!chunk->vao || chunk->indexCount == 0) continue;

        shader->SetMat4("u_Model", chunk->modelMatrix);

        if (chunk->splatmapTexture0) {
            chunk->splatmapTexture0->Bind(0);
            shader->SetInt("u_Splatmap0", 0);
        }
        if (chunk->splatmapTexture1) {
            chunk->splatmapTexture1->Bind(1);
            shader->SetInt("u_Splatmap1", 1);
        }

        chunk->vao->Bind();
        Onyx::RenderCommand::DrawIndexed(*chunk->vao, chunk->indexCount);
        chunk->vao->UnBind();
    }
}

float ClientTerrainSystem::GetHeightAt(float worldX, float worldZ) const {
    int cx = WorldToChunkCoord(worldX);
    int cz = WorldToChunkCoord(worldZ);

    int64_t key = PackKey(cx, cz);
    auto it = m_Chunks.find(key);
    if (it == m_Chunks.end()) return 0.0f;

    float localX = worldX - cx * TERRAIN_CHUNK_SIZE;
    float localZ = worldZ - cz * TERRAIN_CHUNK_SIZE;

    return GetTerrainHeight(it->second->data, localX, localZ);
}

} // namespace MMO
