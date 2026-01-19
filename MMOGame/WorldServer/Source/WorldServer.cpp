#include "WorldServer.h"
#include "Grid/Grid.h"
#include "Items/Items.h"
#include <iostream>
#include <thread>

namespace MMO {

WorldServer::WorldServer()
    : m_Running(false)
    , m_ServerTick(0) {
}

WorldServer::~WorldServer() {
    Stop();
}

bool WorldServer::Initialize(uint16_t port, const std::string& dbConnectionString) {
    if (!m_Network.Start(port)) {
        std::cerr << "Failed to start World Server on port " << port << std::endl;
        return false;
    }

    // Connect to database if connection string provided
    if (!dbConnectionString.empty()) {
        if (!m_Database.Connect(dbConnectionString)) {
            std::cerr << "Warning: Failed to connect to database, running without persistence" << std::endl;
        }
    }

    // Initialize map manager with templates
    MapManager::Instance().Initialize();

    std::cout << "World Server initialized on port " << port << std::endl;
    return true;
}

void WorldServer::Run() {
    m_Running = true;
    m_LastTick = std::chrono::steady_clock::now();

    std::cout << "World Server running at " << TICK_RATE << " Hz..." << std::endl;

    while (m_Running) {
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - m_LastTick).count();

        // Poll network events
        std::vector<NetworkEvent> events;
        m_Network.Poll(events, 1);

        for (const auto& event : events) {
            switch (event.type) {
                case NetworkEventType::CONNECTED:
                    OnPlayerConnect(event.peerId);
                    break;
                case NetworkEventType::DISCONNECTED:
                    OnPlayerDisconnect(event.peerId);
                    break;
                case NetworkEventType::DATA_RECEIVED:
                    ProcessPacket(event.peerId, event.data);
                    break;
            }
        }

        // Game tick
        if (elapsed >= TICK_INTERVAL) {
            m_LastTick = now;
            m_ServerTick++;

            // Update all map instances
            MapManager::Instance().Update(TICK_INTERVAL);

            // Send world state and events for each map
            for (auto& [instanceId, mapInstance] : MapManager::Instance().GetAllInstances()) {
                // Send world state to all players in this map
                SendWorldState(mapInstance.get());

                // Send game events
                SendEvents(mapInstance.get());
                mapInstance->ClearEvents();

                // Send aura updates (to nearby players only)
                SendAuraUpdates(mapInstance.get());

                // Send spawn/despawn notifications
                SendSpawnsAndDespawns(mapInstance.get());

                // Clear dirty flags after all updates sent (AzerothCore-style)
                mapInstance->GetGrid().ClearAllDirtyFlags();
            }

            // Cleanup expired auth tokens
            auto currentTime = std::chrono::steady_clock::now();
            for (auto it = m_PendingAuths.begin(); it != m_PendingAuths.end();) {
                if (currentTime > it->second.expiresAt) {
                    it = m_PendingAuths.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // Small sleep to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void WorldServer::Stop() {
    // Save all connected players before shutdown
    for (const auto& [peerId, player] : m_ConnectedPlayers) {
        SavePlayer(player);
    }

    m_Running = false;
    m_Network.Stop();
}

void WorldServer::AddPendingAuth(const std::string& token, CharacterId characterId, AccountId accountId) {
    PendingAuth auth;
    auth.token = token;
    auth.characterId = characterId;
    auth.accountId = accountId;
    auth.expiresAt = std::chrono::steady_clock::now() + std::chrono::minutes(5);
    m_PendingAuths[token] = auth;
}

// ============================================================
// NETWORK EVENT HANDLERS
// ============================================================

void WorldServer::OnPlayerConnect(uint32_t peerId) {
    std::cout << "Client connected: " << peerId << std::endl;
}

void WorldServer::OnPlayerDisconnect(uint32_t peerId) {
    std::cout << "Client disconnected: " << peerId << std::endl;

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    const ConnectedPlayer& player = it->second;

    // Save player data
    SavePlayer(player);

    // Get the map instance
    MapInstance* map = MapManager::Instance().GetInstanceById(player.mapInstanceId);
    if (map) {
        // Remove entity from map
        map->UnregisterPlayer(player.entityId);

        // Notify other players in this map
        WriteBuffer despawnPacket;
        despawnPacket.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ENTITY_DESPAWN));
        S_EntityDespawn despawn;
        despawn.id = player.entityId;
        despawn.Serialize(despawnPacket);

        for (const auto& [entityId, info] : map->GetAllPlayers()) {
            m_Network.Send(info.peerId, despawnPacket);
        }
    }

    // Clear known entities tracking for this player
    m_PlayerKnownEntities.erase(peerId);

    m_ConnectedPlayers.erase(it);
}

// ============================================================
// PACKET PROCESSING
// ============================================================

void WorldServer::ProcessPacket(uint32_t peerId, const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    ReadBuffer buf(data);
    auto packetType = static_cast<WorldPacketType>(buf.ReadU8());

    // Auth check for non-auth packets
    if (packetType != WorldPacketType::C_AUTH_TOKEN) {
        if (m_ConnectedPlayers.find(peerId) == m_ConnectedPlayers.end()) {
            return;  // Not authenticated
        }
    }

    switch (packetType) {
        case WorldPacketType::C_AUTH_TOKEN:
            HandleAuthToken(peerId, buf);
            break;
        case WorldPacketType::C_INPUT:
            HandleInput(peerId, buf);
            break;
        case WorldPacketType::C_CAST_ABILITY:
            HandleCastAbility(peerId, buf);
            break;
        case WorldPacketType::C_SELECT_TARGET:
            HandleSelectTarget(peerId, buf);
            break;
        case WorldPacketType::C_USE_PORTAL:
            HandleUsePortal(peerId, buf);
            break;
        case WorldPacketType::C_OPEN_LOOT:
            HandleOpenLoot(peerId, buf);
            break;
        case WorldPacketType::C_TAKE_LOOT_MONEY:
            HandleTakeLootMoney(peerId, buf);
            break;
        case WorldPacketType::C_EQUIP_ITEM:
            HandleEquipItem(peerId, buf);
            break;
        case WorldPacketType::C_UNEQUIP_ITEM:
            HandleUnequipItem(peerId, buf);
            break;
        case WorldPacketType::C_SWAP_INVENTORY:
            HandleSwapInventory(peerId, buf);
            break;
        case WorldPacketType::C_TAKE_LOOT_ITEM:
            HandleTakeLootItem(peerId, buf);
            break;
        default:
            std::cerr << "Unknown world packet type: " << static_cast<int>(packetType) << std::endl;
            break;
    }
}

void WorldServer::HandleAuthToken(uint32_t peerId, ReadBuffer& buf) {
    C_AuthToken request;
    request.Deserialize(buf);

    std::cout << "Auth token received from peer " << peerId << std::endl;

    // Load character data from database
    CharacterData charData = LoadCharacter(request.characterId);

    // Get the map instance for this character
    uint32_t mapTemplateId = charData.mapId;
    if (mapTemplateId == 0) mapTemplateId = 1;  // Default to starting zone

    MapInstance* map = MapManager::Instance().GetMapInstance(mapTemplateId);
    if (!map) {
        std::cerr << "Failed to get map instance for template " << mapTemplateId << std::endl;
        SendError(peerId, ErrorCode::UNKNOWN_ERROR);
        return;
    }

    // Spawn player at their saved position or map spawn point
    Vec2 spawnPos = (charData.positionX != 0.0f || charData.positionY != 0.0f)
        ? Vec2(charData.positionX, charData.positionY)
        : map->GetSpawnPoint();

    // Create player entity in the map
    Entity* player = map->CreatePlayer(
        request.characterId,
        charData.name,
        charData.characterClass,
        charData.level,
        spawnPos,
        charData.currentHealth,
        charData.currentMana
    );

    if (!player) {
        SendError(peerId, ErrorCode::UNKNOWN_ERROR);
        return;
    }

    // Register player with map
    map->RegisterPlayer(player->GetId(), peerId, request.characterId, charData.accountId);

    // Track connected player
    ConnectedPlayer connPlayer;
    connPlayer.peerId = peerId;
    connPlayer.characterId = request.characterId;
    connPlayer.accountId = charData.accountId;
    connPlayer.mapInstanceId = map->GetInstanceId();
    connPlayer.entityId = player->GetId();
    m_ConnectedPlayers[peerId] = connPlayer;

    // Load and apply cooldowns
    if (m_Database.IsConnected()) {
        auto cooldowns = m_Database.GetCooldowns(request.characterId);
        auto combat = player->GetCombat();
        if (combat) {
            for (const auto& cd : cooldowns) {
                combat->cooldowns[cd.abilityId] = cd.remaining;
            }
        }
    }

    // Set player's money from database
    if (player->GetWallet()) {
        player->GetWallet()->IsCopper = charData.money;
    }

    // Initialize inventory, equipment, and stats components
    player->AddInventoryComponent();
    player->AddEquipmentComponent();
    player->AddStatsComponent();

    // Load inventory and equipment from database
    LoadPlayerInventory(player, request.characterId);
    LoadPlayerEquipment(player, request.characterId);

    // Recalculate stats from equipped gear
    player->RecalculateStatsFromGear();

    // Send auth result
    WriteBuffer authResult;
    authResult.WriteU8(static_cast<uint8_t>(WorldPacketType::S_AUTH_RESULT));
    S_AuthResult result;
    result.success = true;
    result.errorCode = ErrorCode::NONE;
    result.Serialize(authResult);
    m_Network.Send(peerId, authResult);

    // Send enter world
    SendEnterWorld(peerId, player->GetId(), map);

    // Send zone data (portals)
    SendZoneData(peerId, map);

    // Send initial stats
    SendYourStats(peerId, player);

    // Send initial money
    if (player->GetWallet()) {
        SendMoneyUpdate(peerId, player->GetWallet()->IsCopper);
    }

    // Send inventory, equipment, and character stats
    SendInventoryData(peerId, player);
    SendEquipmentData(peerId, player);
    SendCharacterStats(peerId, player);

    // Send existing entities to new player and track them in known entities
    std::unordered_set<EntityId>& knownEntities = m_PlayerKnownEntities[peerId];
    knownEntities.clear();  // Start fresh
    for (const auto& state : map->GetWorldStateForPlayer(player->GetId())) {
        Entity* entity = map->GetEntity(state.id);
        if (entity && entity->GetId() != player->GetId()) {
            SendEntitySpawn(peerId, entity);
            knownEntities.insert(entity->GetId());  // Track as known
        }
    }

    // Broadcast new player spawn to others in same map
    WriteBuffer spawnPacket;
    spawnPacket.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ENTITY_SPAWN));
    S_EntitySpawn spawn;
    spawn.id = player->GetId();
    spawn.type = EntityType::PLAYER;
    spawn.name = player->GetName();
    spawn.characterClass = player->GetClass();
    spawn.position = player->GetMovement()->position;
    spawn.rotation = player->GetMovement()->rotation;
    spawn.health = player->GetHealth()->current;
    spawn.maxHealth = player->GetHealth()->max;
    spawn.level = player->GetLevel();
    spawn.Serialize(spawnPacket);

