#pragma once

#include "EditorDefines.h"
#include "TileMap.h"
#include <Graphics/Shader.h>
#include <Graphics/Buffers.h>
#include <memory>
#include <vector>
#include <unordered_map>

namespace MMO {

// ============================================================
// TILESET BATCH
// GPU buffers for rendering a group of tiles from one tileset
// ============================================================

struct TilesetBatch {
    std::unique_ptr<Onyx::VertexArray> vao;
    std::unique_ptr<Onyx::VertexBuffer> vbo;
    std::unique_ptr<Onyx::IndexBuffer> ebo;
    uint16_t tilesetIndex = 0;

    uint32_t GetIndexCount() const { return ebo ? ebo->GetCount() : 0; }
};

// ============================================================
// CHUNK CACHE
// Cached GPU batches for one chunk's flat layers
// ============================================================

struct ChunkCache {
    std::vector<TilesetBatch> batches;
    bool dirty = true;

    void Clear() { batches.clear(); }
};

// ============================================================
// TILE MAP RENDERER
// Handles GPU-cached rendering of tile maps
// ============================================================

class TileMapRenderer {
public:
    TileMapRenderer();
    ~TileMapRenderer() = default;

    // Initialize shader
    void Init();

    // Set the tile map to render
    void SetTileMap(TileMap* tileMap) { m_TileMap = tileMap; }

    // Cache invalidation
    void InvalidateAll();
    void InvalidateChunkAt(int32_t tileX, int32_t tileY);
    void InvalidateYSortCache();

    // Render the tile map
    // Camera and viewport info needed for culling and projection
    void Render(float cameraX, float cameraY, float cameraZoom,
                float viewportWidth, float viewportHeight);

private:
    void RebuildChunkCache(ChunkCoord coord);
    void RebuildYSortCache(float cameraX, float cameraY, float cameraZoom,
                           float viewportWidth, float viewportHeight);

private:
    TileMap* m_TileMap = nullptr;

    // Shader for tile rendering
    std::unique_ptr<Onyx::Shader> m_Shader;

    // Flat layers: cached per-chunk
    std::unordered_map<ChunkCoord, ChunkCache, ChunkCoordHash> m_ChunkCaches;

    // Y-sorted layers: cached globally (need cross-chunk sorting)
    std::vector<TilesetBatch> m_YSortCache;
    bool m_YSortCacheDirty = true;

    // Track view for Y-sort cache validity
    float m_CachedCameraX = 0.0f;
    float m_CachedCameraY = 0.0f;
    float m_CachedZoom = 1.0f;
    int32_t m_CachedViewRadius = 0;
};

} // namespace MMO
