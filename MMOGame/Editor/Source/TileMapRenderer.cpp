#include "TileMapRenderer.h"
#include "Tileset.h"
#include <Graphics/RenderCommand.h>
#include <Graphics/VertexLayout.h>
#include <cmath>
#include <algorithm>
#include <climits>

namespace MMO {

// ============================================================
// VERTEX FORMAT
// ============================================================

struct TileVertex {
    float worldX, worldY;
    float cornerX, cornerY;
    float u, v;
};

// ============================================================
// HELPER FUNCTIONS
// ============================================================

static TilesetBatch CreateBatch(uint16_t tilesetIndex,
                                const std::vector<TileVertex>& vertices,
                                const std::vector<uint32_t>& indices) {
    TilesetBatch batch;
    batch.tilesetIndex = tilesetIndex;

    // Create buffers using Onyx abstractions
    batch.vbo = std::make_unique<Onyx::VertexBuffer>(
        vertices.data(),
        static_cast<uint32_t>(vertices.size() * sizeof(TileVertex))
    );
    batch.ebo = std::make_unique<Onyx::IndexBuffer>(
        indices.data(),
        static_cast<uint32_t>(indices.size() * sizeof(uint32_t))
    );
    batch.vao = std::make_unique<Onyx::VertexArray>();

    // Define vertex layout: worldPos(2) + corner(2) + texCoord(2)
    Onyx::VertexLayout layout;
    layout.PushFloat(2);  // a_WorldPos
    layout.PushFloat(2);  // a_Corner
    layout.PushFloat(2);  // a_TexCoord

    // Setup VAO with VBO, EBO, and layout
    batch.vao->SetVertexBuffer(batch.vbo.get());
    batch.vao->SetIndexBuffer(batch.ebo.get());
    batch.vao->SetLayout(layout);

    return batch;
}

static void AddTileToBuffers(int32_t worldX, int32_t worldY, TileId tile,
                             std::unordered_map<uint16_t, std::vector<TileVertex>>& tilesetVertices,
                             std::unordered_map<uint16_t, std::vector<uint32_t>>& tilesetIndices) {
    Tileset* tileset = TilesetManager::Instance().GetTileset(tile.tilesetIndex);
    if (!tileset) return;

    float u, v;
    tileset->GetTileUV(tile.tileIndex, u, v);

    float texWidth = static_cast<float>(tileset->GetTextureWidth());
    float texHeight = static_cast<float>(tileset->GetTextureHeight());
    float srcTileW = static_cast<float>(tileset->GetTileWidth());
    float srcTileH = static_cast<float>(tileset->GetTileHeight());

    float uvMinX = u / texWidth;
    float uvMinY = v / texHeight;
    float uvMaxX = (u + srcTileW) / texWidth;
    float uvMaxY = (v + srcTileH) / texHeight;

    float tileWorldX = static_cast<float>(worldX) + 0.5f;
    float tileWorldY = static_cast<float>(worldY) + 0.5f;

    auto& vertices = tilesetVertices[tile.tilesetIndex];
    auto& indices = tilesetIndices[tile.tilesetIndex];

    uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

    vertices.push_back({tileWorldX, tileWorldY, 0.0f, 0.0f, uvMinX, uvMinY});
    vertices.push_back({tileWorldX, tileWorldY, 1.0f, 0.0f, uvMaxX, uvMinY});
    vertices.push_back({tileWorldX, tileWorldY, 1.0f, 1.0f, uvMaxX, uvMaxY});
    vertices.push_back({tileWorldX, tileWorldY, 0.0f, 1.0f, uvMinX, uvMaxY});

    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

// ============================================================
// TILE MAP RENDERER IMPLEMENTATION
// ============================================================

TileMapRenderer::TileMapRenderer() = default;

void TileMapRenderer::Init() {
    m_Shader = std::make_unique<Onyx::Shader>(
        "MMOGame/Editor/assets/shaders/tile_cached.vert",
        "MMOGame/Editor/assets/shaders/tile_cached.frag"
    );
}

void TileMapRenderer::InvalidateAll() {
    for (auto& [coord, cache] : m_ChunkCaches) {
        cache.dirty = true;
    }
    m_YSortCacheDirty = true;
}

void TileMapRenderer::InvalidateChunkAt(int32_t tileX, int32_t tileY) {
    // Calculate which chunk this tile belongs to
    int32_t chunkX = static_cast<int32_t>(std::floor(static_cast<float>(tileX) / CHUNK_SIZE));
    int32_t chunkY = static_cast<int32_t>(std::floor(static_cast<float>(tileY) / CHUNK_SIZE));
    ChunkCoord coord{chunkX, chunkY};

    // Mark that chunk as dirty
    auto it = m_ChunkCaches.find(coord);
    if (it != m_ChunkCaches.end()) {
        it->second.dirty = true;
    }

    // Y-sorted layers need global rebuild since tiles interleave across chunks
    m_YSortCacheDirty = true;
}

void TileMapRenderer::InvalidateYSortCache() {
    m_YSortCacheDirty = true;
}

void TileMapRenderer::Render(float cameraX, float cameraY, float cameraZoom,
                             float viewportWidth, float viewportHeight) {
    if (!m_TileMap || !m_Shader) return;

    const auto& metadata = m_TileMap->GetMetadata();

    // Check if Y-sort cache needs rebuild due to camera movement
    float dx = std::abs(cameraX - m_CachedCameraX);
    float dy = std::abs(cameraY - m_CachedCameraY);
    float halfRadius = static_cast<float>(m_CachedViewRadius) * 0.5f;

    if (dx > halfRadius || dy > halfRadius || cameraZoom != m_CachedZoom) {
        m_YSortCacheDirty = true;
        m_CachedCameraX = cameraX;
        m_CachedCameraY = cameraY;
        m_CachedZoom = cameraZoom;
        m_CachedViewRadius = static_cast<int32_t>((viewportWidth + viewportHeight) /
            (metadata.tileHeight * cameraZoom)) + 5;
    }

    // Calculate visible chunk range
    int32_t viewRadius = static_cast<int32_t>((viewportWidth + viewportHeight) / (metadata.tileHeight * cameraZoom)) + 5;

    int32_t centerTileX = static_cast<int32_t>(cameraX);
    int32_t centerTileY = static_cast<int32_t>(cameraY);

    int32_t startTileX = centerTileX - viewRadius;
    int32_t startTileY = centerTileY - viewRadius;
    int32_t endTileX = centerTileX + viewRadius;
    int32_t endTileY = centerTileY + viewRadius;

    int32_t startChunkX = static_cast<int32_t>(std::floor(static_cast<float>(startTileX) / CHUNK_SIZE));
    int32_t startChunkY = static_cast<int32_t>(std::floor(static_cast<float>(startTileY) / CHUNK_SIZE));
    int32_t endChunkX = static_cast<int32_t>(std::floor(static_cast<float>(endTileX) / CHUNK_SIZE));
    int32_t endChunkY = static_cast<int32_t>(std::floor(static_cast<float>(endTileY) / CHUNK_SIZE));

    // Ensure visible chunks have valid caches
    for (int32_t chunkY = startChunkY; chunkY <= endChunkY; ++chunkY) {
        for (int32_t chunkX = startChunkX; chunkX <= endChunkX; ++chunkX) {
            ChunkCoord coord{chunkX, chunkY};
            auto it = m_ChunkCaches.find(coord);
            if (it == m_ChunkCaches.end() || it->second.dirty) {
                RebuildChunkCache(coord);
            }
        }
    }

    // Rebuild Y-sort cache if needed
    if (m_YSortCacheDirty) {
        RebuildYSortCache(cameraX, cameraY, cameraZoom, viewportWidth, viewportHeight);
    }

    // Setup shader
    m_Shader->Bind();

    m_Shader->SetVec2("u_ViewportSize", viewportWidth, viewportHeight);
    m_Shader->SetVec2("u_CameraPos", cameraX, cameraY);
    m_Shader->SetFloat("u_Zoom", cameraZoom);
    m_Shader->SetVec2("u_TileSize", static_cast<float>(metadata.tileWidth), static_cast<float>(metadata.tileHeight));

    float tileScreenW = static_cast<float>(metadata.tileWidth) * cameraZoom;
    float tileScreenH = static_cast<float>(metadata.tileHeight) * cameraZoom;
    m_Shader->SetVec2("u_TileScreenSize", tileScreenW, tileScreenH);

    m_Shader->SetVec3("u_ColorKey", 1.0f, 0.0f, 1.0f);
    m_Shader->SetFloat("u_ColorKeyThreshold", 0.1f);

    // Helper to render a batch
    auto renderBatch = [&](const TilesetBatch& batch) {
        uint32_t indexCount = batch.GetIndexCount();
        if (indexCount == 0) return;

        Tileset* tileset = TilesetManager::Instance().GetTileset(batch.tilesetIndex);
        if (!tileset) return;

        // Bind texture using Onyx abstraction
        tileset->GetTexture()->Bind(0);
        m_Shader->SetInt("u_Texture", 0);

        // Draw using RenderCommand
        Onyx::RenderCommand::DrawIndexed(*batch.vao, indexCount);
    };

    // Render flat layers from chunk caches
    for (int32_t chunkY = startChunkY; chunkY <= endChunkY; ++chunkY) {
        for (int32_t chunkX = startChunkX; chunkX <= endChunkX; ++chunkX) {
            auto it = m_ChunkCaches.find({chunkX, chunkY});
            if (it != m_ChunkCaches.end()) {
                for (const auto& batch : it->second.batches) {
                    renderBatch(batch);
                }
            }
        }
    }

    // Render Y-sorted layers
    for (const auto& batch : m_YSortCache) {
        renderBatch(batch);
    }

    Onyx::RenderCommand::ResetState();
}

void TileMapRenderer::RebuildChunkCache(ChunkCoord coord) {
    // Get or create chunk cache
    ChunkCache& cache = m_ChunkCaches[coord];
    cache.Clear();

    Chunk* chunk = m_TileMap->GetChunk(coord);
    if (!chunk) {
        cache.dirty = false;
        return;
    }

    const auto& metadata = m_TileMap->GetMetadata();
    const auto sortedLayers = metadata.GetSortedLayers();

    // Find Y-sort layer range to identify flat layers
    int32_t minYSortOrder = INT32_MAX;
    for (const auto& layer : sortedLayers) {
        if (layer.ySortEnabled && layer.visible) {
            minYSortOrder = std::min(minYSortOrder, layer.sortOrder);
        }
    }

    // Collect flat layers (before and after Y-sort)
    std::vector<const LayerDefinition*> flatLayers;
    for (const auto& layer : sortedLayers) {
        if (!layer.visible) continue;
        if (!layer.ySortEnabled) {
            flatLayers.push_back(&layer);
        }
    }

    // Collect tiles from flat layers for this chunk
    std::unordered_map<uint16_t, std::vector<TileVertex>> tilesetVertices;
    std::unordered_map<uint16_t, std::vector<uint32_t>> tilesetIndices;

    int32_t chunkWorldX = coord.x * CHUNK_SIZE;
    int32_t chunkWorldY = coord.y * CHUNK_SIZE;

    for (const auto* layerDef : flatLayers) {
        const LayerTileData* layerData = chunk->GetLayerData(layerDef->id);
        if (!layerData) continue;

        for (int32_t localY = 0; localY < CHUNK_SIZE; ++localY) {
            for (int32_t localX = 0; localX < CHUNK_SIZE; ++localX) {
                TileId tile = (*layerData)[localY * CHUNK_SIZE + localX];
                if (tile.IsEmpty()) continue;
                AddTileToBuffers(chunkWorldX + localX, chunkWorldY + localY, tile,
                                 tilesetVertices, tilesetIndices);
            }
        }
    }

    // Create GPU batches
    for (auto& [tilesetIndex, vertices] : tilesetVertices) {
        if (vertices.empty()) continue;
        cache.batches.push_back(CreateBatch(tilesetIndex, vertices, tilesetIndices[tilesetIndex]));
    }

    cache.dirty = false;
}

void TileMapRenderer::RebuildYSortCache(float cameraX, float cameraY, float cameraZoom,
                                        float viewportWidth, float viewportHeight) {
    // Clean up old Y-sort cache (RAII handles resource cleanup)
    m_YSortCache.clear();

    const auto& metadata = m_TileMap->GetMetadata();

    // Calculate visible tile range
    int32_t viewRadius = static_cast<int32_t>((viewportWidth + viewportHeight) / (metadata.tileHeight * cameraZoom)) + 5;

    int32_t centerTileX = static_cast<int32_t>(cameraX);
    int32_t centerTileY = static_cast<int32_t>(cameraY);

    int32_t startTileX = centerTileX - viewRadius;
    int32_t startTileY = centerTileY - viewRadius;
    int32_t endTileX = centerTileX + viewRadius;
    int32_t endTileY = centerTileY + viewRadius;

    int32_t startChunkX = static_cast<int32_t>(std::floor(static_cast<float>(startTileX) / CHUNK_SIZE));
    int32_t startChunkY = static_cast<int32_t>(std::floor(static_cast<float>(startTileY) / CHUNK_SIZE));
    int32_t endChunkX = static_cast<int32_t>(std::floor(static_cast<float>(endTileX) / CHUNK_SIZE));
    int32_t endChunkY = static_cast<int32_t>(std::floor(static_cast<float>(endTileY) / CHUNK_SIZE));

    // Get Y-sorted layers
    const auto sortedLayers = metadata.GetSortedLayers();
    std::vector<const LayerDefinition*> ySortLayers;
    for (const auto& layer : sortedLayers) {
        if (layer.visible && layer.ySortEnabled) {
            ySortLayers.push_back(&layer);
        }
    }

    if (ySortLayers.empty()) {
        m_YSortCacheDirty = false;
        return;
    }

    // Collect Y-sorted tiles from visible chunks
    struct YSortTile {
        int32_t worldX, worldY;
        int32_t depth;
        int32_t sortOrder;
        TileId tile;
    };
    std::vector<YSortTile> ySortTiles;

    for (int32_t chunkY = startChunkY; chunkY <= endChunkY; ++chunkY) {
        for (int32_t chunkX = startChunkX; chunkX <= endChunkX; ++chunkX) {
            Chunk* chunk = m_TileMap->GetChunk({chunkX, chunkY});
            if (!chunk) continue;

            int32_t chunkWorldX = chunkX * CHUNK_SIZE;
            int32_t chunkWorldY = chunkY * CHUNK_SIZE;

            for (const auto* layerDef : ySortLayers) {
                const LayerTileData* layerData = chunk->GetLayerData(layerDef->id);
                if (!layerData) continue;

                for (int32_t localY = 0; localY < CHUNK_SIZE; ++localY) {
                    for (int32_t localX = 0; localX < CHUNK_SIZE; ++localX) {
                        TileId tile = (*layerData)[localY * CHUNK_SIZE + localX];
                        if (tile.IsEmpty()) continue;

                        int32_t worldX = chunkWorldX + localX;
                        int32_t worldY = chunkWorldY + localY;

                        // Skip tiles outside visible range
                        if (worldX < startTileX || worldX > endTileX ||
                            worldY < startTileY || worldY > endTileY) continue;

                        ySortTiles.push_back({worldX, worldY, worldX + worldY, layerDef->sortOrder, tile});
                    }
                }
            }
        }
    }

    // Sort by depth, then by layer sortOrder
    std::sort(ySortTiles.begin(), ySortTiles.end(),
        [](const YSortTile& a, const YSortTile& b) {
            if (a.depth != b.depth) return a.depth < b.depth;
            return a.sortOrder < b.sortOrder;
        });

    // Build vertex buffers (must maintain sort order)
    std::unordered_map<uint16_t, std::vector<TileVertex>> tilesetVertices;
    std::unordered_map<uint16_t, std::vector<uint32_t>> tilesetIndices;

    for (const auto& info : ySortTiles) {
        AddTileToBuffers(info.worldX, info.worldY, info.tile, tilesetVertices, tilesetIndices);
    }

    // Create GPU batches
    for (auto& [tilesetIndex, vertices] : tilesetVertices) {
        if (vertices.empty()) continue;
        m_YSortCache.push_back(CreateBatch(tilesetIndex, vertices, tilesetIndices[tilesetIndex]));
    }

    m_YSortCacheDirty = false;
}

} // namespace MMO