    for (const auto& [entityId, info] : map->GetAllPlayers()) {
        m_Network.Send(info.peerId, spawnPacket);
        // Track the new player in other players' known entities
        if (info.peerId != peerId) {
            m_PlayerKnownEntities[info.peerId].insert(player->GetId());
        }
    }
}

void WorldServer::HandleInput(uint32_t peerId, ReadBuffer& buf) {
    C_Input input;
    input.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    MapInstance* map = MapManager::Instance().GetInstanceById(it->second.mapInstanceId);
    if (map) {
        map->ProcessInput(it->second.entityId, input);
    }
}

void WorldServer::HandleCastAbility(uint32_t peerId, ReadBuffer& buf) {
    C_CastAbility request;
    request.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    MapInstance* map = MapManager::Instance().GetInstanceById(it->second.mapInstanceId);
    if (map) {
        map->ProcessAbility(it->second.entityId, request.targetId, request.abilityId);
    }
}

void WorldServer::HandleSelectTarget(uint32_t peerId, ReadBuffer& buf) {
    C_SelectTarget request;
    request.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    MapInstance* map = MapManager::Instance().GetInstanceById(it->second.mapInstanceId);
    if (map) {
        map->ProcessTargetSelection(it->second.entityId, request.targetId);
    }
}

// ============================================================
// PORTAL HANDLING
// ============================================================

