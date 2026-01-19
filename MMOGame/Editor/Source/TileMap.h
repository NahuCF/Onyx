#pragma once

#include "EditorDefines.h"
#include "Chunk.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>

namespace MMO {

// ============================================================
// TILEMAP
// Manages all chunks for a map, handles streaming
// ============================================================

class TileMap {
public:
    TileMap();
    ~TileMap() = default;

    // Create new map
    void Create(const MapMetadata& metadata);

    // Load existing map
    bool Load(const std::string& mapDirectory);

    // Save map
    bool Save(const std::string& mapDirectory);

    // Get/set metadata
    const MapMetadata& GetMetadata() const { return m_Metadata; }
    MapMetadata& GetMetadata() { return m_Metadata; }
    void SetMetadata(const MapMetadata& metadata) { m_Metadata = metadata; }

    // World coordinate to chunk/local coordinate conversion
    ChunkCoord WorldToChunk(float worldX, float worldY) const;
    void WorldToLocal(float worldX, float worldY, ChunkCoord& outChunk, int32_t& outLocalX, int32_t& outLocalY) const;
    void LocalToWorld(ChunkCoord chunk, int32_t localX, int32_t localY, float& outWorldX, float& outWorldY) const;

    // Tile access (world coordinates, tiles)
    TileId GetTile(int32_t worldTileX, int32_t worldTileY, uint16_t layerId);
    void SetTile(int32_t worldTileX, int32_t worldTileY, uint16_t layerId, TileId tile);

    // Collision access
    bool GetCollision(int32_t worldTileX, int32_t worldTileY);
    void SetCollision(int32_t worldTileX, int32_t worldTileY, bool blocked);

    // Get chunk (loads if needed)
    Chunk* GetOrCreateChunk(ChunkCoord coord);
    Chunk* GetChunk(ChunkCoord coord);

    // Chunk streaming - load chunks around a position
    void UpdateLoadedChunks(float centerX, float centerY, int32_t radiusChunks = 2);

    // Get all loaded chunks for rendering
    const std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash>& GetLoadedChunks() const {
        return m_Chunks;
    }

    // Save dirty chunks
    void SaveDirtyChunks();

    // Check if any changes are unsaved
    bool HasUnsavedChanges() const;

    // Get map directory
    const std::string& GetMapDirectory() const { return m_MapDirectory; }

private:
    std::string GetChunkFilePath(ChunkCoord coord) const;
    void LoadChunk(ChunkCoord coord);
    void UnloadChunk(ChunkCoord coord);

    MapMetadata m_Metadata;
    std::string m_MapDirectory;

    // Loaded chunks
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_Chunks;

    // Center of loaded area
    ChunkCoord m_LoadCenter;
    int32_t m_LoadRadius = 2;
};

} // namespace MMO
