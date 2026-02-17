#include "MapRegistry.h"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace MMO {

// ============================================
// MapInstanceType helpers
// ============================================

const char* MapInstanceTypeName(MapInstanceType type) {
    switch (type) {
        case MapInstanceType::OpenWorld:     return "Open World";
        case MapInstanceType::Dungeon:       return "Dungeon";
        case MapInstanceType::Raid:          return "Raid";
        case MapInstanceType::Battleground:  return "Battleground";
        case MapInstanceType::Arena:         return "Arena";
    }
    return "Unknown";
}

MapInstanceType MapInstanceTypeFromString(const std::string& str) {
    if (str == "open_world")    return MapInstanceType::OpenWorld;
    if (str == "dungeon")       return MapInstanceType::Dungeon;
    if (str == "raid")          return MapInstanceType::Raid;
    if (str == "battleground")  return MapInstanceType::Battleground;
    if (str == "arena")         return MapInstanceType::Arena;
    return MapInstanceType::OpenWorld;
}

std::string MapInstanceTypeToString(MapInstanceType type) {
    switch (type) {
        case MapInstanceType::OpenWorld:     return "open_world";
        case MapInstanceType::Dungeon:       return "dungeon";
        case MapInstanceType::Raid:          return "raid";
        case MapInstanceType::Battleground:  return "battleground";
        case MapInstanceType::Arena:         return "arena";
    }
    return "open_world";
}

// ============================================
// MapRegistry
// ============================================

// Helper: extract a quoted string value after a key like "key": "value"
static std::string ExtractStringField(const std::string& line, const std::string& key) {
    size_t keyPos = line.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return "";

    size_t colonPos = line.find(':', keyPos);
    if (colonPos == std::string::npos) return "";

    size_t quoteStart = line.find('"', colonPos + 1);
    if (quoteStart == std::string::npos) return "";
    quoteStart++;

    size_t quoteEnd = line.find('"', quoteStart);
    if (quoteEnd == std::string::npos) return "";

    return line.substr(quoteStart, quoteEnd - quoteStart);
}

// Helper: extract an integer value after a key like "key": 123
static int ExtractIntField(const std::string& line, const std::string& key, int defaultVal = 0) {
    size_t keyPos = line.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return defaultVal;

    size_t colonPos = line.find(':', keyPos);
    if (colonPos == std::string::npos) return defaultVal;

    try {
        return std::stoi(line.substr(colonPos + 1));
    } catch (...) {
        return defaultVal;
    }
}

bool MapRegistry::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open map registry: " << path << std::endl;
        return false;
    }

    m_Entries.clear();

    // Read entire file to handle multi-line entries
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    // Find each map entry block between { and }
    size_t pos = content.find("\"maps\"");
    if (pos == std::string::npos) return false;

    pos = content.find('[', pos);
    if (pos == std::string::npos) return false;

    while (true) {
        size_t entryStart = content.find('{', pos);
        if (entryStart == std::string::npos) break;

        size_t entryEnd = content.find('}', entryStart);
        if (entryEnd == std::string::npos) break;

        // Check if we've gone past the end of the array
        size_t arrayEnd = content.find(']', pos);
        if (arrayEnd != std::string::npos && entryStart > arrayEnd) break;

        std::string entryStr = content.substr(entryStart, entryEnd - entryStart + 1);

        MapEntry entry;
        entry.id = static_cast<uint32_t>(ExtractIntField(entryStr, "id"));
        entry.internalName = ExtractStringField(entryStr, "internalName");
        entry.displayName = ExtractStringField(entryStr, "displayName");
        entry.instanceType = MapInstanceTypeFromString(
            ExtractStringField(entryStr, "instanceType"));
        entry.maxPlayers = ExtractIntField(entryStr, "maxPlayers");

        if (entry.id > 0 && !entry.displayName.empty()) {
            m_Entries.push_back(entry);
        }

        pos = entryEnd + 1;
    }

    RebuildIndices();
    std::cout << "Loaded " << m_Entries.size() << " maps from registry" << std::endl;
    return true;
}

bool MapRegistry::Save(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to save map registry: " << path << std::endl;
        return false;
    }

    file << "{\n";
    file << "    \"maps\": [\n";

    for (size_t i = 0; i < m_Entries.size(); ++i) {
        const auto& entry = m_Entries[i];
        file << "        {\n";
        file << "            \"id\": " << entry.id << ",\n";
        file << "            \"internalName\": \"" << entry.internalName << "\",\n";
        file << "            \"displayName\": \"" << entry.displayName << "\",\n";
        file << "            \"instanceType\": \"" << MapInstanceTypeToString(entry.instanceType) << "\",\n";
        file << "            \"maxPlayers\": " << entry.maxPlayers << "\n";
        file << "        }";
        if (i < m_Entries.size() - 1) file << ",";
        file << "\n";
    }

    file << "    ]\n";
    file << "}\n";

    file.close();
    return true;
}

const MapEntry* MapRegistry::GetEntry(uint32_t mapId) const {
    auto it = m_IdToIndex.find(mapId);
    if (it != m_IdToIndex.end()) {
        return &m_Entries[it->second];
    }
    return nullptr;
}

const MapEntry* MapRegistry::GetEntryByInternalName(const std::string& name) const {
    auto it = m_NameToIndex.find(name);
    if (it != m_NameToIndex.end()) {
        return &m_Entries[it->second];
    }
    return nullptr;
}

void MapRegistry::AddEntry(const MapEntry& entry) {
    auto it = m_IdToIndex.find(entry.id);
    if (it != m_IdToIndex.end()) {
        m_Entries[it->second] = entry;
    } else {
        m_Entries.push_back(entry);
    }
    RebuildIndices();
}

void MapRegistry::RemoveEntry(uint32_t mapId) {
    auto it = m_IdToIndex.find(mapId);
    if (it != m_IdToIndex.end()) {
        m_Entries.erase(m_Entries.begin() + it->second);
        RebuildIndices();
    }
}

uint32_t MapRegistry::GetNextId() const {
    uint32_t maxId = 0;
    for (const auto& entry : m_Entries) {
        if (entry.id > maxId) maxId = entry.id;
    }
    return maxId + 1;
}

void MapRegistry::RebuildIndices() {
    m_IdToIndex.clear();
    m_NameToIndex.clear();

    for (size_t i = 0; i < m_Entries.size(); ++i) {
        m_IdToIndex[m_Entries[i].id] = i;
        if (!m_Entries[i].internalName.empty()) {
            m_NameToIndex[m_Entries[i].internalName] = i;
        }
    }
}

} // namespace MMO
