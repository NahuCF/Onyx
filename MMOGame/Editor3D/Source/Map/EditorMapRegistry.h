#pragma once

#include <Map/MapRegistry.h>  // Shared: MapEntry, MapInstanceType
#include <string>
#include <vector>

namespace Editor3D {

// Re-export shared types for convenience
using MMO::MapInstanceType;
using MMO::MapEntry;
using MMO::MapInstanceTypeName;
using MMO::MapInstanceTypeFromString;
using MMO::MapInstanceTypeToString;

// Editor-specific map registry that adds directory management
// and nlohmann/json serialization on top of the shared types.
class EditorMapRegistry {
public:
    void Load(const std::string& mapsRoot);
    void Save();

    const std::vector<MapEntry>& GetMaps() const { return m_Maps; }
    const MapEntry* GetMapById(uint32_t id) const;
    uint32_t CreateMap(const std::string& name, MapInstanceType type, int maxPlayers);
    void DeleteMap(uint32_t id);
    void UpdateMap(const MapEntry& entry);

    std::string GetMapDirectory(uint32_t id) const;
    std::string GetChunksDirectory(uint32_t id) const;

    const std::string& GetMapsRoot() const { return m_MapsRoot; }

private:
    std::string m_MapsRoot;
    std::string m_MapsJsonPath;
    std::vector<MapEntry> m_Maps;

    uint32_t NextAvailableId() const;
    std::string MakeInternalName(const std::string& displayName) const;
    std::string FormatMapId(uint32_t id) const;
};

} // namespace Editor3D
