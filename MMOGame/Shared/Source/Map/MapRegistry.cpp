#include "MapRegistry.h"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace MMO {

bool MapRegistry::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open map registry: " << path << std::endl;
        return false;
    }

    m_Entries.clear();

    // Simple JSON parsing
    std::string line;
    MapEntry currentEntry;
    bool inMapsArray = false;

    while (std::getline(file, line)) {
        // Check for maps array
        if (line.find("\"maps\"") != std::string::npos) {
            inMapsArray = true;
            continue;
        }

        if (inMapsArray && line.find(']') != std::string::npos && line.find('[') == std::string::npos) {
            inMapsArray = false;
            continue;
        }

        // Parse map entry
        if (inMapsArray && line.find('{') != std::string::npos) {
            currentEntry = MapEntry{};

            // Extract id
            size_t idStart = line.find("\"id\"");
            if (idStart != std::string::npos) {
                size_t valueStart = line.find(':', idStart) + 1;
                currentEntry.id = static_cast<uint32_t>(std::stoul(line.substr(valueStart)));
            }

            // Extract folder
            size_t folderStart = line.find("\"folder\"");
            if (folderStart != std::string::npos) {
                size_t valueStart = line.find(':', folderStart) + 1;
                size_t quoteStart = line.find('"', valueStart) + 1;
                size_t quoteEnd = line.find('"', quoteStart);
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                    currentEntry.folder = line.substr(quoteStart, quoteEnd - quoteStart);
                }
            }

            // Extract name
            size_t nameStart = line.find("\"name\"");
            if (nameStart != std::string::npos) {
                size_t valueStart = line.find(':', nameStart) + 1;
                size_t quoteStart = line.find('"', valueStart) + 1;
                size_t quoteEnd = line.find('"', quoteStart);
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                    currentEntry.name = line.substr(quoteStart, quoteEnd - quoteStart);
                }
            }

            if (currentEntry.id > 0 && !currentEntry.folder.empty()) {
                m_Entries.push_back(currentEntry);
            }
        }
    }

    file.close();
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
    file << "  \"maps\": [\n";

    for (size_t i = 0; i < m_Entries.size(); ++i) {
        const auto& entry = m_Entries[i];
        file << "    { \"id\": " << entry.id
             << ", \"folder\": \"" << entry.folder << "\""
             << ", \"name\": \"" << entry.name << "\" }";
        if (i < m_Entries.size() - 1) {
            file << ",";
        }
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();
    std::cout << "Saved " << m_Entries.size() << " maps to registry" << std::endl;
    return true;
}

const MapEntry* MapRegistry::GetEntry(uint32_t mapId) const {
    auto it = m_IdToIndex.find(mapId);
    if (it != m_IdToIndex.end()) {
        return &m_Entries[it->second];
    }
    return nullptr;
}

const MapEntry* MapRegistry::GetEntryByFolder(const std::string& folder) const {
    auto it = m_FolderToIndex.find(folder);
    if (it != m_FolderToIndex.end()) {
        return &m_Entries[it->second];
    }
    return nullptr;
}

void MapRegistry::AddEntry(const MapEntry& entry) {
    // Check if ID already exists
    auto it = m_IdToIndex.find(entry.id);
    if (it != m_IdToIndex.end()) {
        // Update existing
        m_Entries[it->second] = entry;
    } else {
        // Add new
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
        if (entry.id > maxId) {
            maxId = entry.id;
        }
    }
    return maxId + 1;
}

std::string MapRegistry::GetMapPath(uint32_t mapId, const std::string& mapsRoot) const {
    const MapEntry* entry = GetEntry(mapId);
    if (!entry) {
        return "";
    }
    return mapsRoot + "/" + entry->folder;
}

void MapRegistry::RebuildIndices() {
    m_IdToIndex.clear();
    m_FolderToIndex.clear();

    for (size_t i = 0; i < m_Entries.size(); ++i) {
        m_IdToIndex[m_Entries[i].id] = i;
        m_FolderToIndex[m_Entries[i].folder] = i;
    }
}

} // namespace MMO
