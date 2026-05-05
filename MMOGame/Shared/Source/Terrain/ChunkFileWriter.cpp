#include "ChunkFileWriter.h"
#include "ChunkIO.h"
#include <fstream>
#include <iostream>

namespace MMO {

static void WriteTerrainSection(std::ofstream& file, const TerrainChunkData& data) {
    // Heightmap
    file.write(reinterpret_cast<const char*>(data.heightmap.data()),
               TERRAIN_CHUNK_HEIGHTMAP_SIZE * sizeof(float));

    // Splatmap (8 channels per texel)
    file.write(reinterpret_cast<const char*>(data.splatmap.data()),
               TERRAIN_SPLATMAP_TEXELS * TERRAIN_MAX_LAYERS);

    // Hole mask
    file.write(reinterpret_cast<const char*>(&data.holeMask), sizeof(data.holeMask));

    // Bounds
    file.write(reinterpret_cast<const char*>(&data.minHeight), sizeof(data.minHeight));
    file.write(reinterpret_cast<const char*>(&data.maxHeight), sizeof(data.maxHeight));

    // Material IDs
    for (int i = 0; i < TERRAIN_MAX_LAYERS; i++) {
        WriteString(file, data.materialIds[i]);
    }
}

static void WriteLightsSection(std::ofstream& file, const std::vector<ChunkLightData>& lights) {
    uint32_t count = static_cast<uint32_t>(lights.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& light : lights) {
        file.write(reinterpret_cast<const char*>(&light.type), sizeof(light.type));
        file.write(reinterpret_cast<const char*>(&light.position), sizeof(light.position));
        file.write(reinterpret_cast<const char*>(&light.direction), sizeof(light.direction));
        file.write(reinterpret_cast<const char*>(&light.color), sizeof(light.color));
        file.write(reinterpret_cast<const char*>(&light.intensity), sizeof(light.intensity));
        file.write(reinterpret_cast<const char*>(&light.range), sizeof(light.range));
        file.write(reinterpret_cast<const char*>(&light.innerAngle), sizeof(light.innerAngle));
        file.write(reinterpret_cast<const char*>(&light.outerAngle), sizeof(light.outerAngle));
        file.write(reinterpret_cast<const char*>(&light.castShadows), sizeof(light.castShadows));
    }
}

static void WriteObjectsSection(std::ofstream& file, const std::vector<ChunkObjectData>& objects) {
    uint32_t count = static_cast<uint32_t>(objects.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& obj : objects) {
        WriteString(file, obj.modelPath);
        file.write(reinterpret_cast<const char*>(&obj.position), sizeof(obj.position));
        file.write(reinterpret_cast<const char*>(&obj.rotation), sizeof(obj.rotation));
        file.write(reinterpret_cast<const char*>(&obj.scale), sizeof(obj.scale));
        file.write(reinterpret_cast<const char*>(&obj.flags), sizeof(obj.flags));
        WriteString(file, obj.materialId);
    }
}

bool WriteChunkFile(const std::string& path, const ChunkFileData& data,
                    int32_t chunkX, int32_t chunkZ) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ChunkFileWriter] Failed to open: " << path << "\n";
        return false;
    }

    // Container header
    uint32_t magic = CHNK_MAGIC;
    uint32_t version = CHUNK_FORMAT_VERSION;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&data.mapId), sizeof(data.mapId));
    file.write(reinterpret_cast<const char*>(&chunkX), sizeof(chunkX));
    file.write(reinterpret_cast<const char*>(&chunkZ), sizeof(chunkZ));

    // Count sections
    uint32_t sectionCount = 0;
    bool hasTerrain = !data.terrain.heightmap.empty();
    bool hasLights  = !data.lights.empty();
    bool hasObjects = !data.objects.empty();
    if (hasTerrain) sectionCount++;
    if (hasLights)  sectionCount++;
    if (hasObjects) sectionCount++;
    file.write(reinterpret_cast<const char*>(&sectionCount), sizeof(sectionCount));

    // Terrain section
    if (hasTerrain) {
        SectionWriter sw(file, TERR_TAG);
        WriteTerrainSection(file, data.terrain);
    }

    // Lights section
    if (hasLights) {
        SectionWriter sw(file, LGHT_TAG);
        WriteLightsSection(file, data.lights);
    }

    // Objects section
    if (hasObjects) {
        SectionWriter sw(file, OBJS_TAG);
        WriteObjectsSection(file, data.objects);
    }

    return file.good();
}

} // namespace MMO