void WorldServer::HandleUsePortal(uint32_t peerId, ReadBuffer& buf) {
    C_UsePortal request;
    request.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    const ConnectedPlayer& player = it->second;
    MapInstance* map = MapManager::Instance().GetInstanceById(player.mapInstanceId);
    if (!map) return;

    Entity* entity = map->GetEntity(player.entityId);
    if (!entity || !entity->GetMovement()) return;

    // Find the portal by ID
    const MapTemplate* tmpl = map->GetTemplate();
    const Portal* portal = nullptr;
    for (const auto& p : tmpl->portals) {
        if (p.id == request.portalId) {
            portal = &p;
            break;
        }
    }

    if (!portal) {
        std::cout << "[WorldServer] Portal " << request.portalId << " not found" << std::endl;
        return;
    }

    // Check if player is close enough to the portal (within 5 units)
    Vec2 playerPos = entity->GetMovement()->position;
    float distance = Vec2::Distance(playerPos, portal->position);
    if (distance > 8.0f) {
        std::cout << "[WorldServer] Player too far from portal (distance: " << distance << ")" << std::endl;
        return;
    }

    // Transfer the player
    TransferPlayer(peerId, portal, map);
}

void WorldServer::TransferPlayer(uint32_t peerId, const Portal* portal, MapInstance* fromMap) {
    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    ConnectedPlayer& player = it->second;
    EntityId entityId = player.entityId;

    // Clear known entities when changing maps - player will get fresh spawns
    m_PlayerKnownEntities[peerId].clear();

    // Get destination map first
    MapInstance* destMap = MapManager::Instance().GetMapInstance(portal->destMapId);
    if (!destMap) {
        std::cerr << "Failed to get destination map " << portal->destMapId << std::endl;
        return;
    }

    // Notify players in old map about despawn
    WriteBuffer despawnPacket;
    despawnPacket.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ENTITY_DESPAWN));
    S_EntityDespawn despawn;
    despawn.id = entityId;
    despawn.Serialize(despawnPacket);

    for (const auto& [id, info] : fromMap->GetAllPlayers()) {
        if (info.peerId != peerId) {
            m_Network.Send(info.peerId, despawnPacket);
        }
    }

    // === AzerothCore style: MOVE entity instead of recreate ===
    // Release entity from old map (transfers ownership to us)
    // Note: Don't call UnregisterPlayer here - it would delete the entity!
    // ReleaseEntity will handle removing from m_Entities
    std::unique_ptr<Entity> entity = fromMap->ReleaseEntity(entityId);
    if (!entity) {
        std::cerr << "Failed to release entity " << entityId << " from old map" << std::endl;
        return;
    }

    // Update entity position for new map
    if (entity->GetMovement()) {
        entity->GetMovement()->position = portal->destPosition;
    }

    // Adopt entity into new map (transfers ownership to new map)
    // EntityId stays the SAME - no recreation needed!
    Entity* playerEntity = destMap->AdoptEntity(std::move(entity));
    if (!playerEntity) {
        std::cerr << "Failed to adopt entity into destination map" << std::endl;
        return;
    }

    // Register player in new map
    destMap->RegisterPlayer(entityId, peerId, player.characterId, player.accountId);

    // Update connected player tracking (entityId stays the same!)
    player.mapInstanceId = destMap->GetInstanceId();
    // player.entityId stays the same - no change needed!

    // Send map change to player
    SendMapChange(peerId, entityId, destMap->GetName(), portal->destPosition, destMap);

    // Send player stats (level, XP, health, mana) after map change
    SendYourStats(peerId, playerEntity);

    // Send entities in new map to player and track in known entities
    std::unordered_set<EntityId>& knownEntities = m_PlayerKnownEntities[peerId];
    for (const auto& state : destMap->GetWorldStateForPlayer(entityId)) {
        Entity* ent = destMap->GetEntity(state.id);
        if (ent && ent->GetId() != entityId) {
            SendEntitySpawn(peerId, ent);
            knownEntities.insert(ent->GetId());  // Track as known
        }
    }

    // Notify players in new map about spawn
    WriteBuffer spawnPacket;
    spawnPacket.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ENTITY_SPAWN));
    S_EntitySpawn spawn;
    spawn.id = entityId;
    spawn.type = EntityType::PLAYER;
    spawn.name = playerEntity->GetName();
    spawn.characterClass = playerEntity->GetClass();
    spawn.position = playerEntity->GetMovement()->position;
    spawn.rotation = playerEntity->GetMovement()->rotation;
    spawn.health = playerEntity->GetHealth()->current;
    spawn.maxHealth = playerEntity->GetHealth()->max;
    spawn.level = playerEntity->GetLevel();
    spawn.Serialize(spawnPacket);

    for (const auto& [id, info] : destMap->GetAllPlayers()) {
        m_Network.Send(info.peerId, spawnPacket);
        // Track the transferred player in other players' known entities
        if (info.peerId != peerId) {
            m_PlayerKnownEntities[info.peerId].insert(entityId);
        }
    }

    std::cout << "Player " << playerEntity->GetName() << " transferred to " << destMap->GetName()
              << " (EntityId " << entityId << " preserved)" << std::endl;
}

