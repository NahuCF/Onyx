#include "WorldChunk.h"
#include <Terrain/ChunkFileReader.h>
#include <filesystem>

namespace Editor3D {

static constexpr uint32_t CHUNK_FORMAT_VERSION = 2;

static void WriteString(std::ofstream& f, const std::string& s) {
    uint16_t len = static_cast<uint16_t>(s.size());
    f.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) f.write(s.data(), len);
}

static std::string ReadString(std::ifstream& f) {
    uint16_t len = 0;
    f.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len == 0 || len > 4096) return {};
    std::string s(len, '\0');
    f.read(s.data(), len);
    return s;
}

WorldChunk::WorldChunk(int32_t chunkX, int32_t chunkZ)
    : m_ChunkX(chunkX), m_ChunkZ(chunkZ)
{
    m_Terrain = std::make_unique<TerrainChunk>(chunkX, chunkZ);
}

WorldChunk::~WorldChunk() = default;

void WorldChunk::Load(const std::string& filePath) {
    m_Lights.clear();
    m_Objects.clear();
    m_Sounds.clear();

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        // No file — initialize terrain with defaults
        TerrainChunkData data;
        data.chunkX = m_ChunkX;
        data.chunkZ = m_ChunkZ;
        data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
        data.splatmap.resize(SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS, 0);
        for (int i = 0; i < SPLATMAP_TEXELS; i++) {
            data.splatmap[i * MAX_TERRAIN_LAYERS] = 255;
        }
        data.holeMask = 0;
        data.minHeight = 0.0f;
        data.maxHeight = 0.0f;
        m_Terrain->LoadFromData(data);
        return;
    }

    // Read and validate CHNK header
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != CHNK_MAGIC) {
        file.close();
        return;
    }

    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    m_LoadedVersion = version;

    uint32_t mapId;
    file.read(reinterpret_cast<char*>(&mapId), sizeof(mapId));

    int32_t chunkX_file, chunkZ_file;
    file.read(reinterpret_cast<char*>(&chunkX_file), sizeof(chunkX_file));
    file.read(reinterpret_cast<char*>(&chunkZ_file), sizeof(chunkZ_file));

    uint32_t sectionCount;
    file.read(reinterpret_cast<char*>(&sectionCount), sizeof(sectionCount));

    // Read sections
    for (uint32_t s = 0; s < sectionCount && file.good(); s++) {
        uint32_t sectionType;
        file.read(reinterpret_cast<char*>(&sectionType), sizeof(sectionType));

        uint32_t sectionSize;
        file.read(reinterpret_cast<char*>(&sectionSize), sizeof(sectionSize));

        auto sectionStart = file.tellg();

        switch (sectionType) {
            case TERR_TAG:  LoadTerrainSection(file); break;
            case LGHT_TAG:  LoadLightsSection(file);  break;
            case OBJS_TAG:  LoadObjectsSection(file); break;
            case SNDS_TAG:  LoadSoundsSection(file);  break;
            default: break; // Unknown section — skip
        }

        // Advance to next section regardless of how much the reader consumed
        file.seekg(sectionStart + static_cast<std::streamoff>(sectionSize));
    }

    file.close();
}

void WorldChunk::Save(const std::string& filePath) {
    const auto& terrainData = m_Terrain->GetData();
    if (terrainData.heightmap.size() < CHUNK_HEIGHTMAP_SIZE) return;
    if (terrainData.splatmap.size() < static_cast<size_t>(SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS)) return;

    std::filesystem::path dir = std::filesystem::path(filePath).parent_path();
    std::filesystem::create_directories(dir);

    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return;

    // Count populated sections
    uint32_t sectionCount = 1; // TERR always
    if (!m_Lights.empty()) sectionCount++;
    if (!m_Objects.empty()) sectionCount++;
    if (!m_Sounds.empty()) sectionCount++;

    // CHNK header
    uint32_t magic = CHNK_MAGIC;
    uint32_t version = CHUNK_FORMAT_VERSION;
    uint32_t mapId = 0; // TODO: pass from EditorWorldSystem

    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    file.write(reinterpret_cast<const char*>(&mapId), sizeof(mapId));
    file.write(reinterpret_cast<const char*>(&m_ChunkX), sizeof(m_ChunkX));
    file.write(reinterpret_cast<const char*>(&m_ChunkZ), sizeof(m_ChunkZ));
    file.write(reinterpret_cast<const char*>(&sectionCount), sizeof(sectionCount));

    // Sections
    SaveTerrainSection(file);
    if (!m_Lights.empty()) SaveLightsSection(file);
    if (!m_Objects.empty()) SaveObjectsSection(file);
    if (!m_Sounds.empty()) SaveSoundsSection(file);

    file.close();
}

