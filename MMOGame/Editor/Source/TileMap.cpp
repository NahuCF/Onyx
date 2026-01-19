#include "TileMap.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cmath>

namespace fs = std::filesystem;

namespace MMO {

TileMap::TileMap() : m_LoadCenter{0, 0}, m_LoadRadius(2) {
}

void TileMap::Create(const MapMetadata& metadata) {
    m_Metadata = metadata;
    m_Chunks.clear();
    m_MapDirectory.clear();
}

bool TileMap::Load(const std::string& mapDirectory) {
    m_MapDirectory = mapDirectory;

    // Load metadata
    std::string metaPath = mapDirectory + "/map.json";
    std::ifstream metaFile(metaPath);
    if (!metaFile.is_open()) {
        std::cerr << "Failed to open map metadata: " << metaPath << std::endl;
        return false;
    }

    // Clear previous data
    m_Metadata.tilesets.clear();
    m_Metadata.layers.clear();

    // Simple JSON parsing (basic implementation)
    std::string line;
    bool inTilesetsArray = false;
    bool inLayersArray = false;
    TilesetInfo currentTileset;
    LayerDefinition currentLayer;

    while (std::getline(metaFile, line)) {
        // Check for layers array start/end
        if (line.find("\"layers\"") != std::string::npos && line.find('[') != std::string::npos) {
            inLayersArray = true;
            continue;
        }
        if (inLayersArray && line.find(']') != std::string::npos && line.find('[') == std::string::npos) {
            inLayersArray = false;
            continue;
        }

        // Parse layer entries
        if (inLayersArray && line.find('{') != std::string::npos) {
            currentLayer = LayerDefinition{};

            // Extract id
            size_t idStart = line.find("\"id\"");
            if (idStart != std::string::npos) {
                size_t valueStart = line.find(':', idStart) + 1;
                currentLayer.id = static_cast<uint16_t>(std::stoi(line.substr(valueStart)));
            }

            // Extract name
            size_t nameStart = line.find("\"name\"");
            if (nameStart != std::string::npos) {
                size_t valueStart = line.find(':', nameStart) + 1;
                size_t quoteStart = line.find('"', valueStart) + 1;
                size_t quoteEnd = line.find('"', quoteStart);
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                    currentLayer.name = line.substr(quoteStart, quoteEnd - quoteStart);
                }
            }

            // Extract visible
            size_t visibleStart = line.find("\"visible\"");
            if (visibleStart != std::string::npos) {
                currentLayer.visible = (line.find("true", visibleStart) != std::string::npos);
            }

            // Extract locked
            size_t lockedStart = line.find("\"locked\"");
            if (lockedStart != std::string::npos) {
                currentLayer.locked = (line.find("true", lockedStart) != std::string::npos);
            }

            // Extract sortOrder
            size_t sortStart = line.find("\"sortOrder\"");
            if (sortStart != std::string::npos) {
                size_t valueStart = line.find(':', sortStart) + 1;
                currentLayer.sortOrder = std::stoi(line.substr(valueStart));
            }

            m_Metadata.layers.push_back(currentLayer);
            continue;
        }

        // Check for tilesets array start/end
        if (line.find("\"tilesets\"") != std::string::npos && line.find('[') != std::string::npos) {
            inTilesetsArray = true;
            continue;
        }
        if (inTilesetsArray && line.find(']') != std::string::npos && line.find('[') == std::string::npos) {
            inTilesetsArray = false;
            continue;
        }

        // Parse tileset entries
        if (inTilesetsArray && line.find('{') != std::string::npos) {
            currentTileset = TilesetInfo{};

            // Extract path
            size_t pathStart = line.find("\"path\"");
            if (pathStart != std::string::npos) {
                size_t valueStart = line.find(':', pathStart) + 1;
                size_t quoteStart = line.find('"', valueStart) + 1;
                size_t quoteEnd = line.find('"', quoteStart);
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                    currentTileset.path = line.substr(quoteStart, quoteEnd - quoteStart);
                }
            }

            // Extract tileWidth
            size_t widthStart = line.find("\"tileWidth\"");
            if (widthStart != std::string::npos) {
                size_t valueStart = line.find(':', widthStart) + 1;
                currentTileset.tileWidth = std::stoi(line.substr(valueStart));
            }

            // Extract tileHeight
            size_t heightStart = line.find("\"tileHeight\"");
            if (heightStart != std::string::npos) {
                size_t valueStart = line.find(':', heightStart) + 1;
                currentTileset.tileHeight = std::stoi(line.substr(valueStart));
            }

            if (!currentTileset.path.empty()) {
                m_Metadata.tilesets.push_back(currentTileset);
            }
            continue;
        }