// ============================================================
// PERSISTENCE
// ============================================================

void WorldServer::SavePlayer(const ConnectedPlayer& player) {
    if (!m_Database.IsConnected()) return;

    MapInstance* map = MapManager::Instance().GetInstanceById(player.mapInstanceId);
    if (!map) return;

    Entity* entity = map->GetEntity(player.entityId);
    if (!entity) return;

    CharacterData data;
    data.id = player.characterId;
    data.accountId = player.accountId;
    data.name = entity->GetName();
    data.characterClass = entity->GetClass();
    data.level = entity->GetLevel();
    data.experience = 0;  // TODO: track experience
    data.money = entity->GetWallet() ? entity->GetWallet()->IsCopper : 0;
    data.mapId = map->GetTemplateId();
    data.positionX = entity->GetMovement()->position.x;
    data.positionY = entity->GetMovement()->position.y;
    data.maxHealth = entity->GetHealth()->max;
    data.maxMana = entity->GetMana() ? entity->GetMana()->max : 0;
    data.currentHealth = entity->GetHealth()->current;
    data.currentMana = entity->GetMana() ? entity->GetMana()->current : 0;

    m_Database.SaveCharacter(data);

    // Save cooldowns
    auto combat = entity->GetCombat();
    if (combat) {
        std::vector<CooldownData> cooldowns;
        for (const auto& [abilityId, remaining] : combat->cooldowns) {
            if (remaining > 0.0f) {
                cooldowns.push_back({abilityId, remaining});
            }
        }
        m_Database.SaveCooldowns(player.characterId, cooldowns);
    }

    // Save inventory and equipment
    SavePlayerInventory(entity, player.characterId);
    SavePlayerEquipment(entity, player.characterId);
}

CharacterData WorldServer::LoadCharacter(CharacterId characterId) {
    CharacterData data;
    data.id = characterId;
    data.accountId = 0;
    data.name = "Player" + std::to_string(characterId);
    data.characterClass = (characterId % 2 == 0) ? CharacterClass::WARRIOR : CharacterClass::WITCH;
    data.level = 1;
    data.experience = 0;
    data.money = 0;
    data.mapId = 1;
    data.positionX = 0.0f;
    data.positionY = 0.0f;

    if (data.characterClass == CharacterClass::WARRIOR) {
        data.maxHealth = 150;
        data.maxMana = 50;
    } else {
        data.maxHealth = 80;
        data.maxMana = 120;
    }
    data.currentHealth = data.maxHealth;
    data.currentMana = data.maxMana;

    if (m_Database.IsConnected()) {
        auto dbData = m_Database.GetCharacterById(characterId);
        if (dbData) {
            data = *dbData;
        }
    }

    return data;
}

// ============================================================
// SEND METHODS
// ============================================================

void WorldServer::SendEnterWorld(uint32_t peerId, EntityId entityId, MapInstance* map) {
    Entity* entity = map->GetEntity(entityId);
    if (!entity || !entity->GetMovement()) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ENTER_WORLD));
    S_EnterWorld enter;
    enter.yourEntityId = entityId;
    enter.spawnPosition = entity->GetMovement()->position;
    enter.zoneName = map->GetName();
    enter.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendMapChange(uint32_t peerId, EntityId newEntityId, const std::string& mapName, Vec2 position, MapInstance* map) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ENTER_WORLD));
    S_EnterWorld enter;
    enter.yourEntityId = newEntityId;
    enter.spawnPosition = position;
    enter.zoneName = mapName;
    enter.Serialize(packet);
    m_Network.Send(peerId, packet);

    // Send zone data (portals) for new map
    SendZoneData(peerId, map);
}