void WorldChunk::Unload() {
    if (m_Terrain) m_Terrain->Unload();
    m_Lights.clear();
    m_Objects.clear();
    m_Sounds.clear();
    m_Modified = false;
}

// ---- Section Readers ----

void WorldChunk::LoadTerrainSection(std::ifstream& file) {
    TerrainChunkData data;
    MMO::ReadTerrainSection(file, data, m_ChunkX, m_ChunkZ);
    m_Terrain->LoadFromData(data);
}

void WorldChunk::LoadLightsSection(std::ifstream& file) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (count > 4096) return; // sanity limit

    m_Lights.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        auto& light = m_Lights[i];
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

void WorldChunk::LoadObjectsSection(std::ifstream& file) {
    // v1 OBJS format is incompatible — skip
    if (m_LoadedVersion < 2) return;

    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (count > 65536) return;

    m_Objects.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        auto& obj = m_Objects[i];

        // WorldObject base
        file.read(reinterpret_cast<char*>(&obj.guid), sizeof(obj.guid));
        obj.name = ReadString(file);
        file.read(reinterpret_cast<char*>(&obj.position), sizeof(obj.position));
        file.read(reinterpret_cast<char*>(&obj.rotation), sizeof(obj.rotation));
        file.read(reinterpret_cast<char*>(&obj.scale), sizeof(obj.scale));
        file.read(reinterpret_cast<char*>(&obj.parentGuid), sizeof(obj.parentGuid));

        // StaticObject
        obj.modelPath = ReadString(file);
        obj.materialId = ReadString(file);

        // Collider
        file.read(reinterpret_cast<char*>(&obj.colliderType), sizeof(obj.colliderType));
        file.read(reinterpret_cast<char*>(&obj.colliderCenter), sizeof(obj.colliderCenter));
        file.read(reinterpret_cast<char*>(&obj.colliderHalfExtents), sizeof(obj.colliderHalfExtents));
        file.read(reinterpret_cast<char*>(&obj.colliderRadius), sizeof(obj.colliderRadius));
        file.read(reinterpret_cast<char*>(&obj.colliderHeight), sizeof(obj.colliderHeight));

        // Rendering flags
        uint8_t flags = 0;
        file.read(reinterpret_cast<char*>(&flags), sizeof(flags));
        obj.castsShadow = (flags & 0x01) != 0;
        obj.receivesLightmap = (flags & 0x02) != 0;
        file.read(reinterpret_cast<char*>(&obj.lightmapIndex), sizeof(obj.lightmapIndex));
        file.read(reinterpret_cast<char*>(&obj.lightmapScaleOffset), sizeof(obj.lightmapScaleOffset));

        // Mesh materials
        uint16_t meshMatCount = 0;
        file.read(reinterpret_cast<char*>(&meshMatCount), sizeof(meshMatCount));
        obj.meshMaterials.resize(meshMatCount);
        for (uint16_t m = 0; m < meshMatCount; m++) {
            auto& mm = obj.meshMaterials[m];
            mm.meshName = ReadString(file);
            mm.materialId = ReadString(file);
            file.read(reinterpret_cast<char*>(&mm.positionOffset), sizeof(mm.positionOffset));
            file.read(reinterpret_cast<char*>(&mm.rotationOffset), sizeof(mm.rotationOffset));
            file.read(reinterpret_cast<char*>(&mm.scaleMultiplier), sizeof(mm.scaleMultiplier));
            file.read(reinterpret_cast<char*>(&mm.visible), sizeof(mm.visible));
        }

        // Animations
        uint16_t animCount = 0;
        file.read(reinterpret_cast<char*>(&animCount), sizeof(animCount));
        obj.animationPaths.resize(animCount);
        for (uint16_t a = 0; a < animCount; a++) {
            obj.animationPaths[a] = ReadString(file);
        }
        obj.currentAnimation = ReadString(file);
        file.read(reinterpret_cast<char*>(&obj.animLoop), sizeof(obj.animLoop));
        file.read(reinterpret_cast<char*>(&obj.animSpeed), sizeof(obj.animSpeed));
    }
}

