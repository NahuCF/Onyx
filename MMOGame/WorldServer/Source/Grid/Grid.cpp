#include "Grid.h"
#include "../Map/Map.h"
#include "../Entity/Entity.h"
#include <iostream>
#include <cmath>

namespace MMO {

Grid::Grid(MapInstance* map) : m_Map(map) {}

void Grid::AddEntity(EntityId id, Vec2 position, bool isPlayer) {
    CellCoord coord = PositionToCell(position);
    GridCell* cell = GetOrCreateCell(coord);

    cell->AddEntity(id);
    if (isPlayer) {
        cell->AddPlayer(id);
    }

    m_EntityCells[id] = coord;

    // Mark as spawned (needs full data sent to nearby players)
    DirtyFlags flags;
    flags.spawned = true;
    m_DirtyFlags[id] = flags;
    m_DirtyEntities.insert(id);  // Add to dirty set for O(d) iteration
}

void Grid::RemoveEntity(EntityId id) {
    auto it = m_EntityCells.find(id);
    if (it == m_EntityCells.end()) return;

    CellCoord coord = it->second;
    GridCell* cell = GetCell(coord);
    if (cell) {
        cell->RemoveEntity(id);
        cell->RemovePlayer(id);
    }

    // Mark as despawned before removing tracking
    DirtyFlags flags;
    flags.despawned = true;
    m_DirtyFlags[id] = flags;
    m_DirtyEntities.insert(id);  // Add to dirty set for O(d) iteration

    m_EntityCells.erase(it);
}

void Grid::MoveEntity(EntityId id, Vec2 oldPos, Vec2 newPos, bool isPlayer) {
    CellCoord oldCoord = PositionToCell(oldPos);
    CellCoord newCoord = PositionToCell(newPos);

    // Update cell tracking if cell changed
    if (!(oldCoord == newCoord)) {
        GridCell* oldCell = GetCell(oldCoord);
        if (oldCell) {
            oldCell->RemoveEntity(id);
            if (isPlayer) {
                oldCell->RemovePlayer(id);
            }
        }

        GridCell* newCell = GetOrCreateCell(newCoord);
        newCell->AddEntity(id);
        if (isPlayer) {
            newCell->AddPlayer(id);
        }

        m_EntityCells[id] = newCoord;
    }

    // Mark position dirty
    auto& flags = m_DirtyFlags[id];
    flags.position = true;
    m_DirtyEntities.insert(id);  // Add to dirty set for O(d) iteration
}

GridCell* Grid::GetOrCreateCell(CellCoord coord) {
    auto it = m_Cells.find(coord);
    if (it != m_Cells.end()) {
        return it->second.get();
    }

    auto cell = std::make_unique<GridCell>(coord);
    GridCell* ptr = cell.get();
    m_Cells[coord] = std::move(cell);
    return ptr;
}

GridCell* Grid::GetCell(CellCoord coord) {
    auto it = m_Cells.find(coord);
    return it != m_Cells.end() ? it->second.get() : nullptr;
}

void Grid::MarkDirty(EntityId id, DirtyFlags flags) {
    auto& existing = m_DirtyFlags[id];
    // Combine flags
    if (flags.position) existing.position = true;
    if (flags.health) existing.health = true;
    if (flags.mana) existing.mana = true;
    if (flags.target) existing.target = true;
    if (flags.casting) existing.casting = true;
    if (flags.moveState) existing.moveState = true;
    if (flags.spawned) existing.spawned = true;
    if (flags.despawned) existing.despawned = true;

    // Add to dirty set for O(d) iteration (AzerothCore-style)
    m_DirtyEntities.insert(id);
}

DirtyFlags Grid::GetDirtyFlags(EntityId id) const {
    auto it = m_DirtyFlags.find(id);
    return it != m_DirtyFlags.end() ? it->second : DirtyFlags();
}

void Grid::ClearDirtyFlags(EntityId id) {
    auto it = m_DirtyFlags.find(id);
    if (it != m_DirtyFlags.end()) {
        it->second.Clear();
    }
}

void Grid::ClearAllDirtyFlags() {
    // Remove entries for despawned entities (no longer tracked)
    for (auto it = m_DirtyFlags.begin(); it != m_DirtyFlags.end();) {
        if (it->second.despawned) {
            it = m_DirtyFlags.erase(it);
        } else {
            it->second.Clear();
            ++it;
        }
    }

    // Clear dirty set (AzerothCore-style)
    m_DirtyEntities.clear();
}

std::vector<EntityId> Grid::GetEntitiesInRadius(Vec2 center, float radius) {
    std::vector<EntityId> result;
    float radiusSq = radius * radius;

    CellCoord centerCell = PositionToCell(center);
    int32_t cellRadius = static_cast<int32_t>(std::ceil(radius / GRID_CELL_SIZE)) + 1;

    for (int32_t dx = -cellRadius; dx <= cellRadius; dx++) {
        for (int32_t dy = -cellRadius; dy <= cellRadius; dy++) {
            CellCoord coord{centerCell.x + dx, centerCell.y + dy};
            GridCell* cell = GetCell(coord);
            if (!cell) continue;

            for (EntityId id : cell->GetEntities()) {
                Entity* entity = m_Map->GetEntity(id);
                if (!entity) continue;

                auto movement = entity->GetMovement();
                if (!movement) continue;

                float distSq = Vec2::DistanceSquared(center, movement->position);
                if (distSq <= radiusSq) {
                    result.push_back(id);
                }
            }
        }
    }

    return result;
}

std::vector<EntityId> Grid::GetPlayersInRadius(Vec2 center, float radius) {
    std::vector<EntityId> result;
    float radiusSq = radius * radius;

    CellCoord centerCell = PositionToCell(center);
    int32_t cellRadius = static_cast<int32_t>(std::ceil(radius / GRID_CELL_SIZE)) + 1;

    for (int32_t dx = -cellRadius; dx <= cellRadius; dx++) {
        for (int32_t dy = -cellRadius; dy <= cellRadius; dy++) {
            CellCoord coord{centerCell.x + dx, centerCell.y + dy};
            GridCell* cell = GetCell(coord);
            if (!cell) continue;

            for (EntityId id : cell->GetPlayers()) {
                Entity* entity = m_Map->GetEntity(id);
                if (!entity) continue;

                auto movement = entity->GetMovement();
                if (!movement) continue;

                float distSq = Vec2::DistanceSquared(center, movement->position);
                if (distSq <= radiusSq) {
                    result.push_back(id);
                }
            }
        }
    }

    return result;
}

std::vector<EntityId> Grid::GetNearbyPlayers(Vec2 position) {
    return GetPlayersInRadius(position, VIEW_DISTANCE);
}

void Grid::ForEachPlayerNear(Vec2 position, std::function<void(EntityId playerId)> callback) {
    auto players = GetNearbyPlayers(position);
    for (EntityId playerId : players) {
        callback(playerId);
    }
}

std::vector<EntityId> Grid::GetVisibleEntities(Vec2 playerPosition) {
    return GetEntitiesInRadius(playerPosition, VIEW_DISTANCE);
}

void Grid::GetDirtyEntitiesForPlayer(EntityId playerId, Vec2 playerPosition,
                                       std::vector<EntityId>& outSpawns,
                                       std::vector<EntityId>& outDespawns,
                                       std::vector<EntityId>& outUpdates) {
    // Get all entities in view range
    auto visibleEntities = GetVisibleEntities(playerPosition);

    for (EntityId entityId : visibleEntities) {
        if (entityId == playerId) continue;  // Skip self

        DirtyFlags flags = GetDirtyFlags(entityId);
        if (!flags.IsAnyDirty()) continue;

        if (flags.spawned) {
            outSpawns.push_back(entityId);
        } else if (flags.despawned) {
            outDespawns.push_back(entityId);
        } else {
            outUpdates.push_back(entityId);
        }
    }

    // Also check despawned entities that may no longer be in cells
    for (const auto& [entityId, flags] : m_DirtyFlags) {
        if (flags.despawned) {
            // Check if already added
            bool found = false;
            for (EntityId id : outDespawns) {
                if (id == entityId) { found = true; break; }
            }
            if (!found) {
                outDespawns.push_back(entityId);
            }
        }
    }
}

// ============================================================
// GRID ACTIVATION SYSTEM (AzerothCore-style)
// ============================================================

void Grid::RegisterSpawnPoint(uint32_t spawnPointId, Vec2 position) {
    CellCoord coord = PositionToCell(position);
    GridCell* cell = GetOrCreateCell(coord);
    cell->AddSpawnPoint(spawnPointId);
    m_CellsWithSpawns.insert(coord);

    std::cout << "[Grid] Registered spawn point " << spawnPointId
              << " at cell (" << coord.x << ", " << coord.y << ")" << std::endl;
}

void Grid::UpdateGridActivation(float dt,
                                 std::vector<CellCoord>& outCellsToLoad,
                                 std::vector<CellCoord>& outCellsToUnload) {
    // Check each cell that has spawn points
    for (const CellCoord& coord : m_CellsWithSpawns) {
        GridCell* cell = GetCell(coord);
        if (!cell) continue;

        bool hasPlayersNearby = HasPlayersNearCell(coord);
        GridCellState state = cell->GetState();

        switch (state) {
            case GridCellState::UNLOADED:
                // Check if we should load this cell
                if (hasPlayersNearby) {
                    cell->SetState(GridCellState::LOADING);
                    outCellsToLoad.push_back(coord);
                }
                break;

            case GridCellState::LOADING:
                // Waiting for MapInstance to spawn mobs and call ActivateCell()
                break;

            case GridCellState::ACTIVE:
                // Check if we should start unloading
                if (!hasPlayersNearby) {
                    cell->SetState(GridCellState::UNLOADING);
                    cell->SetUnloadTimer(GRID_UNLOAD_DELAY);
                }
                break;

            case GridCellState::UNLOADING:
                // Players came back?
                if (hasPlayersNearby) {
                    cell->SetState(GridCellState::ACTIVE);
                    cell->SetUnloadTimer(0.0f);
                } else {
                    // Count down timer
                    cell->UpdateUnloadTimer(dt);
                    if (cell->GetUnloadTimer() <= 0.0f) {
                        outCellsToUnload.push_back(coord);
                    }
                }
                break;
        }
    }
}

bool Grid::IsCellActive(CellCoord coord) const {
    auto it = m_Cells.find(coord);
    if (it == m_Cells.end()) return false;
    return it->second->IsActive();
}

const std::unordered_set<uint32_t>* Grid::GetCellSpawnPoints(CellCoord coord) const {
    auto it = m_Cells.find(coord);
    if (it == m_Cells.end()) return nullptr;
    return &it->second->GetSpawnPoints();
}

void Grid::RegisterSpawnedMob(CellCoord coord, EntityId mobId) {
    GridCell* cell = GetCell(coord);
    if (cell) {
        cell->AddSpawnedMob(mobId);
    }
}

void Grid::UnregisterSpawnedMob(CellCoord coord, EntityId mobId) {
    GridCell* cell = GetCell(coord);
    if (cell) {
        cell->RemoveSpawnedMob(mobId);
    }
}

std::vector<EntityId> Grid::GetCellSpawnedMobs(CellCoord coord) const {
    auto it = m_Cells.find(coord);
    if (it == m_Cells.end()) return {};

    const auto& mobs = it->second->GetSpawnedMobs();
    return std::vector<EntityId>(mobs.begin(), mobs.end());
}

void Grid::ActivateCell(CellCoord coord) {
    GridCell* cell = GetCell(coord);
    if (cell) {
        cell->SetState(GridCellState::ACTIVE);
        std::cout << "[Grid] Activated cell (" << coord.x << ", " << coord.y << ")" << std::endl;
    }
}

void Grid::DeactivateCell(CellCoord coord) {
    GridCell* cell = GetCell(coord);
    if (cell) {
        cell->SetState(GridCellState::UNLOADED);
        cell->ClearSpawnedMobs();
        std::cout << "[Grid] Deactivated cell (" << coord.x << ", " << coord.y << ")" << std::endl;
    }
}

bool Grid::HasPlayersNearCell(CellCoord coord) const {
    // Check this cell and adjacent cells for players
    for (int32_t dx = -GRID_SEARCH_RADIUS; dx <= GRID_SEARCH_RADIUS; dx++) {
        for (int32_t dy = -GRID_SEARCH_RADIUS; dy <= GRID_SEARCH_RADIUS; dy++) {
            CellCoord checkCoord{coord.x + dx, coord.y + dy};
            auto it = m_Cells.find(checkCoord);
            if (it != m_Cells.end() && it->second->HasPlayers()) {
                return true;
            }
        }
    }
    return false;
}

} // namespace MMO