void WorldServer::SendZoneData(uint32_t peerId, MapInstance* map) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ZONE_DATA));

    S_ZoneData zoneData;
    const MapTemplate* tmpl = map->GetTemplate();
    for (const auto& portal : tmpl->portals) {
        PortalInfo info;
        info.id = portal.id;
        info.position = portal.position;
        info.size = portal.size;
        // Get destination map name
        const MapTemplate* destTmpl = MapManager::Instance().GetTemplate(portal.destMapId);
        info.destMapName = destTmpl ? destTmpl->name : "Unknown";
        zoneData.portals.push_back(info);
    }

    zoneData.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendWorldState(MapInstance* map) {
    Grid& grid = map->GetGrid();

    // 1. Send each player their authoritative position (client prediction reconciliation)
    for (const auto& [entityId, info] : map->GetAllPlayers()) {
        Entity* player = map->GetEntity(entityId);
        if (!player || !player->GetMovement()) continue;

        WriteBuffer posPacket;
        posPacket.WriteU8(static_cast<uint8_t>(WorldPacketType::S_PLAYER_POSITION));
        S_PlayerPosition pos;
        pos.serverTick = m_ServerTick;
        pos.lastInputSeq = info.lastInputSeq;
        pos.position = player->GetMovement()->position;
        pos.Serialize(posPacket);
        m_Network.Send(info.peerId, posPacket);

        // Send player's own stats
        SendYourStats(info.peerId, player);
    }

    // 2. Entity-centric updates: iterate ONLY dirty entities (AzerothCore-style O(d) optimization)
    const auto& dirtyEntities = grid.GetDirtyEntities();
    for (EntityId entityId : dirtyEntities) {
        Entity* entity = map->GetEntity(entityId);
        if (!entity) continue;  // Entity may have been despawned

        DirtyFlags flags = grid.GetDirtyFlags(entityId);

        // Skip if only spawn/despawn flags (handled by SendSpawnsAndDespawns)
        if (!flags.position && !flags.health && !flags.mana &&
            !flags.target && !flags.casting && !flags.moveState) {
            continue;
        }

        // Build update mask
        uint8_t updateMask = 0;
        if (flags.position) updateMask |= UPDATE_POSITION;
        if (flags.health) updateMask |= UPDATE_HEALTH;
        if (flags.mana) updateMask |= UPDATE_MANA;
        if (flags.target) updateMask |= UPDATE_TARGET;
        if (flags.casting) updateMask |= UPDATE_CASTING;
        if (flags.moveState) updateMask |= UPDATE_MOVE_STATE;

        // Build update packet
        S_EntityUpdate update;
        update.id = entityId;
        update.updateMask = updateMask;

        auto movement = entity->GetMovement();
        auto health = entity->GetHealth();
        auto mana = entity->GetMana();
        auto combat = entity->GetCombat();

        if (movement) {
            update.position = movement->position;
            update.rotation = movement->rotation;
            update.moveState = movement->moveState;
        }
        if (health) {
            update.health = health->current;
            update.maxHealth = health->max;
        }
        if (mana) {
            update.mana = mana->current;
            update.maxMana = mana->max;
        }
        if (combat) {
            update.targetId = combat->targetId;
            update.isCasting = combat->IsCasting();
            update.castingAbilityId = combat->currentCast;
            update.castProgress = combat->castDuration > 0 ?
                (combat->castTimer / combat->castDuration) : 0.0f;
        }

        WriteBuffer updatePacket;
        updatePacket.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ENTITY_UPDATE));
        update.Serialize(updatePacket);

        // Send to all players who can see this entity
        Vec2 entityPos = movement ? movement->position : Vec2(0, 0);
        grid.ForEachPlayerNear(entityPos, [&, entityId](EntityId playerId) {
            // Don't send entity's own update via this path (they get S_PLAYER_POSITION)
            if (playerId == entityId) return;

            const PlayerInfo* playerInfo = map->GetPlayerInfo(playerId);
            if (playerInfo) {
                // Only send if player knows about this entity
                auto& knownEntities = m_PlayerKnownEntities[playerInfo->peerId];
                if (knownEntities.find(entityId) != knownEntities.end()) {
                    m_Network.Send(playerInfo->peerId, updatePacket);
                }
            }
        });
    }
}

void WorldServer::SendEvents(MapInstance* map) {
    auto& events = map->GetPendingEvents();
    if (events.empty()) return;

    for (const auto& event : events) {
        WriteBuffer packet;
        packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_EVENT));
        S_Event sEvent;
        sEvent.type = event.type;
        sEvent.sourceId = event.sourceId;
        sEvent.targetId = event.targetId;
        sEvent.abilityId = event.abilityId;
        sEvent.value = event.value;
        sEvent.position = event.position;
        sEvent.Serialize(packet);

        // Send only to players near the event (AzerothCore-style)
        map->GetGrid().ForEachPlayerNear(event.position, [&](EntityId playerId) {
            const PlayerInfo* info = map->GetPlayerInfo(playerId);
            if (info) {
                m_Network.Send(info->peerId, packet);
            }
        });
    }
}

void WorldServer::SendAuraUpdates(MapInstance* map) {
    auto& updates = map->GetPendingAuraUpdates();
    if (updates.empty()) return;

    for (const auto& update : updates) {
        // Create packet
        WriteBuffer packet;
        packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_AURA_UPDATE));

        S_AuraUpdate auraPacket;
        auraPacket.targetId = update.targetId;
        auraPacket.updateType = update.updateType;
        auraPacket.aura.auraId = update.aura.id;
        auraPacket.aura.sourceAbility = update.aura.sourceAbility;
        auraPacket.aura.auraType = static_cast<uint8_t>(update.aura.type);
        auraPacket.aura.value = update.aura.value;
        auraPacket.aura.duration = update.aura.duration;
        auraPacket.aura.maxDuration = update.aura.maxDuration;
        auraPacket.aura.casterId = update.aura.casterId;
        auraPacket.Serialize(packet);

        // Use grid to find nearby players (AzerothCore-style visitor pattern)
        // Only visits cells within VIEW_DISTANCE, not all players on the map
        std::unordered_set<uint32_t> sentToPeers;
        map->GetGrid().ForEachPlayerNear(update.targetPosition, [&](EntityId playerId) {
            const PlayerInfo* playerInfo = map->GetPlayerInfo(playerId);
            if (playerInfo) {
                m_Network.Send(playerInfo->peerId, packet);
                sentToPeers.insert(playerInfo->peerId);
            }
        });

        // Always send to target if they're a player (even if grid didn't include them)
        const PlayerInfo* targetInfo = map->GetPlayerInfo(update.targetId);
        if (targetInfo && sentToPeers.find(targetInfo->peerId) == sentToPeers.end()) {
            m_Network.Send(targetInfo->peerId, packet);
        }
    }

    map->ClearAuraUpdates();
}

void WorldServer::SendEntitySpawn(uint32_t peerId, Entity* entity) {
    if (!entity) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ENTITY_SPAWN));

    S_EntitySpawn spawn;
    spawn.id = entity->GetId();
    spawn.type = entity->GetType();
    spawn.name = entity->GetName();
    spawn.characterClass = entity->GetClass();

    auto movement = entity->GetMovement();
    if (movement) {
        spawn.position = movement->position;
        spawn.rotation = movement->rotation;
    }

    auto health = entity->GetHealth();
    if (health) {
        spawn.health = health->current;
        spawn.maxHealth = health->max;
    }

    spawn.level = entity->GetLevel();
    spawn.Serialize(packet);

    m_Network.Send(peerId, packet);
}

