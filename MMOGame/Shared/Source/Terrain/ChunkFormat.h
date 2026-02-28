#pragma once

#include <cstdint>
#include <string>

namespace MMO {

// ---- Container magic ----
constexpr uint32_t CHNK_MAGIC = 0x43484E4B;  // "CHNK"

// ---- Section tags ----
constexpr uint32_t TERR_TAG = 0x54455252;  // Terrain
constexpr uint32_t LGHT_TAG = 0x4C474854;  // Lights
constexpr uint32_t OBJS_TAG = 0x4F424A53;  // Objects
constexpr uint32_t SNDS_TAG = 0x534E4453;  // Sounds

// ---- Format versions ----
constexpr uint32_t CHUNK_FORMAT_VERSION   = 2;  // CHNK container
constexpr uint32_t TERRAIN_FORMAT_VERSION = 4;  // Standalone TERR

// ---- Sanity limits ----
constexpr uint16_t MAX_STRING_LENGTH      = 4096;
constexpr uint32_t MAX_LIGHTS_PER_CHUNK   = 4096;
constexpr uint32_t MAX_OBJECTS_PER_CHUNK  = 65536;
constexpr uint32_t MAX_SOUNDS_PER_CHUNK   = 4096;

// ---- Section data structs (plain data — no engine dependency) ----

struct ChunkLightData {
    uint8_t type = 0;  // 0=Point, 1=Spot
    float position[3]  = {0, 0, 0};
    float direction[3] = {0, -1, 0};
    float color[3]     = {1, 1, 1};
    float intensity    = 1.0f;
    float range        = 10.0f;
    float innerAngle   = 30.0f;
    float outerAngle   = 45.0f;
    bool  castShadows  = false;
};

struct ChunkObjectData {
    std::string modelPath;
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};
    float scale[3]    = {1, 1, 1};
    uint32_t flags    = 0;
};

} // namespace MMO
