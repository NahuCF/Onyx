#include "ChunkFileReader.h"
#include <fstream>
#include <iostream>

namespace MMO {

static constexpr uint32_t CHNK_MAGIC = 0x43484E4B;
static constexpr uint32_t TERR_TAG   = 0x54455252;
static constexpr uint32_t LGHT_TAG   = 0x4C474854;
static constexpr uint32_t OBJS_TAG   = 0x4F424A53;
static constexpr uint32_t SNDS_TAG   = 0x534E4453;

static void ReadTerrainSection(std::ifstream& file, TerrainChunkData& data, int32_t chunkX, int32_t chunkZ) {
    data.chunkX = chunkX;
    data.chunkZ = chunkZ;

    data.heightmap.resize(TERRAIN_CHUNK_HEIGHTMAP_SIZE);
    file.read(reinterpret_cast<char*>(data.heightmap.data()),
              TERRAIN_CHUNK_HEIGHTMAP_SIZE * sizeof(float));

    data.splatmap.resize(TERRAIN_SPLATMAP_TEXELS * TERRAIN_MAX_LAYERS);
    file.read(reinterpret_cast<char*>(data.splatmap.data()),
              TERRAIN_SPLATMAP_TEXELS * TERRAIN_MAX_LAYERS);

    file.read(reinterpret_cast<char*>(&data.holeMask), sizeof(data.holeMask));
    file.read(reinterpret_cast<char*>(&data.minHeight), sizeof(data.minHeight));
    file.read(reinterpret_cast<char*>(&data.maxHeight), sizeof(data.maxHeight));

    for (int i = 0; i < TERRAIN_MAX_LAYERS; i++) {
        uint16_t len = 0;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (len > 0 && len < 256) {
            data.materialIds[i].resize(len);
            file.read(data.materialIds[i].data(), len);
        } else {
            data.materialIds[i].clear();
        }
    }
}

static void ReadLightsSection(std::ifstream& file, std::vector<ChunkLightData>& lights) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (count > 4096) return;

    lights.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        auto& light = lights[i];
        file.read(reinterpret_cast<char*>(&light.type), sizeof(light.type));
        file.read(reinterpret_cast<char*>(&light.position), sizeof(light.position));
        file.read(reinterpret_cast<char*>(&light.direction), sizeof(light.direction));
        file.read(reinterpret_cast<char*>(&light.color), sizeof(light.color));
        file.read(reinterpret_cast<char*>(&light.intensity), sizeof(light.intensity));
        file.read(reinterpret_cast<char*>(&light.range), sizeof(light.range));
        file.read(reinterpret_cast<char*>(&light.innerAngle), sizeof(light.innerAngle));
        file.read(reinterpret_cast<char*>(&light.outerAngle), sizeof(light.outerAngle));
        file.read(reinterpret_cast<char*>(&light.castShadows), sizeof(light.castShadows));
    }
}

static void ReadObjectsSection(std::ifstream& file, std::vector<ChunkObjectData>& objects) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (count > 65536) return;

    objects.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        auto& obj = objects[i];

        uint16_t pathLen = 0;
        file.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
        if (pathLen > 0 && pathLen < 1024) {
            obj.modelPath.resize(pathLen);
            file.read(obj.modelPath.data(), pathLen);
        }

        file.read(reinterpret_cast<char*>(&obj.position), sizeof(obj.position));
        file.read(reinterpret_cast<char*>(&obj.rotation), sizeof(obj.rotation));
        file.read(reinterpret_cast<char*>(&obj.scale), sizeof(obj.scale));
        file.read(reinterpret_cast<char*>(&obj.flags), sizeof(obj.flags));
    }
}

bool LoadChunkFile(const std::string& path, ChunkFileData& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != CHNK_MAGIC) {
        return false;
    }

    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    file.read(reinterpret_cast<char*>(&out.mapId), sizeof(out.mapId));

    int32_t chunkX, chunkZ;
    file.read(reinterpret_cast<char*>(&chunkX), sizeof(chunkX));
    file.read(reinterpret_cast<char*>(&chunkZ), sizeof(chunkZ));

    uint32_t sectionCount;
    file.read(reinterpret_cast<char*>(&sectionCount), sizeof(sectionCount));

    for (uint32_t s = 0; s < sectionCount && file.good(); s++) {
        uint32_t sectionType;
        file.read(reinterpret_cast<char*>(&sectionType), sizeof(sectionType));

        uint32_t sectionSize;
        file.read(reinterpret_cast<char*>(&sectionSize), sizeof(sectionSize));

        auto sectionStart = file.tellg();

        switch (sectionType) {
            case TERR_TAG:  ReadTerrainSection(file, out.terrain, chunkX, chunkZ); break;
            case LGHT_TAG:  ReadLightsSection(file, out.lights); break;
            case OBJS_TAG:  ReadObjectsSection(file, out.objects); break;
            case SNDS_TAG:  break; // Skip sounds for now
            default: break;
        }

        file.seekg(sectionStart + static_cast<std::streamoff>(sectionSize));
    }

    return true;
}

} // namespace MMO
