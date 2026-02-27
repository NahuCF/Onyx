#pragma once

#include "TerrainData.h"
#include <functional>
#include <vector>
#include <cstdint>

namespace MMO {

// Callback: (chunkX, chunkZ, localX, localZ) -> height
// Used for cross-chunk boundary sampling. If null, clamps to edge.
using HeightSamplerFn = std::function<float(int cx, int cz, int lx, int lz)>;

struct TerrainMeshOptions {
    int meshResolution = TERRAIN_CHUNK_RESOLUTION;  // 65 default
    bool sobelNormals = true;
    bool smoothNormals = false;
    bool diamondGrid = false;
    bool generatePaddedHeightmap = false;
    HeightSamplerFn heightSampler;  // null = clamp to edge
};

struct TerrainMeshData {
    std::vector<float> vertices;       // pos(3) + normal(3) + uv(2) per vertex
    std::vector<uint32_t> indices;
    uint32_t indexCount = 0;
    std::vector<float> paddedHeightmap;       // (TERRAIN_CHUNK_RESOLUTION+2)^2 floats
    int paddedHeightmapResolution = 0;
    std::vector<uint8_t> splatmapRGBA0;       // TERRAIN_SPLATMAP_TEXELS * 4
    std::vector<uint8_t> splatmapRGBA1;       // TERRAIN_SPLATMAP_TEXELS * 4
};

// Pure CPU mesh generation — no GL calls, thread-safe.
// Generates vertices, indices, optionally padded heightmap and split splatmaps.
void GenerateTerrainMesh(const TerrainChunkData& data,
                         const TerrainMeshOptions& options,
                         TerrainMeshData& out);

} // namespace MMO