void WorldServer::SendEntityDespawn(uint32_t peerId, EntityId entityId) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_ENTITY_DESPAWN));
    S_EntityDespawn despawn;
    despawn.id = entityId;
    despawn.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendSpawnsAndDespawns(MapInstance* map) {
    Grid& grid = map->GetGrid();

    // Per-player visibility tracking (AzerothCore-style)
    // Instead of relying on dirty flags that get cleared each tick,
    // we track what each player knows about and compare with what they can see
    for (const auto& [playerEntityId, info] : map->GetAllPlayers()) {
        Entity* playerEntity = map->GetEntity(playerEntityId);
        if (!playerEntity || !playerEntity->GetMovement()) continue;

        Vec2 playerPos = playerEntity->GetMovement()->position;

        // Get entities currently visible to this player
        std::vector<EntityId> visibleEntities = grid.GetVisibleEntities(playerPos);

        // Get or create the player's known entities set
        std::unordered_set<EntityId>& knownEntities = m_PlayerKnownEntities[info.peerId];

        // Build set of currently visible entity IDs for quick lookup
        std::unordered_set<EntityId> visibleSet;
        for (EntityId id : visibleEntities) {
            if (id != playerEntityId) {  // Skip self
                visibleSet.insert(id);
            }
        }

        // Find entities to spawn: visible but not known
        for (EntityId id : visibleSet) {
            if (knownEntities.find(id) == knownEntities.end()) {
                // Player doesn't know about this entity - send spawn
                Entity* entity = map->GetEntity(id);
                if (entity) {
                    SendEntitySpawn(info.peerId, entity);
                    knownEntities.insert(id);
                }
            }
        }

        // Find entities to despawn: known but no longer visible (or entity removed)
        std::vector<EntityId> toRemove;
        for (EntityId id : knownEntities) {
            if (visibleSet.find(id) == visibleSet.end()) {
                // Entity is no longer visible to player - send despawn
                SendEntityDespawn(info.peerId, id);
                toRemove.push_back(id);
            }
        }

        // Remove despawned entities from known set
        for (EntityId id : toRemove) {
            knownEntities.erase(id);
        }
    }

    // Clear dirty flags (still used for incremental state updates)
    grid.ClearAllDirtyFlags();
}

void WorldServer::SendYourStats(uint32_t peerId, Entity* player) {
    if (!player) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_YOUR_STATS));

    S_YourStats stats;
    auto health = player->GetHealth();
    auto mana = player->GetMana();

    stats.health = health ? health->current : 0;
    stats.maxHealth = health ? health->max : 0;
    stats.mana = mana ? mana->current : 0;
    stats.maxMana = mana ? mana->max : 0;
    stats.level = player->GetLevel();

    auto xp = player->GetExperience();
    stats.experience = xp ? xp->current : 0;
    stats.experienceToNext = player->GetXPForNextLevel();

    stats.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendError(uint32_t peerId, ErrorCode code) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_AUTH_RESULT));
    S_AuthResult result;
    result.success = false;
    result.errorCode = code;
    result.Serialize(packet);
    m_Network.Send(peerId, packet);
}

// ============================================================
// LOOT HANDLING
// ============================================================

void WorldServer::HandleOpenLoot(uint32_t peerId, ReadBuffer& buf) {
    C_OpenLoot request;
    request.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    const ConnectedPlayer& player = it->second;
    MapInstance* map = MapManager::Instance().GetInstanceById(player.mapInstanceId);
    if (!map) return;

    Entity* playerEntity = map->GetEntity(player.entityId);
    Entity* corpse = map->GetEntity(request.targetId);

    if (!playerEntity || !corpse) return;

    // Check if corpse is dead
    auto corpseHealth = corpse->GetHealth();
    if (!corpseHealth || !corpseHealth->IsDead()) return;

    // Check distance (must be within 3 units)
    auto playerMove = playerEntity->GetMovement();
    auto corpseMove = corpse->GetMovement();
    if (!playerMove || !corpseMove) return;

    float dist = Vec2::Distance(playerMove->position, corpseMove->position);
    if (dist > 3.0f) {
        std::cout << "[Loot] Player too far from corpse (dist: " << dist << ")" << std::endl;
        return;
    }

    // Check if there's loot
    LootData* loot = map->GetLoot(request.targetId);
    if (!loot) {
        std::cout << "[Loot] No loot on corpse " << request.targetId << std::endl;
        return;
    }

    // Check loot rights
    if (loot->killerEntityId != player.entityId) {
        std::cout << "[Loot] Player doesn't have loot rights" << std::endl;
        return;
    }

    // Send loot contents (money + items)
    SendLootContents(peerId, request.targetId, loot);
}

void WorldServer::HandleTakeLootMoney(uint32_t peerId, ReadBuffer& buf) {
    C_TakeLootMoney request;
    request.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    const ConnectedPlayer& player = it->second;
    MapInstance* map = MapManager::Instance().GetInstanceById(player.mapInstanceId);
    if (!map) return;

    Entity* playerEntity = map->GetEntity(player.entityId);
    if (!playerEntity) return;

    // Get loot data
    LootData* loot = map->GetLoot(request.corpseId);
    if (!loot || loot->moneyLooted) return;

    // Check loot rights
    if (loot->killerEntityId != player.entityId) return;

    uint32_t moneyTaken = loot->money;

    // Take the money
    if (map->TakeLootMoney(request.corpseId, player.entityId)) {
        // Send confirmation (255 = money, not item slot)
        SendLootTaken(peerId, request.corpseId, moneyTaken, 255, loot->IsEmpty());

        // Send updated wallet
        auto wallet = playerEntity->GetWallet();
        if (wallet) {
            SendMoneyUpdate(peerId, wallet->IsCopper);
        }
    }
}

