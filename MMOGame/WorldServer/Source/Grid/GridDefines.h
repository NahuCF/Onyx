#pragma once

#include "../../../Shared/Source/Types/Types.h"
#include <cstdint>
#include <cmath>

namespace MMO {

// ============================================================
// GRID CONSTANTS
// ============================================================

constexpr float GRID_CELL_SIZE = 32.0f;      // Size of each grid cell
constexpr float VIEW_DISTANCE = 50.0f;       // How far players can see
constexpr int32_t GRID_SEARCH_RADIUS = 2;    // Cells to search in each direction (covers VIEW_DISTANCE)
constexpr float GRID_UNLOAD_DELAY = 30.0f;   // Seconds before unloading inactive grid

// ============================================================
// CELL COORDINATE
// ============================================================

struct CellCoord {
    int32_t x = 0;
    int32_t y = 0;

    bool operator==(const CellCoord& other) const {
        return x == other.x && y == other.y;
    }
};

struct CellCoordHash {
    size_t operator()(const CellCoord& coord) const {
        // Simple hash combining x and y
        return std::hash<int32_t>()(coord.x) ^ (std::hash<int32_t>()(coord.y) << 16);
    }
};

// ============================================================
// DIRTY FLAGS (Granular per-entity)
// ============================================================

struct DirtyFlags {
    bool position : 1;      // Position changed
    bool health : 1;        // Health changed
    bool mana : 1;          // Mana changed
    bool target : 1;        // Target changed
    bool casting : 1;       // Casting state changed
    bool moveState : 1;     // Move state changed
    bool spawned : 1;       // Just spawned (needs full data)
    bool despawned : 1;     // Just despawned (needs removal)

    DirtyFlags() : position(false), health(false), mana(false),
                   target(false), casting(false), moveState(false),
                   spawned(false), despawned(false) {}

    void Clear() {
        position = health = mana = target = casting = moveState = spawned = despawned = false;
    }

    bool IsAnyDirty() const {
        return position || health || mana || target || casting || moveState || spawned || despawned;
    }

    void MarkAll() {
        position = health = mana = target = casting = moveState = true;
    }
};

// ============================================================
// GRID CELL STATE
// ============================================================

enum class GridCellState : uint8_t {
    UNLOADED = 0,   // No mobs spawned, not active
    LOADING = 1,    // Currently loading mobs
    ACTIVE = 2,     // Mobs spawned, players nearby
    UNLOADING = 3   // No players, waiting to unload
};

} // namespace MMO