void WorldChunk::LoadSoundsSection(std::ifstream& file) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (count > 4096) return;

    m_Sounds.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        auto& snd = m_Sounds[i];

        uint16_t pathLen = 0;
        file.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
        if (pathLen > 0 && pathLen < 1024) {
            snd.soundPath.resize(pathLen);
            file.read(snd.soundPath.data(), pathLen);
        }

        file.read(reinterpret_cast<char*>(&snd.position), sizeof(snd.position));
        file.read(reinterpret_cast<char*>(&snd.volume), sizeof(snd.volume));
        file.read(reinterpret_cast<char*>(&snd.minRange), sizeof(snd.minRange));
        file.read(reinterpret_cast<char*>(&snd.maxRange), sizeof(snd.maxRange));
        file.read(reinterpret_cast<char*>(&snd.loop), sizeof(snd.loop));
    }
}

// ---- Section Writers ----

void WorldChunk::SaveTerrainSection(std::ofstream& file) {
    const auto& data = m_Terrain->GetData();

    uint32_t tag = TERR_TAG;
    uint32_t size = 0; // placeholder
    file.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
    auto sizePos = file.tellp();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    auto dataStart = file.tellp();

    file.write(reinterpret_cast<const char*>(data.heightmap.data()),
               CHUNK_HEIGHTMAP_SIZE * sizeof(float));
    file.write(reinterpret_cast<const char*>(data.splatmap.data()),
               SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS);
    file.write(reinterpret_cast<const char*>(&data.holeMask), sizeof(data.holeMask));
    file.write(reinterpret_cast<const char*>(&data.minHeight), sizeof(data.minHeight));
    file.write(reinterpret_cast<const char*>(&data.maxHeight), sizeof(data.maxHeight));

    for (int i = 0; i < MAX_TERRAIN_LAYERS; i++) {
        uint16_t len = static_cast<uint16_t>(data.materialIds[i].size());
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (len > 0) {
            file.write(data.materialIds[i].data(), len);
        }
    }

    auto dataEnd = file.tellp();
    size = static_cast<uint32_t>(dataEnd - dataStart);
    file.seekp(sizePos);
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    file.seekp(dataEnd);
}

