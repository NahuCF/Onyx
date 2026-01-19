#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace MMO {

// Entry in the map registry (like Map.dbc)
struct MapEntry {
    uint32_t id = 0;
    std::string folder;     // Folder name (e.g., "StartingZone")
    std::string name;       // Display name (e.g., "Starting Zone")
};

// Map registry - singleton that holds all map definitions
// Similar to Map.dbc in AzerothCore
class MapRegistry {
public:
    static MapRegistry& Instance() {
        static MapRegistry instance;
        return instance;
    }

    // Load registry from file
    bool Load(const std::string& path);

    // Save registry to file
    bool Save(const std::string& path) const;

    // Get map entry by ID
    const MapEntry* GetEntry(uint32_t mapId) const;

    // Get map entry by folder name
    const MapEntry* GetEntryByFolder(const std::string& folder) const;

    // Get all entries
    const std::vector<MapEntry>& GetAllEntries() const { return m_Entries; }

    // Add/update entry (for editor)
    void AddEntry(const MapEntry& entry);

    // Remove entry by ID (for editor)
    void RemoveEntry(uint32_t mapId);

    // Get next available ID (for editor)
    uint32_t GetNextId() const;

    // Build full path to map folder
    std::string GetMapPath(uint32_t mapId, const std::string& mapsRoot) const;

private:
    MapRegistry() = default;
    ~MapRegistry() = default;
    MapRegistry(const MapRegistry&) = delete;
    MapRegistry& operator=(const MapRegistry&) = delete;

    std::vector<MapEntry> m_Entries;
    std::unordered_map<uint32_t, size_t> m_IdToIndex;
    std::unordered_map<std::string, size_t> m_FolderToIndex;

    void RebuildIndices();
};

} // namespace MMO
