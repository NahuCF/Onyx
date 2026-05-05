#include "MapManager.h"
#include "MapInstance.h"
#include "Items/Items.h"
#include "../../../Shared/Source/Database/Database.h"
#include <iostream>

namespace MMO {

// ============================================================
// MAP MANAGER
// ============================================================

void MapManager::Initialize(Database& db) {
    // Initialize item templates
    ItemTemplateManager::Instance().Initialize();

    // Load all map templates from database
    auto dbTemplates = db.LoadAllMapTemplates();

    if (dbTemplates.empty()) {
        std::cerr << "[MapManager] WARNING: No maps found in database! "
                  << "Apply schema migrations and seed map_template rows." << std::endl;
    }

    for (auto& t : dbTemplates) {
        MapTemplate tmpl;
        tmpl.id = t.id;
        tmpl.name = t.name;
        tmpl.width = t.width;
        tmpl.height = t.height;
        tmpl.spawnPoint = Vec2(t.spawnX, t.spawnY);

        // Load portals for this map
        auto dbPortals = db.LoadPortals(t.id);
        uint32_t portalIdx = 1;
        for (auto& p : dbPortals) {
            Portal portal;
            portal.id = portalIdx++;
            portal.position = Vec2(p.positionX, p.positionY);
            portal.size = Vec2(p.sizeX, p.sizeY);
            portal.destMapId = p.destMapId;
            portal.destPosition = Vec2(p.destX, p.destY);
            tmpl.portals.push_back(portal);
        }

        // Load creature spawns for this map
        auto dbSpawns = db.LoadCreatureSpawns(t.id);
        uint32_t spawnIdx = 1;
        for (auto& s : dbSpawns) {
            MobSpawnPoint spawn;
            spawn.id = spawnIdx++;
            spawn.guid = s.guid;
            spawn.creatureTemplateId = s.creatureTemplateId;
            spawn.position = Vec2(s.positionX, s.positionY);
            spawn.positionZ = s.positionZ;
            spawn.orientation = s.orientation;
            spawn.respawnTimeOverride = s.respawnTime;
            spawn.wanderRadius = s.wanderRadius;
            spawn.maxCount = s.maxCount;
            tmpl.mobSpawns.push_back(spawn);
        }

        m_Templates[tmpl.id] = std::move(tmpl);
    }

    std::cout << "[MapManager] Initialized with " << m_Templates.size() << " map templates from database" << std::endl;
}

MapInstance* MapManager::GetMapInstance(uint32_t templateId) {
    // Check if instance already exists
    for (auto& [id, instance] : m_Instances) {
        if (instance->GetTemplateId() == templateId) {
            return instance.get();
        }
    }

    // Create new instance
    const MapTemplate* tmpl = GetTemplate(templateId);
    if (!tmpl) {
        std::cout << "[MapManager] Unknown template: " << templateId << std::endl;
        return nullptr;
    }

    uint32_t instanceId = m_NextInstanceId++;
    auto instance = std::make_unique<MapInstance>(instanceId, tmpl);
    instance->SpawnInitialMobs();

    MapInstance* ptr = instance.get();
    m_Instances[instanceId] = std::move(instance);

    std::cout << "[MapManager] Created instance " << instanceId << " for map '" << tmpl->name << "'" << std::endl;
    return ptr;
}

MapInstance* MapManager::GetInstanceById(uint32_t instanceId) {
    auto it = m_Instances.find(instanceId);
    return it != m_Instances.end() ? it->second.get() : nullptr;
}

const MapTemplate* MapManager::GetTemplate(uint32_t templateId) {
    auto it = m_Templates.find(templateId);
    return it != m_Templates.end() ? &it->second : nullptr;
}

void MapManager::Update(float dt) {
    for (auto& [id, instance] : m_Instances) {
        instance->Update(dt);
    }
}

EntityId MapManager::TransferPlayer(EntityId playerId, uint32_t fromInstanceId,
                                     uint32_t toTemplateId, Vec2 position,
                                     uint32_t peerId, CharacterId characterId, AccountId accountId) {
    // Get source instance
    MapInstance* fromInstance = GetInstanceById(fromInstanceId);
    if (!fromInstance) return INVALID_ENTITY_ID;

    // Get player data before removing
    Entity* oldPlayer = fromInstance->GetEntity(playerId);
    if (!oldPlayer) return INVALID_ENTITY_ID;

    std::string name = oldPlayer->GetName();
    CharacterClass charClass = oldPlayer->GetClass();
    uint32_t level = oldPlayer->GetLevel();
    int32_t currentHealth = oldPlayer->GetHealth() ? oldPlayer->GetHealth()->current : 100;
    int32_t currentMana = oldPlayer->GetMana() ? oldPlayer->GetMana()->current : 100;
    uint32_t currentMoney = oldPlayer->GetWallet() ? oldPlayer->GetWallet()->IsCopper : 0;
    auto* oldMove = oldPlayer->GetMovement();
    float currentHeight = oldMove ? oldMove->height : 0.0f;
    float currentOrientation = oldMove ? oldMove->rotation : 0.0f;

    // Remove from old instance
    fromInstance->UnregisterPlayer(playerId);

    // Get or create destination instance
    MapInstance* toInstance = GetMapInstance(toTemplateId);
    if (!toInstance) return INVALID_ENTITY_ID;

    // Create player in new instance
    Entity* newPlayer = toInstance->CreatePlayer(characterId, name, charClass, level, position,
                                                  currentHeight, currentOrientation,
                                                  currentHealth, currentMana);
    if (!newPlayer) return INVALID_ENTITY_ID;

    // Restore wallet money
    if (newPlayer->GetWallet()) {
        newPlayer->GetWallet()->IsCopper = currentMoney;
    }

    EntityId newEntityId = newPlayer->GetId();
    toInstance->RegisterPlayer(newEntityId, peerId, characterId, accountId);

    std::cout << "[MapManager] Transferred player " << name << " from instance " << fromInstanceId
              << " to " << toInstance->GetInstanceId() << std::endl;

    return newEntityId;
}

} // namespace MMO
