#pragma once

#include "MapDefines.h"
#include "../../../Shared/Source/Types/Types.h"
#include <unordered_map>
#include <memory>

namespace MMO {

// Forward declarations
class MapInstance;

// ============================================================
// MAP MANAGER (Singleton)
// ============================================================

class MapManager {
public:
    static MapManager& Instance() {
        static MapManager instance;
        return instance;
    }

    // Initialize with map templates
    void Initialize();

    // Get map instance (creates if needed for world maps)
    MapInstance* GetMapInstance(uint32_t templateId);

    // Get map by instance ID
    MapInstance* GetInstanceById(uint32_t instanceId);

    // Get template
    const MapTemplate* GetTemplate(uint32_t templateId);

    // Update all maps
    void Update(float dt);

    // Transfer player between maps (returns new entity ID)
    EntityId TransferPlayer(EntityId playerId, uint32_t fromInstanceId,
                            uint32_t toTemplateId, Vec2 position,
                            uint32_t peerId, CharacterId characterId, AccountId accountId);

    // Get all active map instances
    const std::unordered_map<uint32_t, std::unique_ptr<MapInstance>>& GetAllInstances() const {
        return m_Instances;
    }

    // Global entity ID generation (prevents ID collision across maps)
    EntityId GenerateGlobalEntityId() { return m_NextGlobalEntityId++; }

private:
    MapManager() = default;
    ~MapManager() = default;
    MapManager(const MapManager&) = delete;
    MapManager& operator=(const MapManager&) = delete;

    std::unordered_map<uint32_t, MapTemplate> m_Templates;
    std::unordered_map<uint32_t, std::unique_ptr<MapInstance>> m_Instances;
    uint32_t m_NextInstanceId = 1;
    EntityId m_NextGlobalEntityId = 1;  // Global counter across all maps
};

} // namespace MMO
