#pragma once

#include "GridDefines.h"
#include "GridCell.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <memory>

namespace MMO {

class Entity;
class MapInstance;

// ============================================================
// GRID
// ============================================================

class Grid {
public:
    Grid(MapInstance* map);
    ~Grid() = default;

    // Convert world position to cell coordinate
    static CellCoord PositionToCell(Vec2 position) {
        return CellCoord{
            static_cast<int32_t>(std::floor(position.x / GRID_CELL_SIZE)),
            static_cast<int32_t>(std::floor(position.y / GRID_CELL_SIZE))
        };
    }

    // Entity management
    void AddEntity(EntityId id, Vec2 position, bool isPlayer);
    void RemoveEntity(EntityId id);
    void MoveEntity(EntityId id, Vec2 oldPos, Vec2 newPos, bool isPlayer);

    // Get cell at position (creates if doesn't exist)
    GridCell* GetOrCreateCell(CellCoord coord);
    GridCell* GetCell(CellCoord coord);

    // Dirty flag management (AzerothCore-style with dirty set for O(d) iteration)
    void MarkDirty(EntityId id, DirtyFlags flags);
    DirtyFlags GetDirtyFlags(EntityId id) const;
    void ClearDirtyFlags(EntityId id);
    void ClearAllDirtyFlags();
    const std::unordered_set<EntityId>& GetDirtyEntities() const { return m_DirtyEntities; }

    // Query methods
    std::vector<EntityId> GetEntitiesInRadius(Vec2 center, float radius);
    std::vector<EntityId> GetPlayersInRadius(Vec2 center, float radius);
    std::vector<EntityId> GetNearbyPlayers(Vec2 position);

    // For broadcasting - get all players who can see a position
    void ForEachPlayerNear(Vec2 position, std::function<void(EntityId playerId)> callback);

    // For full world state sync - get all entities a player can see
    std::vector<EntityId> GetVisibleEntities(Vec2 playerPosition);

    // Get entities that need update for a specific player
    void GetDirtyEntitiesForPlayer(EntityId playerId, Vec2 playerPosition,
                                    std::vector<EntityId>& outSpawns,
                                    std::vector<EntityId>& outDespawns,
                                    std::vector<EntityId>& outUpdates);

    // ============================================================
    // GRID ACTIVATION SYSTEM (AzerothCore-style)
    // ============================================================

    // Register spawn points from map template (called during map init)
    void RegisterSpawnPoint(uint32_t spawnPointId, Vec2 position);

    // Update grid activation states (called every tick)
    // Returns cells that need mob spawning and cells that need mob despawning
    void UpdateGridActivation(float dt,
                              std::vector<CellCoord>& outCellsToLoad,
                              std::vector<CellCoord>& outCellsToUnload);

    // Check if a cell is active (has mobs loaded)
    bool IsCellActive(CellCoord coord) const;

    // Get all spawn points for a cell
    const std::unordered_set<uint32_t>* GetCellSpawnPoints(CellCoord coord) const;

    // Track spawned mobs per cell (for cleanup on unload)
    void RegisterSpawnedMob(CellCoord coord, EntityId mobId);
    void UnregisterSpawnedMob(CellCoord coord, EntityId mobId);
    std::vector<EntityId> GetCellSpawnedMobs(CellCoord coord) const;

    // Activate a cell (called after mobs are spawned)
    void ActivateCell(CellCoord coord);

    // Deactivate a cell (called after mobs are despawned)
    void DeactivateCell(CellCoord coord);

    // Check if any player is near a cell (within activation range)
    bool HasPlayersNearCell(CellCoord coord) const;

private:
    MapInstance* m_Map;
    std::unordered_map<CellCoord, std::unique_ptr<GridCell>, CellCoordHash> m_Cells;
    std::unordered_map<EntityId, CellCoord> m_EntityCells;  // Track which cell each entity is in
    std::unordered_map<EntityId, DirtyFlags> m_DirtyFlags;  // Per-entity dirty flags
    std::unordered_set<EntityId> m_DirtyEntities;           // Set of dirty entities for O(d) iteration

    // Cells that have spawn points (even if not yet active)
    std::unordered_set<CellCoord, CellCoordHash> m_CellsWithSpawns;
};

} // namespace MMO
