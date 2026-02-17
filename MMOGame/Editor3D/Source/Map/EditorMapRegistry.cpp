#include "EditorMapRegistry.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace Editor3D {

using json = nlohmann::json;

void EditorMapRegistry::Load(const std::string& mapsRoot) {
    m_MapsRoot = mapsRoot;
    m_MapsJsonPath = m_MapsRoot + "/maps.json";
    m_Maps.clear();

    std::filesystem::create_directories(m_MapsRoot);

    if (!std::filesystem::exists(m_MapsJsonPath)) {
        Save();
        return;
    }

    std::ifstream file(m_MapsJsonPath);
    if (!file.is_open()) return;

    json j;
    try {
        file >> j;
    } catch (...) {
        return;
    }

    if (!j.contains("maps") || !j["maps"].is_array()) return;

    for (const auto& mapJson : j["maps"]) {
        MapEntry entry;
        entry.id = mapJson.value("id", 0u);
        entry.internalName = mapJson.value("internalName", "");
        entry.displayName = mapJson.value("displayName", "");
        entry.instanceType = MapInstanceTypeFromString(
            mapJson.value("instanceType", "open_world"));
        entry.maxPlayers = mapJson.value("maxPlayers", 0);

        if (entry.id > 0 && !entry.displayName.empty()) {
            m_Maps.push_back(entry);
        }
    }
}

void EditorMapRegistry::Save() {
    json j;
    json mapsArray = json::array();

    for (const auto& entry : m_Maps) {
        json mapJson;
        mapJson["id"] = entry.id;
        mapJson["internalName"] = entry.internalName;
        mapJson["displayName"] = entry.displayName;
        mapJson["instanceType"] = MapInstanceTypeToString(entry.instanceType);
        mapJson["maxPlayers"] = entry.maxPlayers;
        mapsArray.push_back(mapJson);
    }

    j["maps"] = mapsArray;

    std::filesystem::create_directories(m_MapsRoot);

    std::ofstream file(m_MapsJsonPath);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

const MapEntry* EditorMapRegistry::GetMapById(uint32_t id) const {
    for (const auto& entry : m_Maps) {
        if (entry.id == id) return &entry;
    }
    return nullptr;
}

uint32_t EditorMapRegistry::CreateMap(const std::string& name, MapInstanceType type, int maxPlayers) {
    MapEntry entry;
    entry.id = NextAvailableId();
    entry.internalName = MakeInternalName(name);
    entry.displayName = name;
    entry.instanceType = type;
    entry.maxPlayers = maxPlayers;

    std::string mapDir = GetMapDirectory(entry.id);
    std::filesystem::create_directories(mapDir + "/chunks");

    m_Maps.push_back(entry);
    Save();

    return entry.id;
}

void EditorMapRegistry::DeleteMap(uint32_t id) {
    auto it = std::find_if(m_Maps.begin(), m_Maps.end(),
        [id](const MapEntry& e) { return e.id == id; });

    if (it != m_Maps.end()) {
        std::string mapDir = GetMapDirectory(id);
        if (std::filesystem::exists(mapDir)) {
            std::filesystem::remove_all(mapDir);
        }
        m_Maps.erase(it);
        Save();
    }
}

void EditorMapRegistry::UpdateMap(const MapEntry& entry) {
    for (auto& existing : m_Maps) {
        if (existing.id == entry.id) {
            existing.displayName = entry.displayName;
            existing.maxPlayers = entry.maxPlayers;
            Save();
            return;
        }
    }
}

std::string EditorMapRegistry::GetMapDirectory(uint32_t id) const {
    return m_MapsRoot + "/" + FormatMapId(id);
}

std::string EditorMapRegistry::GetChunksDirectory(uint32_t id) const {
    return GetMapDirectory(id) + "/chunks";
}

uint32_t EditorMapRegistry::NextAvailableId() const {
    uint32_t maxId = 0;
    for (const auto& entry : m_Maps) {
        if (entry.id > maxId) maxId = entry.id;
    }
    return maxId + 1;
}

std::string EditorMapRegistry::MakeInternalName(const std::string& displayName) const {
    std::string result;
    result.reserve(displayName.size());
    for (char c : displayName) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else if (c == ' ' && !result.empty() && result.back() != '_') {
            result += '_';
        }
    }
    if (!result.empty() && result.back() == '_') {
        result.pop_back();
    }
    return result;
}

std::string EditorMapRegistry::FormatMapId(uint32_t id) const {
    char buf[8];
    snprintf(buf, sizeof(buf), "%03u", id);
    return std::string(buf);
}

} // namespace Editor3D