void WorldServer::HandleTakeLootItem(uint32_t peerId, ReadBuffer& buf) {
    C_TakeLootItem request;
    request.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    const ConnectedPlayer& player = it->second;
    MapInstance* map = MapManager::Instance().GetInstanceById(player.mapInstanceId);
    if (!map) return;

    Entity* playerEntity = map->GetEntity(player.entityId);
    if (!playerEntity) return;

    // Get loot data
    LootData* loot = map->GetLoot(request.corpseId);
    if (!loot) return;

    // Check loot rights
    if (loot->killerEntityId != player.entityId) return;

    // Try to take the item
    ItemInstance lootedItem;
    uint8_t inventorySlot = 255;
    if (map->TakeLootItem(request.corpseId, player.entityId, request.lootSlot, lootedItem, &inventorySlot)) {
        // Send confirmation
        SendLootTaken(peerId, request.corpseId, 0, request.lootSlot, loot->IsEmpty());

        // Send the inventory update for the exact slot where item was added
        if (inventorySlot < INVENTORY_SIZE) {
            SendInventoryUpdate(peerId, inventorySlot, &lootedItem);
        }
    }
}

void WorldServer::SendLootContents(uint32_t peerId, EntityId corpseId, const LootData* loot) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_LOOT_CONTENTS));
    S_LootContents contents;
    contents.corpseId = corpseId;
    contents.money = loot->moneyLooted ? 0 : loot->money;

    // Add unlooted items with their server-assigned slot IDs
    for (const auto& item : loot->items) {
        if (!item.looted) {
            LootItemInfo info;
            info.slotId = item.slotId;  // Server-assigned stable slot ID
            info.templateId = item.templateId;
            info.stackCount = item.stackCount;
            contents.items.push_back(info);
        }
    }

    contents.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendLootTaken(uint32_t peerId, EntityId corpseId, uint32_t money, uint8_t itemSlot, bool isEmpty) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_LOOT_TAKEN));
    S_LootTaken taken;
    taken.corpseId = corpseId;
    taken.moneyTaken = money;
    taken.itemSlotTaken = itemSlot;
    taken.lootEmpty = isEmpty;
    taken.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendMoneyUpdate(uint32_t peerId, uint32_t totalCopper) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_MONEY_UPDATE));
    S_MoneyUpdate update;
    update.totalCopper = totalCopper;
    update.Serialize(packet);
    m_Network.Send(peerId, packet);
}

// ============================================================
// INVENTORY & EQUIPMENT HANDLERS
// ============================================================

void WorldServer::HandleEquipItem(uint32_t peerId, ReadBuffer& buf) {
    C_EquipItem request;
    request.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    const ConnectedPlayer& player = it->second;
    MapInstance* map = MapManager::Instance().GetInstanceById(player.mapInstanceId);
    if (!map) return;

    Entity* entity = map->GetEntity(player.entityId);
    if (!entity || !entity->GetInventory() || !entity->GetEquipment()) return;

    // Try to equip the item
    if (entity->EquipItem(request.inventorySlot, request.equipSlot)) {
        // Send updated inventory slot
        SendInventoryUpdate(peerId, request.inventorySlot, entity->GetInventory()->GetItem(request.inventorySlot));

        // Send updated equipment slot
        SendEquipmentUpdate(peerId, request.equipSlot, entity->GetEquipment()->GetEquipped(request.equipSlot));

        // Send updated character stats
        SendCharacterStats(peerId, entity);

        // Send updated health/mana (may have changed due to stamina/intellect)
        SendYourStats(peerId, entity);

        std::cout << "[Inventory] Player equipped item from slot " << (int)request.inventorySlot
                  << " to equipment slot " << (int)request.equipSlot << std::endl;
    }
}

void WorldServer::HandleUnequipItem(uint32_t peerId, ReadBuffer& buf) {
    C_UnequipItem request;
    request.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    const ConnectedPlayer& player = it->second;
    MapInstance* map = MapManager::Instance().GetInstanceById(player.mapInstanceId);
    if (!map) return;

    Entity* entity = map->GetEntity(player.entityId);
    if (!entity || !entity->GetInventory() || !entity->GetEquipment()) return;

    uint8_t inventorySlot = 0;
    if (entity->UnequipItem(request.equipSlot, &inventorySlot)) {
        // Send updated equipment slot (now empty)
        SendEquipmentUpdate(peerId, request.equipSlot, nullptr);

        // Send updated inventory slot
        SendInventoryUpdate(peerId, inventorySlot, entity->GetInventory()->GetItem(inventorySlot));

        // Send updated character stats
        SendCharacterStats(peerId, entity);

        // Send updated health/mana
        SendYourStats(peerId, entity);

        std::cout << "[Inventory] Player unequipped item from slot " << (int)request.equipSlot
                  << " to inventory slot " << (int)inventorySlot << std::endl;
    }
}

void WorldServer::HandleSwapInventory(uint32_t peerId, ReadBuffer& buf) {
    C_SwapInventory request;
    request.Deserialize(buf);

    auto it = m_ConnectedPlayers.find(peerId);
    if (it == m_ConnectedPlayers.end()) return;

    const ConnectedPlayer& player = it->second;
    MapInstance* map = MapManager::Instance().GetInstanceById(player.mapInstanceId);
    if (!map) return;

    Entity* entity = map->GetEntity(player.entityId);
    if (!entity || !entity->GetInventory()) return;

    if (entity->GetInventory()->SwapSlots(request.slotA, request.slotB)) {
        // Send both updated slots
        SendInventoryUpdate(peerId, request.slotA, entity->GetInventory()->GetItem(request.slotA));
        SendInventoryUpdate(peerId, request.slotB, entity->GetInventory()->GetItem(request.slotB));
    }
}

// ============================================================
// INVENTORY & EQUIPMENT SEND METHODS
// ============================================================

