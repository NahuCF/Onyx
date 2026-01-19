#pragma once

#include "EditorDefines.h"
#include <array>
#include <vector>
#include <string>
#include <unordered_map>

namespace MMO {

// ============================================================
// CHUNK
// A chunk contains CHUNK_SIZE x CHUNK_SIZE tiles across multiple layers
// ============================================================

// Type alias for layer tile data
using LayerTileData = std::array<TileId, CHUNK_SIZE * CHUNK_SIZE>;

class Chunk {
public:
    Chunk();
    Chunk(ChunkCoord coord);
    ~Chunk() = default;

    // Tile access (by layer ID)
    TileId GetTile(int32_t localX, int32_t localY, uint16_t layerId) const;
    void SetTile(int32_t localX, int32_t localY, uint16_t layerId, TileId tile);

    // Collision access
    bool GetCollision(int32_t localX, int32_t localY) const;
    void SetCollision(int32_t localX, int32_t localY, bool blocked);

    // Fill operations
    void Fill(uint16_t layerId, TileId tile);
    void ClearLayer(uint16_t layerId);
    void ClearAll();

    // Check if chunk has any non-empty tiles
    bool IsEmpty() const;

    // Check if a specific layer exists in this chunk
    bool HasLayer(uint16_t layerId) const { return m_Tiles.find(layerId) != m_Tiles.end(); }

    // Get all layer IDs that have data in this chunk
    std::vector<uint16_t> GetLayerIds() const;

    // Ensure a layer exists (creates empty layer if needed)
    void EnsureLayer(uint16_t layerId);

    // Coordinate
    ChunkCoord GetCoord() const { return m_Coord; }

    // Dirty flag for saving
    bool IsDirty() const { return m_Dirty; }
    void ClearDirty() { m_Dirty = false; }
    void MarkDirty() { m_Dirty = true; }

    // Serialization
    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);

    // Get raw data for rendering (returns nullptr if layer doesn't exist)
    const LayerTileData* GetLayerData(uint16_t layerId) const;

private:
    int32_t ToIndex(int32_t x, int32_t y) const {
        return y * CHUNK_SIZE + x;
    }

    ChunkCoord m_Coord;
    bool m_Dirty = false;

    // Tile data for each layer (sparse - only contains layers with data)
    std::unordered_map<uint16_t, LayerTileData> m_Tiles;

    // Collision grid
    std::array<bool, CHUNK_SIZE * CHUNK_SIZE> m_Collision;
};

} // namespace MMO
