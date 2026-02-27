#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace MMO {

// Terrain chunk constants (shared between Editor3D and Client)
constexpr float TERRAIN_CHUNK_SIZE = 64.0f;
constexpr int TERRAIN_CHUNK_RESOLUTION = 65;
constexpr int TERRAIN_CHUNK_HEIGHTMAP_SIZE = TERRAIN_CHUNK_RESOLUTION * TERRAIN_CHUNK_RESOLUTION;
constexpr int TERRAIN_MAX_LAYERS = 8;
constexpr int TERRAIN_SPLATMAP_RESOLUTION = 64;
constexpr int TERRAIN_SPLATMAP_TEXELS = TERRAIN_SPLATMAP_RESOLUTION * TERRAIN_SPLATMAP_RESOLUTION;
constexpr int TERRAIN_HOLE_GRID_SIZE = 8;

struct TerrainChunkData {
    int32_t chunkX = 0;
    int32_t chunkZ = 0;

    std::vector<float> heightmap;
    std::vector<uint8_t> splatmap;  // 8 channels per texel
    uint64_t holeMask = 0;

    std::string materialIds[TERRAIN_MAX_LAYERS];

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

// Bilinear height interpolation (localX/localZ in [0, TERRAIN_CHUNK_SIZE])
inline float GetTerrainHeight(const TerrainChunkData& data, float localX, float localZ) {
    if (data.heightmap.empty()) return 0.0f;

    float u = std::clamp(localX / TERRAIN_CHUNK_SIZE, 0.0f, 1.0f);
    float v = std::clamp(localZ / TERRAIN_CHUNK_SIZE, 0.0f, 1.0f);

    float fx = u * (TERRAIN_CHUNK_RESOLUTION - 1);
    float fz = v * (TERRAIN_CHUNK_RESOLUTION - 1);

    int x0 = static_cast<int>(fx);
    int z0 = static_cast<int>(fz);
    int x1 = std::min(x0 + 1, TERRAIN_CHUNK_RESOLUTION - 1);
    int z1 = std::min(z0 + 1, TERRAIN_CHUNK_RESOLUTION - 1);

    float fracX = fx - x0;
    float fracZ = fz - z0;

    float h00 = data.heightmap[z0 * TERRAIN_CHUNK_RESOLUTION + x0];
    float h10 = data.heightmap[z0 * TERRAIN_CHUNK_RESOLUTION + x1];
    float h01 = data.heightmap[z1 * TERRAIN_CHUNK_RESOLUTION + x0];
    float h11 = data.heightmap[z1 * TERRAIN_CHUNK_RESOLUTION + x1];

    float h0 = h00 + fracX * (h10 - h00);
    float h1 = h01 + fracX * (h11 - h01);

    return h0 + fracZ * (h1 - h0);
}

inline int WorldToChunkCoord(float worldCoord) {
    return static_cast<int>(std::floor(worldCoord / TERRAIN_CHUNK_SIZE));
}

// Split 8-channel interleaved splatmap into two RGBA textures
inline void SplitSplatmapToRGBA(const std::vector<uint8_t>& splatmap,
                                 std::vector<uint8_t>& rgba0,
                                 std::vector<uint8_t>& rgba1) {
    rgba0.resize(TERRAIN_SPLATMAP_TEXELS * 4);
    rgba1.resize(TERRAIN_SPLATMAP_TEXELS * 4);

    for (int i = 0; i < TERRAIN_SPLATMAP_TEXELS; i++) {
        int src = i * TERRAIN_MAX_LAYERS;
        rgba0[i * 4 + 0] = splatmap[src + 0];
        rgba0[i * 4 + 1] = splatmap[src + 1];
        rgba0[i * 4 + 2] = splatmap[src + 2];
        rgba0[i * 4 + 3] = splatmap[src + 3];

        rgba1[i * 4 + 0] = splatmap[src + 4];
        rgba1[i * 4 + 1] = splatmap[src + 5];
        rgba1[i * 4 + 2] = splatmap[src + 6];
        rgba1[i * 4 + 3] = splatmap[src + 7];
    }
}

} // namespace MMO
