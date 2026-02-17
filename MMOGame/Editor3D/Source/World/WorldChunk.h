#pragma once

#include "Terrain/TerrainChunk.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <fstream>

namespace Editor3D {

// ---- Light data ----
enum class LightType : uint8_t { Point = 0, Spot = 1 };

struct EditorLight {
    LightType type = LightType::Point;
    glm::vec3 position = {0, 0, 0};
    glm::vec3 direction = {0, -1, 0};
    glm::vec3 color = {1, 1, 1};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerAngle = 30.0f;
    float outerAngle = 45.0f;
    bool castShadows = false;
};

constexpr int MAX_POINT_LIGHTS = 32;
constexpr int MAX_SPOT_LIGHTS = 8;

// ---- Static object placement data ----
struct ChunkStaticObject {
    std::string modelPath;
    glm::vec3 position = {0, 0, 0};
    glm::vec3 rotation = {0, 0, 0};    // euler degrees
    glm::vec3 scale = {1, 1, 1};
    uint32_t flags = 0;                 // bit 0: hasCollision, bit 1: castShadows
};

// ---- Sound emitter data ----
struct SoundEmitter {
    std::string soundPath;
    glm::vec3 position = {0, 0, 0};
    float volume = 1.0f;
    float minRange = 5.0f;
    float maxRange = 20.0f;
    bool loop = true;
};

// ---- WorldChunk: the spatial container (64x64m) ----
class WorldChunk {
public:
    WorldChunk(int32_t chunkX, int32_t chunkZ);
    ~WorldChunk();

    int32_t GetChunkX() const { return m_ChunkX; }
    int32_t GetChunkZ() const { return m_ChunkZ; }

    // Terrain access
    TerrainChunk* GetTerrain() { return m_Terrain.get(); }
    const TerrainChunk* GetTerrain() const { return m_Terrain.get(); }

    // Non-terrain data
    std::vector<EditorLight>& GetLights() { m_Modified = true; return m_Lights; }
    const std::vector<EditorLight>& GetLights() const { return m_Lights; }
    std::vector<ChunkStaticObject>& GetObjects() { m_Modified = true; return m_Objects; }
    const std::vector<ChunkStaticObject>& GetObjects() const { return m_Objects; }
    std::vector<SoundEmitter>& GetSounds() { m_Modified = true; return m_Sounds; }
    const std::vector<SoundEmitter>& GetSounds() const { return m_Sounds; }

    // File I/O (.chunk section format)
    void Load(const std::string& filePath);
    void Save(const std::string& filePath);
    void Unload();

    // State (delegates terrain state + tracks non-terrain modifications)
    ChunkState GetState() const { return m_Terrain ? m_Terrain->GetState() : ChunkState::Unloaded; }
    bool IsReady() const { return m_Terrain && m_Terrain->IsReady(); }
    bool IsModified() const { return m_Modified || (m_Terrain && m_Terrain->IsModified()); }
    void ClearModified() { m_Modified = false; if (m_Terrain) m_Terrain->ClearModified(); }

    // Magic values for file format
    static constexpr uint32_t CHNK_MAGIC = 0x43484E4B;  // "CHNK"
    static constexpr uint32_t TERR_TAG   = 0x54455252;   // Section tag: terrain
    static constexpr uint32_t OBJS_TAG   = 0x4F424A53;   // Section tag: objects
    static constexpr uint32_t LGHT_TAG   = 0x4C474854;   // Section tag: lights
    static constexpr uint32_t SNDS_TAG   = 0x534E4453;   // Section tag: sounds

private:
    int32_t m_ChunkX, m_ChunkZ;
    bool m_Modified = false;

    std::unique_ptr<TerrainChunk> m_Terrain;
    std::vector<EditorLight> m_Lights;
    std::vector<ChunkStaticObject> m_Objects;
    std::vector<SoundEmitter> m_Sounds;

    // Section readers/writers
    void LoadTerrainSection(std::ifstream& file);
    void LoadLightsSection(std::ifstream& file);
    void LoadObjectsSection(std::ifstream& file);
    void LoadSoundsSection(std::ifstream& file);

    void SaveTerrainSection(std::ofstream& file);
    void SaveLightsSection(std::ofstream& file);
    void SaveObjectsSection(std::ofstream& file);
    void SaveSoundsSection(std::ofstream& file);
};

} // namespace Editor3D