        // Parse "key": value pairs for non-array fields
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        // Remove quotes and whitespace
        auto stripQuotes = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\""));
            s.erase(s.find_last_not_of(" \t\",[]{}") + 1);
        };
        stripQuotes(key);
        stripQuotes(value);

        if (key == "id") m_Metadata.id = std::stoul(value);
        else if (key == "name") m_Metadata.name = value;
        else if (key == "chunkSize") m_Metadata.chunkSize = std::stoi(value);
        else if (key == "tileWidth") m_Metadata.tileWidth = std::stoi(value);
        else if (key == "tileHeight") m_Metadata.tileHeight = std::stoi(value);
        else if (key == "worldWidthChunks") m_Metadata.worldWidthChunks = std::stoi(value);
        else if (key == "worldHeightChunks") m_Metadata.worldHeightChunks = std::stoi(value);
        else if (key == "defaultTileset") m_Metadata.defaultTileset = value;
        else if (key == "cameraX") m_Metadata.cameraX = std::stof(value);
        else if (key == "cameraY") m_Metadata.cameraY = std::stof(value);
        else if (key == "cameraZoom") m_Metadata.cameraZoom = std::stof(value);
        else if (key == "nextLayerId") m_Metadata.nextLayerId = static_cast<uint16_t>(std::stoi(value));
    }

    // If no layers were loaded, use default layers
    if (m_Metadata.layers.empty()) {
        m_Metadata.layers = {
            {0, "Ground", true, false, 0},
            {1, "Decoration", true, false, 1},
            {2, "Overlay", true, false, 2}
        };
        m_Metadata.nextLayerId = 3;
    }

    metaFile.close();

    // Clear existing chunks
    m_Chunks.clear();

    std::cout << "Loaded map '" << m_Metadata.name << "' from " << mapDirectory << std::endl;
    return true;
}

bool TileMap::Save(const std::string& mapDirectory) {
    m_MapDirectory = mapDirectory;

    // Create directories
    try {
        std::cout << "Saving to: " << mapDirectory << std::endl;
        fs::create_directories(mapDirectory);
        fs::create_directories(mapDirectory + "/chunks");
    } catch (const std::system_error& e) {
        std::cerr << "Failed to create directory: " << mapDirectory << std::endl;
        std::cerr << "Error: " << e.what() << " (code: " << e.code() << ")" << std::endl;
        return false;
    }

    // Save metadata
    std::string metaPath = mapDirectory + "/map.json";
    std::ofstream metaFile(metaPath);
    if (!metaFile.is_open()) {
        std::cerr << "Failed to create map metadata: " << metaPath << std::endl;
        return false;
    }

    metaFile << "{\n";
    metaFile << "  \"id\": " << m_Metadata.id << ",\n";
    metaFile << "  \"name\": \"" << m_Metadata.name << "\",\n";
    metaFile << "  \"chunkSize\": " << m_Metadata.chunkSize << ",\n";
    metaFile << "  \"tileWidth\": " << m_Metadata.tileWidth << ",\n";
    metaFile << "  \"tileHeight\": " << m_Metadata.tileHeight << ",\n";
    metaFile << "  \"worldWidthChunks\": " << m_Metadata.worldWidthChunks << ",\n";
    metaFile << "  \"worldHeightChunks\": " << m_Metadata.worldHeightChunks << ",\n";
    metaFile << "  \"defaultTileset\": \"" << m_Metadata.defaultTileset << "\",\n";
    metaFile << "  \"cameraX\": " << m_Metadata.cameraX << ",\n";
    metaFile << "  \"cameraY\": " << m_Metadata.cameraY << ",\n";
    metaFile << "  \"cameraZoom\": " << m_Metadata.cameraZoom << ",\n";
    metaFile << "  \"nextLayerId\": " << m_Metadata.nextLayerId << ",\n";
    metaFile << "  \"layers\": [\n";
    for (size_t i = 0; i < m_Metadata.layers.size(); ++i) {
        const auto& layer = m_Metadata.layers[i];
        metaFile << "    { \"id\": " << layer.id
                 << ", \"name\": \"" << layer.name << "\""
                 << ", \"visible\": " << (layer.visible ? "true" : "false")
                 << ", \"locked\": " << (layer.locked ? "true" : "false")
                 << ", \"sortOrder\": " << layer.sortOrder << " }";
        if (i < m_Metadata.layers.size() - 1) metaFile << ",";
        metaFile << "\n";
    }
    metaFile << "  ],\n";
    metaFile << "  \"tilesets\": [\n";
    for (size_t i = 0; i < m_Metadata.tilesets.size(); ++i) {
        const auto& ts = m_Metadata.tilesets[i];
        metaFile << "    { \"path\": \"" << ts.path << "\", \"tileWidth\": " << ts.tileWidth << ", \"tileHeight\": " << ts.tileHeight << " }";
        if (i < m_Metadata.tilesets.size() - 1) metaFile << ",";
        metaFile << "\n";
    }
    metaFile << "  ]\n";
    metaFile << "}\n";
    metaFile.close();

    // Save all dirty chunks
    SaveDirtyChunks();

    std::cout << "Saved map '" << m_Metadata.name << "' to " << mapDirectory << std::endl;
    return true;
}

