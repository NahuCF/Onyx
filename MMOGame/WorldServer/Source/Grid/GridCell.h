#pragma once

#include "GridDefines.h"
#include "../../../Shared/Source/Types/Types.h"
#include <unordered_set>
#include <cstdint>

namespace MMO {

// ============================================================
// GRID CELL
// ============================================================

class GridCell {
public:
    GridCell(CellCoord coord) : m_Coord(coord) {}

    CellCoord GetCoord() const { return m_Coord; }

    // Entity management
    void AddEntity(EntityId id) { m_Entities.insert(id); }
    void RemoveEntity(EntityId id) { m_Entities.erase(id); }
    bool HasEntity(EntityId id) const { return m_Entities.count(id) > 0; }

    // Player management (subset of entities)
    void AddPlayer(EntityId id) { m_Players.insert(id); }
    void RemovePlayer(EntityId id) { m_Players.erase(id); }
    bool HasPlayers() const { return !m_Players.empty(); }
    int GetPlayerCount() const { return static_cast<int>(m_Players.size()); }

    const std::unordered_set<EntityId>& GetEntities() const { return m_Entities; }
    const std::unordered_set<EntityId>& GetPlayers() const { return m_Players; }

    // Activation state (for mob spawning)
    GridCellState GetState() const { return m_State; }
    void SetState(GridCellState state) { m_State = state; }
    bool IsActive() const { return m_State == GridCellState::ACTIVE; }
    bool IsLoaded() const { return m_State == GridCellState::ACTIVE || m_State == GridCellState::UNLOADING; }

    // Unload timer (for delayed despawning)
    float GetUnloadTimer() const { return m_UnloadTimer; }
    void SetUnloadTimer(float timer) { m_UnloadTimer = timer; }
    void UpdateUnloadTimer(float dt) { m_UnloadTimer -= dt; }

    // Spawn points assigned to this grid cell
    void AddSpawnPoint(uint32_t spawnPointId) { m_SpawnPoints.insert(spawnPointId); }
    const std::unordered_set<uint32_t>& GetSpawnPoints() const { return m_SpawnPoints; }

    // Track which mobs were spawned by this grid (for cleanup)
    void AddSpawnedMob(EntityId mobId) { m_SpawnedMobs.insert(mobId); }
    void RemoveSpawnedMob(EntityId mobId) { m_SpawnedMobs.erase(mobId); }
    const std::unordered_set<EntityId>& GetSpawnedMobs() const { return m_SpawnedMobs; }
    void ClearSpawnedMobs() { m_SpawnedMobs.clear(); }

private:
    CellCoord m_Coord;
    std::unordered_set<EntityId> m_Entities;
    std::unordered_set<EntityId> m_Players;

    // Activation state
    GridCellState m_State = GridCellState::UNLOADED;
    float m_UnloadTimer = 0.0f;

    // Spawn management
    std::unordered_set<uint32_t> m_SpawnPoints;      // Spawn point IDs in this cell
    std::unordered_set<EntityId> m_SpawnedMobs;      // Mobs spawned by this cell
};

} // namespace MMO
