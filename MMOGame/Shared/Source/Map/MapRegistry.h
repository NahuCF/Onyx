#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace MMO {

// ============================================
// Map instance types - determines server behavior
// ============================================
enum class MapInstanceType {
    OpenWorld = 0,
    Dungeon = 1,
    Raid = 2,
    Battleground = 3,
    Arena = 4
};

const char* MapInstanceTypeName(MapInstanceType type);
MapInstanceType MapInstanceTypeFromString(const std::string& str);
std::string MapInstanceTypeToString(MapInstanceType type);

// ============================================
// Map entry - one row in the map registry
// ============================================
struct MapEntry {
    uint32_t id = 0;
    std::string internalName;   // Filesystem-safe identifier (e.g., "starting_zone")
    std::string displayName;    // Human-readable name (e.g., "Starting Zone")
    MapInstanceType instanceType = MapInstanceType::OpenWorld;
    int maxPlayers = 0;         // 0 = unlimited
};

// ============================================
// Map registry - holds all map definitions
// Similar to Map.dbc in AzerothCore
// ============================================
class MapRegistry {
public:
    static MapRegistry& Instance() {
        static MapRegistry instance;
        return instance;
    }

    bool Load(const std::string& path);
    bool Save(const std::string& path) const;

    const MapEntry* GetEntry(uint32_t mapId) const;
    const MapEntry* GetEntryByInternalName(const std::string& name) const;
    const std::vector<MapEntry>& GetAllEntries() const { return m_Entries; }

    void AddEntry(const MapEntry& entry);
    void RemoveEntry(uint32_t mapId);
    uint32_t GetNextId() const;

private:
    MapRegistry() = default;
    ~MapRegistry() = default;
    MapRegistry(const MapRegistry&) = delete;
    MapRegistry& operator=(const MapRegistry&) = delete;

    std::vector<MapEntry> m_Entries;
    std::unordered_map<uint32_t, size_t> m_IdToIndex;
    std::unordered_map<std::string, size_t> m_NameToIndex;

    void RebuildIndices();
};

} // namespace MMO
