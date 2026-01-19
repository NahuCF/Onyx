#include "MapManager.h"
#include "MapInstance.h"
#include "MapTemplates.h"
#include "Items/Items.h"
#include <iostream>

namespace MMO {

// ============================================================
// MAP MANAGER
// ============================================================

void MapManager::Initialize() {
    // Initialize item templates
    ItemTemplateManager::Instance().Initialize();

    // Register map templates
    m_Templates[1] = MapTemplates::CreateStartingZone();
    m_Templates[2] = MapTemplates::CreateDarkForest();

    std::cout << "[MapManager] Initialized with " << m_Templates.size() << " map templates" << std::endl;
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

    // Remove from old instance
    fromInstance->UnregisterPlayer(playerId);

    // Get or create destination instance
    MapInstance* toInstance = GetMapInstance(toTemplateId);
    if (!toInstance) return INVALID_ENTITY_ID;

    // Create player in new instance
    Entity* newPlayer = toInstance->CreatePlayer(characterId, name, charClass, level, position,
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