void WorldChunk::SaveLightsSection(std::ofstream& file) {
    uint32_t tag = LGHT_TAG;
    uint32_t size = 0;
    file.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
    auto sizePos = file.tellp();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    auto dataStart = file.tellp();

    uint32_t count = static_cast<uint32_t>(m_Lights.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& light : m_Lights) {
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

    auto dataEnd = file.tellp();
    size = static_cast<uint32_t>(dataEnd - dataStart);
    file.seekp(sizePos);
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    file.seekp(dataEnd);
}

void WorldChunk::SaveObjectsSection(std::ofstream& file) {
    uint32_t tag = OBJS_TAG;
    uint32_t size = 0;
    file.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
    auto sizePos = file.tellp();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    auto dataStart = file.tellp();

    uint32_t count = static_cast<uint32_t>(m_Objects.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& obj : m_Objects) {
        // WorldObject base
        file.write(reinterpret_cast<const char*>(&obj.guid), sizeof(obj.guid));
        WriteString(file, obj.name);
        file.write(reinterpret_cast<const char*>(&obj.position), sizeof(obj.position));
        file.write(reinterpret_cast<const char*>(&obj.rotation), sizeof(obj.rotation));
        file.write(reinterpret_cast<const char*>(&obj.scale), sizeof(obj.scale));
        file.write(reinterpret_cast<const char*>(&obj.parentGuid), sizeof(obj.parentGuid));

        // StaticObject
        WriteString(file, obj.modelPath);
        WriteString(file, obj.materialId);

        // Collider
        file.write(reinterpret_cast<const char*>(&obj.colliderType), sizeof(obj.colliderType));
        file.write(reinterpret_cast<const char*>(&obj.colliderCenter), sizeof(obj.colliderCenter));
        file.write(reinterpret_cast<const char*>(&obj.colliderHalfExtents), sizeof(obj.colliderHalfExtents));
        file.write(reinterpret_cast<const char*>(&obj.colliderRadius), sizeof(obj.colliderRadius));
        file.write(reinterpret_cast<const char*>(&obj.colliderHeight), sizeof(obj.colliderHeight));

        // Rendering flags
        uint8_t flags = 0;
        if (obj.castsShadow)      flags |= 0x01;
        if (obj.receivesLightmap) flags |= 0x02;
        file.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
        file.write(reinterpret_cast<const char*>(&obj.lightmapIndex), sizeof(obj.lightmapIndex));
        file.write(reinterpret_cast<const char*>(&obj.lightmapScaleOffset), sizeof(obj.lightmapScaleOffset));

        // Mesh materials
        uint16_t meshMatCount = static_cast<uint16_t>(obj.meshMaterials.size());
        file.write(reinterpret_cast<const char*>(&meshMatCount), sizeof(meshMatCount));
        for (const auto& mm : obj.meshMaterials) {
            WriteString(file, mm.meshName);
            WriteString(file, mm.materialId);
            file.write(reinterpret_cast<const char*>(&mm.positionOffset), sizeof(mm.positionOffset));
            file.write(reinterpret_cast<const char*>(&mm.rotationOffset), sizeof(mm.rotationOffset));
            file.write(reinterpret_cast<const char*>(&mm.scaleMultiplier), sizeof(mm.scaleMultiplier));
            file.write(reinterpret_cast<const char*>(&mm.visible), sizeof(mm.visible));
        }

        // Animations
        uint16_t animCount = static_cast<uint16_t>(obj.animationPaths.size());
        file.write(reinterpret_cast<const char*>(&animCount), sizeof(animCount));
        for (const auto& path : obj.animationPaths) {
            WriteString(file, path);
        }
        WriteString(file, obj.currentAnimation);
        file.write(reinterpret_cast<const char*>(&obj.animLoop), sizeof(obj.animLoop));
        file.write(reinterpret_cast<const char*>(&obj.animSpeed), sizeof(obj.animSpeed));
    }

    auto dataEnd = file.tellp();
    size = static_cast<uint32_t>(dataEnd - dataStart);
    file.seekp(sizePos);
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    file.seekp(dataEnd);
}

void WorldChunk::SaveSoundsSection(std::ofstream& file) {
    uint32_t tag = SNDS_TAG;
    uint32_t size = 0;
    file.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
    auto sizePos = file.tellp();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    auto dataStart = file.tellp();

    uint32_t count = static_cast<uint32_t>(m_Sounds.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& snd : m_Sounds) {
        uint16_t pathLen = static_cast<uint16_t>(snd.soundPath.size());
        file.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
        if (pathLen > 0) file.write(snd.soundPath.data(), pathLen);

        file.write(reinterpret_cast<const char*>(&snd.position), sizeof(snd.position));
        file.write(reinterpret_cast<const char*>(&snd.volume), sizeof(snd.volume));
        file.write(reinterpret_cast<const char*>(&snd.minRange), sizeof(snd.minRange));
        file.write(reinterpret_cast<const char*>(&snd.maxRange), sizeof(snd.maxRange));
        file.write(reinterpret_cast<const char*>(&snd.loop), sizeof(snd.loop));
    }

    auto dataEnd = file.tellp();
    size = static_cast<uint32_t>(dataEnd - dataStart);
    file.seekp(sizePos);
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    file.seekp(dataEnd);
}

} // namespace Editor3D
