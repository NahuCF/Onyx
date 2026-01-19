#include "Chunk.h"
#include <fstream>
#include <iostream>
#include <cstring>

namespace MMO {

// ============================================================
// CHUNK FILE FORMAT
// ============================================================
// Header:
//   Magic: 4 bytes "CHNK" - identifies this as a chunk file
//   ChunkX: 4 bytes (int32)
//   ChunkY: 4 bytes (int32)
//   LayerCount: 2 bytes (uint16)
// Per Layer:
//   LayerId: 2 bytes (uint16)
//   TileData: CHUNK_SIZE * CHUNK_SIZE * sizeof(TileId) bytes
// Collision:
//   CollisionData: CHUNK_SIZE * CHUNK_SIZE / 8 bytes (bit-packed)
// ============================================================

constexpr uint32_t CHUNK_MAGIC = 0x4B4E4843;  // "CHNK" in little-endian

Chunk::Chunk() : m_Coord{0, 0}, m_Dirty(false) {
    ClearAll();
}

Chunk::Chunk(ChunkCoord coord) : m_Coord(coord), m_Dirty(false) {
    ClearAll();
}

TileId Chunk::GetTile(int32_t localX, int32_t localY, uint16_t layerId) const {
    if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 || localY >= CHUNK_SIZE) {
        return EMPTY_TILE;
    }
    auto it = m_Tiles.find(layerId);
    if (it == m_Tiles.end()) {
        return EMPTY_TILE;
    }
    return it->second[ToIndex(localX, localY)];
}

void Chunk::SetTile(int32_t localX, int32_t localY, uint16_t layerId, TileId tile) {
    if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 || localY >= CHUNK_SIZE) {
        return;
    }
    EnsureLayer(layerId);
    m_Tiles[layerId][ToIndex(localX, localY)] = tile;
    m_Dirty = true;
}

bool Chunk::GetCollision(int32_t localX, int32_t localY) const {
    if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 || localY >= CHUNK_SIZE) {
        return true;  // Out of bounds is blocked
    }
    return m_Collision[ToIndex(localX, localY)];
}

void Chunk::SetCollision(int32_t localX, int32_t localY, bool blocked) {
    if (localX < 0 || localX >= CHUNK_SIZE || localY < 0 || localY >= CHUNK_SIZE) {
        return;
    }
    m_Collision[ToIndex(localX, localY)] = blocked;
    m_Dirty = true;
}

void Chunk::Fill(uint16_t layerId, TileId tile) {
    EnsureLayer(layerId);
    for (auto& t : m_Tiles[layerId]) {
        t = tile;
    }
    m_Dirty = true;
}

void Chunk::ClearLayer(uint16_t layerId) {
    auto it = m_Tiles.find(layerId);
    if (it != m_Tiles.end()) {
        m_Tiles.erase(it);
        m_Dirty = true;
    }
}

void Chunk::ClearAll() {
    m_Tiles.clear();
    for (auto& c : m_Collision) {
        c = false;
    }
    m_Dirty = false;
}

bool Chunk::IsEmpty() const {
    for (const auto& [layerId, layerData] : m_Tiles) {
        for (const auto& t : layerData) {
            if (!t.IsEmpty()) {
                return false;
            }
        }
    }
    return true;
}

std::vector<uint16_t> Chunk::GetLayerIds() const {
    std::vector<uint16_t> ids;
    ids.reserve(m_Tiles.size());
    for (const auto& [layerId, layerData] : m_Tiles) {
        ids.push_back(layerId);
    }
    return ids;
}

void Chunk::EnsureLayer(uint16_t layerId) {
    if (m_Tiles.find(layerId) == m_Tiles.end()) {
        m_Tiles[layerId] = LayerTileData{};
        // Initialize all tiles to empty
        for (auto& t : m_Tiles[layerId]) {
            t = EMPTY_TILE;
        }
    }
}

const LayerTileData* Chunk::GetLayerData(uint16_t layerId) const {
    auto it = m_Tiles.find(layerId);
    if (it == m_Tiles.end()) {
        return nullptr;
    }
    return &it->second;
}

bool Chunk::SaveToFile(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open chunk file for writing: " << path << std::endl;
        return false;
    }

    // Write header
    file.write(reinterpret_cast<const char*>(&CHUNK_MAGIC), sizeof(CHUNK_MAGIC));
    file.write(reinterpret_cast<const char*>(&m_Coord.x), sizeof(m_Coord.x));
    file.write(reinterpret_cast<const char*>(&m_Coord.y), sizeof(m_Coord.y));

    // Write layer count
    uint16_t layerCount = static_cast<uint16_t>(m_Tiles.size());
    file.write(reinterpret_cast<const char*>(&layerCount), sizeof(layerCount));

    // Write each layer
    for (const auto& [layerId, layerData] : m_Tiles) {
        // Write layer ID
        file.write(reinterpret_cast<const char*>(&layerId), sizeof(layerId));
        // Write tile data
        file.write(reinterpret_cast<const char*>(layerData.data()),
                   layerData.size() * sizeof(TileId));
    }

    // Write collision (bit-packed)
    constexpr size_t collisionBytes = (CHUNK_SIZE * CHUNK_SIZE + 7) / 8;
    std::vector<uint8_t> collisionPacked(collisionBytes, 0);
    for (size_t i = 0; i < m_Collision.size(); ++i) {
        if (m_Collision[i]) {
            collisionPacked[i / 8] |= (1 << (i % 8));
        }
    }
    file.write(reinterpret_cast<const char*>(collisionPacked.data()), collisionBytes);

    file.close();
    return true;
}

bool Chunk::LoadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;  // File doesn't exist, which is OK for new chunks
    }

    // Read and verify header
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));

    if (magic != CHUNK_MAGIC) {
        std::cerr << "Invalid chunk file (bad magic): " << path << std::endl;
        return false;
    }

    // Read coordinate
    file.read(reinterpret_cast<char*>(&m_Coord.x), sizeof(m_Coord.x));
    file.read(reinterpret_cast<char*>(&m_Coord.y), sizeof(m_Coord.y));

    // Clear existing data
    m_Tiles.clear();

    // Read layer count and layers
    uint16_t layerCount;
    file.read(reinterpret_cast<char*>(&layerCount), sizeof(layerCount));

    for (uint16_t i = 0; i < layerCount; ++i) {
        uint16_t layerId;
        file.read(reinterpret_cast<char*>(&layerId), sizeof(layerId));

        LayerTileData layerData;
        file.read(reinterpret_cast<char*>(layerData.data()),
                  layerData.size() * sizeof(TileId));

        m_Tiles[layerId] = std::move(layerData);
    }

    // Read collision (bit-packed)
    constexpr size_t collisionBytes = (CHUNK_SIZE * CHUNK_SIZE + 7) / 8;
    std::vector<uint8_t> collisionPacked(collisionBytes, 0);
    file.read(reinterpret_cast<char*>(collisionPacked.data()), collisionBytes);
    for (size_t i = 0; i < m_Collision.size(); ++i) {
        m_Collision[i] = (collisionPacked[i / 8] & (1 << (i % 8))) != 0;
    }

    file.close();
    m_Dirty = false;
    return true;
}

} // namespace MMO