void WorldServer::SendInventoryData(uint32_t peerId, Entity* player) {
    if (!player || !player->GetInventory()) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_INVENTORY_DATA));

    S_InventoryData data;
    auto inventory = player->GetInventory();
    for (uint8_t i = 0; i < INVENTORY_SIZE; i++) {
        const ItemInstance* item = inventory->GetItem(i);
        if (item) {
            data.slots[i].instanceId = item->instanceId;
            data.slots[i].templateId = item->templateId;
            data.slots[i].stackCount = item->stackCount;
        }
    }
    data.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendEquipmentData(uint32_t peerId, Entity* player) {
    if (!player || !player->GetEquipment()) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_EQUIPMENT_DATA));

    S_EquipmentData data;
    auto equipment = player->GetEquipment();
    for (uint8_t i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
        const ItemInstance* item = equipment->GetEquipped(static_cast<EquipmentSlot>(i));
        if (item) {
            data.slots[i].instanceId = item->instanceId;
            data.slots[i].templateId = item->templateId;
            data.slots[i].stackCount = item->stackCount;
        }
    }
    data.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendInventoryUpdate(uint32_t peerId, uint8_t slot, const ItemInstance* item) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_INVENTORY_UPDATE));

    S_InventoryUpdate update;
    update.slot = slot;
    if (item && item->IsValid()) {
        update.item.instanceId = item->instanceId;
        update.item.templateId = item->templateId;
        update.item.stackCount = item->stackCount;
    }
    update.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendEquipmentUpdate(uint32_t peerId, EquipmentSlot slot, const ItemInstance* item) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_EQUIPMENT_UPDATE));

    S_EquipmentUpdate update;
    update.slot = slot;
    if (item && item->IsValid()) {
        update.item.instanceId = item->instanceId;
        update.item.templateId = item->templateId;
        update.item.stackCount = item->stackCount;
    }
    update.Serialize(packet);
    m_Network.Send(peerId, packet);
}

void WorldServer::SendCharacterStats(uint32_t peerId, Entity* player) {
    if (!player || !player->GetStats()) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::S_CHARACTER_STATS));

    S_CharacterStats stats;
    auto playerStats = player->GetStats();

    stats.strength = playerStats->GetStat(StatType::STRENGTH);
    stats.agility = playerStats->GetStat(StatType::AGILITY);
    stats.stamina = playerStats->GetStat(StatType::STAMINA);
    stats.intellect = playerStats->GetStat(StatType::INTELLECT);
    stats.spirit = playerStats->GetStat(StatType::SPIRIT);

    stats.attackPower = playerStats->GetAttackPower();
    stats.spellPower = playerStats->GetSpellPower();
    stats.armor = playerStats->totalArmor;
    stats.meleeDamageMin = playerStats->GetMeleeDamageMin();
    stats.meleeDamageMax = playerStats->GetMeleeDamageMax();
    stats.weaponSpeed = playerStats->weaponSpeed;

    stats.Serialize(packet);
    m_Network.Send(peerId, packet);
}

// ============================================================
// INVENTORY & EQUIPMENT PERSISTENCE
// ============================================================

void WorldServer::LoadPlayerInventory(Entity* player, CharacterId characterId) {
    if (!player || !player->GetInventory() || !m_Database.IsConnected()) return;

    auto inventory = player->GetInventory();
    auto items = m_Database.GetInventory(characterId);

    for (const auto& itemData : items) {
        if (itemData.slot >= INVENTORY_SIZE) continue;

        ItemInstance item;
        item.instanceId = itemData.instanceId;
        item.templateId = itemData.templateId;
        item.stackCount = itemData.stackCount;

        inventory->slots[itemData.slot].item = item;
    }
    std::cout << "[Inventory] Loaded " << items.size() << " items for character " << characterId << std::endl;
}

void WorldServer::LoadPlayerEquipment(Entity* player, CharacterId characterId) {
    if (!player || !player->GetEquipment() || !m_Database.IsConnected()) return;

    auto equipment = player->GetEquipment();
    
    auto items = m_Database.GetEquipment(characterId);
    for (const auto& itemData : items) {
        if (itemData.slot >= EQUIPMENT_SLOT_COUNT) continue;

        ItemInstance item;
        item.instanceId = itemData.instanceId;
        item.templateId = itemData.templateId;
        item.stackCount = 1;  // Equipment doesn't stack

        equipment->slots[itemData.slot] = item;
    }
    std::cout << "[Equipment] Loaded " << items.size() << " items for character " << characterId << std::endl;
}

void WorldServer::SavePlayerInventory(Entity* player, CharacterId characterId) {
    if (!player || !player->GetInventory() || !m_Database.IsConnected()) return;

    auto inventory = player->GetInventory();
    std::vector<Database::InventoryItemData> items;

    for (uint8_t i = 0; i < INVENTORY_SIZE; i++) {
        const ItemInstance* item = inventory->GetItem(i);
        if (item && item->IsValid()) {
            Database::InventoryItemData data;
            data.instanceId = item->instanceId;
            data.templateId = item->templateId;
            data.slot = i;
            data.stackCount = item->stackCount;
            items.push_back(data);
        }
    }

    m_Database.SaveInventory(characterId, items);
}

void WorldServer::SavePlayerEquipment(Entity* player, CharacterId characterId) {
    if (!player || !player->GetEquipment() || !m_Database.IsConnected()) return;

    auto equipment = player->GetEquipment();
    std::vector<Database::EquipmentItemData> items;

    for (uint8_t i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
        const ItemInstance* item = equipment->GetEquipped(static_cast<EquipmentSlot>(i));
        if (item && item->IsValid()) {
            Database::EquipmentItemData data;
            data.instanceId = item->instanceId;
            data.templateId = item->templateId;
            data.slot = i;
            items.push_back(data);
        }
    }

    m_Database.SaveEquipment(characterId, items);
}

} // namespace MMO