ChunkCoord TileMap::WorldToChunk(float worldTileX, float worldTileY) const {
    // Input is in tile coordinates, convert to chunk coordinates
    return ChunkCoord{
        static_cast<int32_t>(std::floor(worldTileX / m_Metadata.chunkSize)),
        static_cast<int32_t>(std::floor(worldTileY / m_Metadata.chunkSize))
    };
}

void TileMap::WorldToLocal(float worldTileX, float worldTileY, ChunkCoord& outChunk,
                           int32_t& outLocalX, int32_t& outLocalY) const {
    // Input is in tile coordinates
    int32_t tileX = static_cast<int32_t>(std::floor(worldTileX));
    int32_t tileY = static_cast<int32_t>(std::floor(worldTileY));

    outChunk.x = static_cast<int32_t>(std::floor(static_cast<float>(tileX) / m_Metadata.chunkSize));
    outChunk.y = static_cast<int32_t>(std::floor(static_cast<float>(tileY) / m_Metadata.chunkSize));

    // Handle negative coordinates properly
    outLocalX = tileX - outChunk.x * m_Metadata.chunkSize;
    outLocalY = tileY - outChunk.y * m_Metadata.chunkSize;

    // Ensure local coords are positive
    if (outLocalX < 0) {
        outLocalX += m_Metadata.chunkSize;
        outChunk.x--;
    }
    if (outLocalY < 0) {
        outLocalY += m_Metadata.chunkSize;
        outChunk.y--;
    }
}

void TileMap::LocalToWorld(ChunkCoord chunk, int32_t localX, int32_t localY,
                           float& outWorldTileX, float& outWorldTileY) const {
    // Output is in tile coordinates
    outWorldTileX = static_cast<float>(chunk.x * m_Metadata.chunkSize + localX);
    outWorldTileY = static_cast<float>(chunk.y * m_Metadata.chunkSize + localY);
}

TileId TileMap::GetTile(int32_t worldTileX, int32_t worldTileY, uint16_t layerId) {
    int32_t chunkX = static_cast<int32_t>(std::floor(static_cast<float>(worldTileX) / m_Metadata.chunkSize));
    int32_t chunkY = static_cast<int32_t>(std::floor(static_cast<float>(worldTileY) / m_Metadata.chunkSize));

    int32_t localX = worldTileX - chunkX * m_Metadata.chunkSize;
    int32_t localY = worldTileY - chunkY * m_Metadata.chunkSize;

    Chunk* chunk = GetChunk(ChunkCoord{chunkX, chunkY});
    if (!chunk) {
        return EMPTY_TILE;
    }

    return chunk->GetTile(localX, localY, layerId);
}

void TileMap::SetTile(int32_t worldTileX, int32_t worldTileY, uint16_t layerId, TileId tile) {
    int32_t chunkX = static_cast<int32_t>(std::floor(static_cast<float>(worldTileX) / m_Metadata.chunkSize));
    int32_t chunkY = static_cast<int32_t>(std::floor(static_cast<float>(worldTileY) / m_Metadata.chunkSize));

    int32_t localX = worldTileX - chunkX * m_Metadata.chunkSize;
    int32_t localY = worldTileY - chunkY * m_Metadata.chunkSize;

    Chunk* chunk = GetOrCreateChunk(ChunkCoord{chunkX, chunkY});
    if (chunk) {
        chunk->SetTile(localX, localY, layerId, tile);
    }
}

bool TileMap::GetCollision(int32_t worldTileX, int32_t worldTileY) {
    int32_t chunkX = static_cast<int32_t>(std::floor(static_cast<float>(worldTileX) / m_Metadata.chunkSize));
    int32_t chunkY = static_cast<int32_t>(std::floor(static_cast<float>(worldTileY) / m_Metadata.chunkSize));

    int32_t localX = worldTileX - chunkX * m_Metadata.chunkSize;
    int32_t localY = worldTileY - chunkY * m_Metadata.chunkSize;

    Chunk* chunk = GetChunk(ChunkCoord{chunkX, chunkY});
    if (!chunk) {
        return false;
    }

    return chunk->GetCollision(localX, localY);
}

