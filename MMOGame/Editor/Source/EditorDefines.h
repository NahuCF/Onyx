#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

namespace MMO {

// ============================================================
// EDITOR CONSTANTS
// ============================================================

constexpr int32_t CHUNK_SIZE = 64;           // Tiles per chunk dimension
constexpr int32_t MAX_TILE_LAYERS = 16;      // Maximum supported layers

// Default tile dimensions (isometric 2:1 ratio)
constexpr int32_t DEFAULT_TILE_WIDTH = 160;   // Screen width of tile
constexpr int32_t DEFAULT_TILE_HEIGHT = 80;  // Screen height of tile

// ============================================================
// EDITOR ENUMS
// ============================================================

enum class EditorTool {
    SELECT,
    BRUSH,
    FILL,
    ERASER,
    ENTITY_PLACE
};

// ============================================================
// LAYER DEFINITION (dynamic layers)
// ============================================================

struct LayerDefinition {
    uint16_t id = 0;
    std::string name;
    bool visible = true;
    bool locked = false;
    int32_t sortOrder = 0;      // Lower = rendered first (behind)
    bool ySortEnabled = false;  // If true, tiles sort with entities by depth

    bool operator==(const LayerDefinition& other) const {
        return id == other.id;
    }
};

// ============================================================
// TILE DATA
// ============================================================

struct TileId {
    uint16_t tilesetIndex = 0;  // Which tileset
    uint16_t tileIndex = 0;     // Which tile in the tileset

    bool IsEmpty() const { return tilesetIndex == 0 && tileIndex == 0; }

    bool operator==(const TileId& other) const {
        return tilesetIndex == other.tilesetIndex && tileIndex == other.tileIndex;
    }

    bool operator!=(const TileId& other) const {
        return !(*this == other);
    }
};

// Empty tile constant
constexpr TileId EMPTY_TILE = { 0, 0 };

// ============================================================
// CHUNK COORDINATE
// ============================================================

struct ChunkCoord {
    int32_t x = 0;
    int32_t y = 0;

    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y;
    }
};

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& coord) const {
        return std::hash<int64_t>()((static_cast<int64_t>(coord.x) << 32) |
                                    static_cast<uint32_t>(coord.y));
    }
};

// ============================================================
// TILESET INFO (for saving/loading)
// ============================================================

struct TilesetInfo {
    std::string path;
    int32_t tileWidth = 32;   // Source tile width in tileset image
    int32_t tileHeight = 32;  // Source tile height in tileset image
};

// ============================================================
// MAP METADATA
// ============================================================

struct MapMetadata {
    uint32_t id = 0;
    std::string name = "Untitled Map";
    int32_t chunkSize = CHUNK_SIZE;
    int32_t tileWidth = DEFAULT_TILE_WIDTH;    // Screen width of one tile
    int32_t tileHeight = DEFAULT_TILE_HEIGHT;  // Screen height of one tile
    int32_t worldWidthChunks = 10;
    int32_t worldHeightChunks = 10;
    std::string defaultTileset;

    // Camera position
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float cameraZoom = 1.0f;

    // Loaded tilesets
    std::vector<TilesetInfo> tilesets;

    // Dynamic layers
    std::vector<LayerDefinition> layers = {
        {0, "Ground",   true, false, 0,   false},  // Y-Sort OFF - always behind
        {1, "Details",  true, false, 10,  false},  // Y-Sort OFF - ground decor
        {2, "Objects",  true, false, 50,  true},   // Y-Sort ON  - sorts with entities
        {3, "Overhead", true, false, 100, false},  // Y-Sort OFF - always on top
    };
    uint16_t nextLayerId = 4;

    // Helper methods
    const LayerDefinition* GetLayer(uint16_t id) const {
        for (const auto& layer : layers) {
            if (layer.id == id) return &layer;
        }
        return nullptr;
    }

    LayerDefinition* GetLayer(uint16_t id) {
        for (auto& layer : layers) {
            if (layer.id == id) return &layer;
        }
        return nullptr;
    }

    std::vector<LayerDefinition> GetSortedLayers() const {
        std::vector<LayerDefinition> sorted = layers;
        std::sort(sorted.begin(), sorted.end(),
            [](const LayerDefinition& a, const LayerDefinition& b) {
                return a.sortOrder < b.sortOrder;
            });
        return sorted;
    }
};

} // namespace MMO
