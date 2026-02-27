#pragma once

#include "TerrainData.h"
#include <string>
#include <vector>
#include <fstream>

namespace MMO {

// Light data read from .chunk files (plain floats to avoid engine dependency)
struct ChunkLightData {
    uint8_t type = 0;  // 0=Point, 1=Spot
    float position[3] = {0, 0, 0};
    float direction[3] = {0, -1, 0};
    float color[3] = {1, 1, 1};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerAngle = 30.0f;
    float outerAngle = 45.0f;
    bool castShadows = false;
};

// Static object data read from .chunk files
struct ChunkObjectData {
    std::string modelPath;
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};
    float scale[3] = {1, 1, 1};
    uint32_t flags = 0;
};

// Complete data from a single .chunk file
struct ChunkFileData {
    uint32_t mapId = 0;
    TerrainChunkData terrain;
    std::vector<ChunkLightData> lights;
    std::vector<ChunkObjectData> objects;
};

// Read a .chunk file (CHNK container format)
// Returns true on success
bool LoadChunkFile(const std::string& path, ChunkFileData& out);

// Read just the terrain section from an already-open .chunk stream
void ReadTerrainSection(std::ifstream& file, TerrainChunkData& data,
                        int32_t chunkX, int32_t chunkZ);

} // namespace MMO