void TileMap::SetCollision(int32_t worldTileX, int32_t worldTileY, bool blocked) {
    int32_t chunkX = static_cast<int32_t>(std::floor(static_cast<float>(worldTileX) / m_Metadata.chunkSize));
    int32_t chunkY = static_cast<int32_t>(std::floor(static_cast<float>(worldTileY) / m_Metadata.chunkSize));

    int32_t localX = worldTileX - chunkX * m_Metadata.chunkSize;
    int32_t localY = worldTileY - chunkY * m_Metadata.chunkSize;

    Chunk* chunk = GetOrCreateChunk(ChunkCoord{chunkX, chunkY});
    if (chunk) {
        chunk->SetCollision(localX, localY, blocked);
    }
}

Chunk* TileMap::GetOrCreateChunk(ChunkCoord coord) {
    auto it = m_Chunks.find(coord);
    if (it != m_Chunks.end()) {
        return it->second.get();
    }

    // Create new chunk
    auto chunk = std::make_unique<Chunk>(coord);

    // Try to load from file if exists
    if (!m_MapDirectory.empty()) {
        chunk->LoadFromFile(GetChunkFilePath(coord));
    }

    Chunk* ptr = chunk.get();
    m_Chunks[coord] = std::move(chunk);
    return ptr;
}

Chunk* TileMap::GetChunk(ChunkCoord coord) {
    auto it = m_Chunks.find(coord);
    if (it != m_Chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

void TileMap::UpdateLoadedChunks(float centerX, float centerY, int32_t radiusChunks) {
    ChunkCoord newCenter = WorldToChunk(centerX, centerY);

    // Load chunks in radius
    for (int32_t dy = -radiusChunks; dy <= radiusChunks; ++dy) {
        for (int32_t dx = -radiusChunks; dx <= radiusChunks; ++dx) {
            ChunkCoord coord{newCenter.x + dx, newCenter.y + dy};
            GetOrCreateChunk(coord);  // This loads if needed
        }
    }

    // Unload chunks outside radius
    std::vector<ChunkCoord> toUnload;
    for (const auto& [coord, chunk] : m_Chunks) {
        int32_t distX = std::abs(coord.x - newCenter.x);
        int32_t distY = std::abs(coord.y - newCenter.y);
        if (distX > radiusChunks + 1 || distY > radiusChunks + 1) {
            toUnload.push_back(coord);
        }
    }

    for (const auto& coord : toUnload) {
        UnloadChunk(coord);
    }

    m_LoadCenter = newCenter;
    m_LoadRadius = radiusChunks;
}

void TileMap::SaveDirtyChunks() {
    if (m_MapDirectory.empty()) {
        std::cerr << "Cannot save chunks: no map directory set" << std::endl;
        return;
    }

    fs::create_directories(m_MapDirectory + "/chunks");

    for (auto& [coord, chunk] : m_Chunks) {
        if (chunk->IsDirty()) {
            std::string path = GetChunkFilePath(coord);
            if (chunk->SaveToFile(path)) {
                chunk->ClearDirty();
            }
        }
    }
}

bool TileMap::HasUnsavedChanges() const {
    for (const auto& [coord, chunk] : m_Chunks) {
        if (chunk->IsDirty()) {
            return true;
        }
    }
    return false;
}

std::string TileMap::GetChunkFilePath(ChunkCoord coord) const {
    return m_MapDirectory + "/chunks/" + std::to_string(coord.x) + "_" + std::to_string(coord.y) + ".chunk";
}

void TileMap::LoadChunk(ChunkCoord coord) {
    if (m_Chunks.find(coord) != m_Chunks.end()) {
        return;  // Already loaded
    }

    auto chunk = std::make_unique<Chunk>(coord);
    if (!m_MapDirectory.empty()) {
        chunk->LoadFromFile(GetChunkFilePath(coord));
    }
    m_Chunks[coord] = std::move(chunk);
}

void TileMap::UnloadChunk(ChunkCoord coord) {
    auto it = m_Chunks.find(coord);
    if (it == m_Chunks.end()) {
        return;
    }

    // Save if dirty
    if (it->second->IsDirty() && !m_MapDirectory.empty()) {
        it->second->SaveToFile(GetChunkFilePath(coord));
    }

    m_Chunks.erase(it);
}

} // namespace MMO
