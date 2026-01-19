#include "GameClient.h"
#include <iostream>
#include <chrono>
#include <algorithm>

namespace MMO {

GameClient::GameClient()
    : m_State(ClientState::DISCONNECTED)
    , m_AccountId(0)
    , m_SelectedCharacterId(0)
    , m_WorldPort(0)
    , m_TimeSinceLastInput(0.0f) {
    m_LocalPlayer.entityId = INVALID_ENTITY_ID;
    m_LocalPlayer.inputSequence = 0;
}

GameClient::~GameClient() {
    Disconnect();
}

// ============================================================
// CONNECTION
// ============================================================

bool GameClient::ConnectToLoginServer(const std::string& host, uint16_t port) {
    m_State = ClientState::CONNECTING_LOGIN;
    if (m_LoginConnection.Connect(host, port)) {
        m_State = ClientState::LOGIN_SCREEN;
        return true;
    }
    m_State = ClientState::DISCONNECTED;
    m_LastError = "Failed to connect to login server";
    return false;
}

bool GameClient::ConnectToWorldServer(const std::string& host, uint16_t port) {
    m_State = ClientState::CONNECTING_WORLD;
    if (m_WorldConnection.Connect(host, port)) {
        // Send auth token
        WriteBuffer packet;
        packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_AUTH_TOKEN));
        C_AuthToken auth;
        auth.token = m_WorldAuthToken;
        auth.characterId = m_SelectedCharacterId;
        auth.Serialize(packet);
        m_WorldConnection.Send(packet);
        return true;
    }
    m_State = ClientState::CHARACTER_SELECT;
    m_LastError = "Failed to connect to world server";
    return false;
}

void GameClient::Disconnect() {
    m_LoginConnection.Disconnect();
    m_WorldConnection.Disconnect();
    m_State = ClientState::DISCONNECTED;
    m_Entities.clear();
    m_CharacterList.clear();
    m_LocalPlayer.entityId = INVALID_ENTITY_ID;
}

// ============================================================
// LOGIN FLOW
// ============================================================

void GameClient::Register(const std::string& username, const std::string& password,
                          const std::string& email) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(LoginPacketType::C_REGISTER_REQUEST));
    C_RegisterRequest request;
    request.username = username;
    request.password = password;
    request.email = email;
    request.Serialize(packet);
    m_LoginConnection.Send(packet);
}

void GameClient::Login(const std::string& username, const std::string& password) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(LoginPacketType::C_LOGIN_REQUEST));
    C_LoginRequest request;
    request.username = username;
    request.password = password;
    request.clientVersion = 1;
    request.Serialize(packet);
    m_LoginConnection.Send(packet);
}

void GameClient::CreateCharacter(const std::string& name, CharacterClass charClass) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(LoginPacketType::C_CREATE_CHARACTER));
    C_CreateCharacter request;
    request.name = name;
    request.characterClass = charClass;
    request.Serialize(packet);
    m_LoginConnection.Send(packet);
}

void GameClient::DeleteCharacter(CharacterId id) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(LoginPacketType::C_DELETE_CHARACTER));
    C_DeleteCharacter request;
    request.characterId = id;
    request.Serialize(packet);
    m_LoginConnection.Send(packet);
}

void GameClient::SelectCharacter(CharacterId id) {
    m_SelectedCharacterId = id;
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(LoginPacketType::C_SELECT_CHARACTER));
    C_SelectCharacter request;
    request.characterId = id;
    request.Serialize(packet);
    m_LoginConnection.Send(packet);
}

// ============================================================
// GAME INPUT
// ============================================================

void GameClient::SendInput(int8_t moveX, int8_t moveY, float rotation) {
    if (m_State != ClientState::IN_GAME) return;

    WriteBuffer packet;
    C_Input input;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_INPUT));
    input.sequence = ++m_LocalPlayer.inputSequence;
    input.moveX = moveX;
    input.moveY = moveY;
    input.rotation = rotation;
    input.Serialize(packet);
    m_WorldConnection.Send(packet);

    // Client-side prediction
    if (moveX != 0 || moveY != 0) {
        Vec2 inputDir(static_cast<float>(moveX), static_cast<float>(moveY));
        Vec2 movement = inputDir.Normalized() * 5.0f * INPUT_SEND_RATE;  // speed * dt
        m_LocalPlayer.position += movement;

        // Store for reconciliation
        m_LocalPlayer.pendingInputs.push_back({input.sequence, m_LocalPlayer.position});
        if (m_LocalPlayer.pendingInputs.size() > 60) {
            m_LocalPlayer.pendingInputs.pop_front();
        }
    }
}

void GameClient::CastAbility(AbilityId abilityId) {
    if (m_State != ClientState::IN_GAME) return;

    AbilityData ability = AbilityData::GetAbilityData(abilityId);
    std::cout << "[Client] CastAbility: " << ability.name
              << " (ID: " << static_cast<int>(abilityId) << ")"
              << ", Target: " << m_LocalPlayer.targetId
              << ", Mana: " << m_LocalPlayer.mana << "/" << m_LocalPlayer.maxMana
              << ", Health: " << m_LocalPlayer.health << "/" << m_LocalPlayer.maxHealth
              << std::endl;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_CAST_ABILITY));
    C_CastAbility request;
    request.abilityId = abilityId;
    request.targetId = m_LocalPlayer.targetId;
    request.targetPosition = m_LocalPlayer.position;
    request.Serialize(packet);
    m_WorldConnection.Send(packet);
}

void GameClient::SelectTarget(EntityId targetId) {
    m_LocalPlayer.targetId = targetId;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_SELECT_TARGET));
    C_SelectTarget request;
    request.targetId = targetId;
    request.Serialize(packet);
    m_WorldConnection.Send(packet);
}

void GameClient::UsePortal(uint32_t portalId) {
    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_USE_PORTAL));
    C_UsePortal request;
    request.portalId = portalId;
    request.Serialize(packet);
    m_WorldConnection.Send(packet);
}

// ============================================================
// UPDATE
// ============================================================

void GameClient::Update(float dt) {
    // Poll login server
    if (m_LoginConnection.IsConnected()) {
        std::vector<NetworkEvent> events;
        m_LoginConnection.Poll(events, 0);
        for (const auto& event : events) {
            if (event.type == NetworkEventType::DATA_RECEIVED) {
                ProcessLoginPacket(event.data);
            } else if (event.type == NetworkEventType::DISCONNECTED) {
                if (m_State == ClientState::LOGIN_SCREEN ||
                    m_State == ClientState::CHARACTER_SELECT) {
                    m_State = ClientState::DISCONNECTED;
                    m_LastError = "Disconnected from login server";
                }
            }
        }
    }

    // Poll world server
    if (m_WorldConnection.IsConnected()) {
        std::vector<NetworkEvent> events;
        m_WorldConnection.Poll(events, 0);
        for (const auto& event : events) {
            if (event.type == NetworkEventType::DATA_RECEIVED) {
                ProcessWorldPacket(event.data);
            } else if (event.type == NetworkEventType::DISCONNECTED) {
                m_State = ClientState::CHARACTER_SELECT;
                m_LastError = "Disconnected from world server";
                m_Entities.clear();
            }
        }
    }

    // Update cooldowns
    for (auto& [id, cooldown] : m_LocalPlayer.cooldowns) {
        if (cooldown > 0.0f) {
            cooldown -= dt;
        }
    }

    // Update aura durations (client-side countdown)
    UpdateAuras(dt);

    // Interpolate remote entities
    InterpolateEntities(dt);

    // Update projectile positions (client-side interpolation)
    const float PROJECTILE_SPEED = 15.0f;
    for (auto& proj : m_Projectiles) {
        // Get target position
        Vec2 targetPos = proj.position;
        auto it = m_Entities.find(proj.targetId);
        if (it != m_Entities.end()) {
            targetPos = it->second.position;
        } else if (proj.targetId == m_LocalPlayer.entityId) {
            targetPos = m_LocalPlayer.position;
        }

        // Move towards target
        Vec2 direction = targetPos - proj.position;
        float distance = direction.Length();
        if (distance > 0.5f) {
            proj.position += direction.Normalized() * PROJECTILE_SPEED * dt;
        }
    }

    // Clean up old events
    float currentTime = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count() / 1000.0f
    );
    m_PendingEvents.erase(
        std::remove_if(m_PendingEvents.begin(), m_PendingEvents.end(),
            [currentTime](const GameEvent& e) { return currentTime - e.timestamp > 2.0f; }),
        m_PendingEvents.end()
    );
}

void GameClient::InterpolateEntities(float dt) {
    for (auto& [id, entity] : m_Entities) {
        if (entity.interpolationTime < 1.0f) {
            entity.interpolationTime += dt * 10.0f;  // Interpolate over ~100ms
            if (entity.interpolationTime > 1.0f) {
                entity.interpolationTime = 1.0f;
            }

            float t = entity.interpolationTime;
            entity.position = entity.previousPosition * (1.0f - t) + entity.targetPosition * t;
        }
    }
}

// ============================================================
// PACKET PROCESSING - LOGIN
// ============================================================

void GameClient::ProcessLoginPacket(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    ReadBuffer buf(data);
    auto packetType = static_cast<LoginPacketType>(buf.ReadU8());

    switch (packetType) {
        case LoginPacketType::S_LOGIN_RESPONSE:
            HandleLoginResponse(buf);
            break;
        case LoginPacketType::S_REGISTER_RESPONSE:
            HandleRegisterResponse(buf);
            break;
        case LoginPacketType::S_CHARACTER_LIST:
            HandleCharacterList(buf);
            break;
        case LoginPacketType::S_CHARACTER_CREATED:
            HandleCharacterCreated(buf);
            break;
        case LoginPacketType::S_WORLD_SERVER_INFO:
            HandleWorldServerInfo(buf);
            break;
        case LoginPacketType::S_ERROR:
            HandleError(buf);
            break;
        default:
            break;
    }
}

void GameClient::HandleLoginResponse(ReadBuffer& buf) {
    S_LoginResponse response;
    response.Deserialize(buf);

    if (response.success) {
        m_SessionToken = response.sessionToken;
        m_AccountId = response.accountId;
        m_State = ClientState::CHARACTER_SELECT;
        std::cout << "Login successful!" << std::endl;
    } else {
        m_LastError = "Login failed";
    }
}

void GameClient::HandleRegisterResponse(ReadBuffer& buf) {
    S_RegisterResponse response;
    response.Deserialize(buf);

    if (response.success) {
        std::cout << "Registration successful! You can now login." << std::endl;
    } else {
        m_LastError = response.message;
    }
}

void GameClient::HandleCharacterList(ReadBuffer& buf) {
    S_CharacterList list;
    list.Deserialize(buf);
    m_CharacterList = list.characters;
    std::cout << "Received " << m_CharacterList.size() << " characters" << std::endl;
}

void GameClient::HandleCharacterCreated(ReadBuffer& buf) {
    S_CharacterCreated response;
    response.Deserialize(buf);

    if (response.success) {
        std::cout << "Character created: " << response.character.name << std::endl;
    } else {
        m_LastError = "Failed to create character";
    }
}

void GameClient::HandleWorldServerInfo(ReadBuffer& buf) {
    S_WorldServerInfo info;
    info.Deserialize(buf);

    m_WorldHost = info.host;
    m_WorldPort = info.port;
    m_WorldAuthToken = info.authToken;
    m_SelectedCharacterId = info.characterId;

    // Disconnect from login server and connect to world
    m_LoginConnection.Disconnect();
    ConnectToWorldServer(m_WorldHost, m_WorldPort);
}

void GameClient::HandleError(ReadBuffer& buf) {
    S_Error error;
    error.Deserialize(buf);
    m_LastError = error.message;
    std::cerr << "Server error: " << error.message << std::endl;
}

// ============================================================
// PACKET PROCESSING - WORLD
// ============================================================

void GameClient::ProcessWorldPacket(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    ReadBuffer buf(data);
    auto packetType = static_cast<WorldPacketType>(buf.ReadU8());

    switch (packetType) {
        case WorldPacketType::S_AUTH_RESULT:
            HandleAuthResult(buf);
            break;
        case WorldPacketType::S_ENTER_WORLD:
            HandleEnterWorld(buf);
            break;
        case WorldPacketType::S_WORLD_STATE:
            HandleWorldState(buf);
            break;
        case WorldPacketType::S_ENTITY_SPAWN:
            HandleEntitySpawn(buf);
            break;
        case WorldPacketType::S_ENTITY_DESPAWN:
            HandleEntityDespawn(buf);
            break;
        case WorldPacketType::S_EVENT:
            HandleEvent(buf);
            break;
        case WorldPacketType::S_YOUR_STATS:
            HandleYourStats(buf);
            break;
        case WorldPacketType::S_ZONE_DATA:
            HandleZoneData(buf);
            break;
        case WorldPacketType::S_LOOT_CONTENTS:
            HandleLootContents(buf);
            break;
        case WorldPacketType::S_LOOT_TAKEN:
            HandleLootTaken(buf);
            break;
        case WorldPacketType::S_MONEY_UPDATE:
            HandleMoneyUpdate(buf);
            break;
        case WorldPacketType::S_INVENTORY_DATA:
            HandleInventoryData(buf);
            break;
        case WorldPacketType::S_EQUIPMENT_DATA:
            HandleEquipmentData(buf);
            break;
        case WorldPacketType::S_INVENTORY_UPDATE:
            HandleInventoryUpdate(buf);
            break;
        case WorldPacketType::S_EQUIPMENT_UPDATE:
            HandleEquipmentUpdate(buf);
            break;
        case WorldPacketType::S_CHARACTER_STATS:
            HandleCharacterStats(buf);
            break;
        case WorldPacketType::S_ENTITY_UPDATE:
            HandleEntityUpdate(buf);
            break;
        case WorldPacketType::S_PLAYER_POSITION:
            HandlePlayerPosition(buf);
            break;
        case WorldPacketType::S_AURA_UPDATE:
            HandleAuraUpdate(buf);
            break;
        case WorldPacketType::S_AURA_UPDATE_ALL:
            HandleAuraUpdateAll(buf);
            break;
        default:
            break;
    }
}

void GameClient::HandleAuthResult(ReadBuffer& buf) {
    S_AuthResult result;
    result.Deserialize(buf);

    if (!result.success) {
        m_State = ClientState::CHARACTER_SELECT;
        m_LastError = "World authentication failed";
    }
}

void GameClient::HandleEnterWorld(ReadBuffer& buf) {
    S_EnterWorld enter;
    enter.Deserialize(buf);

    // Clear old zone data when entering a new zone
    m_Entities.clear();
    m_Projectiles.clear();
    m_PendingEvents.clear();
    m_LocalPlayer.targetId = INVALID_ENTITY_ID;

    m_LocalPlayer.entityId = enter.yourEntityId;
    m_LocalPlayer.position = enter.spawnPosition;
    m_ZoneName = enter.zoneName;
    m_State = ClientState::IN_GAME;

    // Set abilities based on class
    m_LocalPlayer.abilities.clear();
    for (const auto& charInfo : m_CharacterList) {
        if (charInfo.id == m_SelectedCharacterId) {
            m_LocalPlayer.characterClass = charInfo.characterClass;
            m_LocalPlayer.name = charInfo.name;

            switch (charInfo.characterClass) {
                case CharacterClass::WARRIOR:
                    m_LocalPlayer.abilities.push_back(AbilityId::WARRIOR_SLASH);
                    m_LocalPlayer.abilities.push_back(AbilityId::WARRIOR_SHIELD_BASH);
                    m_LocalPlayer.abilities.push_back(AbilityId::WARRIOR_CHARGE);
                    break;
                case CharacterClass::WITCH:
                    m_LocalPlayer.abilities.push_back(AbilityId::WITCH_FIREBALL);
                    m_LocalPlayer.abilities.push_back(AbilityId::WITCH_HEAL);
                    m_LocalPlayer.abilities.push_back(AbilityId::WITCH_FROST_BOLT);
                    break;
                default:
                    break;
            }
            break;
        }
    }

    std::cout << "Entered world: " << m_ZoneName << std::endl;
}

void GameClient::HandleWorldState(ReadBuffer& buf) {
    S_WorldState state;
    state.Deserialize(buf);

    // Update local player position (reconciliation)
    auto it = std::find_if(m_LocalPlayer.pendingInputs.begin(),
                           m_LocalPlayer.pendingInputs.end(),
                           [&state](const auto& p) { return p.first == state.yourLastInputSeq; });

    if (it != m_LocalPlayer.pendingInputs.end()) {
        // Remove acknowledged inputs
        m_LocalPlayer.pendingInputs.erase(m_LocalPlayer.pendingInputs.begin(), it + 1);
    }

    // Small correction if server position differs significantly
    float drift = Vec2::Distance(m_LocalPlayer.position, state.yourPosition);
    if (drift > 0.5f) {
        m_LocalPlayer.position = state.yourPosition;
        m_LocalPlayer.pendingInputs.clear();
    }

    // Update remote entities
    for (const auto& entityState : state.entities) {
        if (entityState.id == m_LocalPlayer.entityId) {
            // Update our own state (health, etc)
            m_LocalPlayer.health = entityState.health;
            m_LocalPlayer.maxHealth = entityState.maxHealth;
            m_LocalPlayer.isCasting = entityState.isCasting;
            m_LocalPlayer.castingAbilityId = entityState.castingAbilityId;
            m_LocalPlayer.castProgress = entityState.castProgress;
            continue;
        }

        auto entityIt = m_Entities.find(entityState.id);
        if (entityIt != m_Entities.end()) {
            // Update existing entity
            auto& entity = entityIt->second;
            entity.previousPosition = entity.position;
            entity.targetPosition = entityState.position;
            entity.interpolationTime = 0.0f;
            entity.rotation = entityState.rotation;
            entity.moveState = entityState.moveState;
            entity.health = entityState.health;
            entity.maxHealth = entityState.maxHealth;
            entity.isCasting = entityState.isCasting;
            entity.castingAbilityId = entityState.castingAbilityId;
            entity.castProgress = entityState.castProgress;
        }
    }
}

void GameClient::HandlePlayerPosition(ReadBuffer& buf) {
    S_PlayerPosition pos;
    pos.Deserialize(buf);

    // Update local player position (reconciliation)
    auto it = std::find_if(m_LocalPlayer.pendingInputs.begin(),
                           m_LocalPlayer.pendingInputs.end(),
                           [&pos](const auto& p) { return p.first == pos.lastInputSeq; });

    if (it != m_LocalPlayer.pendingInputs.end()) {
        // Remove acknowledged inputs
        m_LocalPlayer.pendingInputs.erase(m_LocalPlayer.pendingInputs.begin(), it + 1);
    }

    // Small correction if server position differs significantly
    float drift = Vec2::Distance(m_LocalPlayer.position, pos.position);
    if (drift > 0.5f) {
        m_LocalPlayer.position = pos.position;
        m_LocalPlayer.pendingInputs.clear();
    }
}

void GameClient::HandleEntityUpdate(ReadBuffer& buf) {
    S_EntityUpdate update;
    update.Deserialize(buf);

    // Handle local player updates
    if (update.id == m_LocalPlayer.entityId) {
        if (update.updateMask & UPDATE_HEALTH) {
            m_LocalPlayer.health = update.health;
            m_LocalPlayer.maxHealth = update.maxHealth;
        }
        if (update.updateMask & UPDATE_CASTING) {
            m_LocalPlayer.isCasting = update.isCasting;
            m_LocalPlayer.castingAbilityId = update.castingAbilityId;
            m_LocalPlayer.castProgress = update.castProgress;
        }
        return;
    }

    // Update remote entities
    auto entityIt = m_Entities.find(update.id);
    if (entityIt == m_Entities.end()) return;  // Unknown entity

    auto& entity = entityIt->second;

    if (update.updateMask & UPDATE_POSITION) {
        entity.previousPosition = entity.position;
        entity.targetPosition = update.position;
        entity.interpolationTime = 0.0f;
        entity.rotation = update.rotation;
    }
    if (update.updateMask & UPDATE_MOVE_STATE) {
        entity.moveState = update.moveState;
    }
    if (update.updateMask & UPDATE_HEALTH) {
        entity.health = update.health;
        entity.maxHealth = update.maxHealth;
    }
    // Note: RemoteEntity doesn't track mana or target (not displayed for remote entities)
    if (update.updateMask & UPDATE_CASTING) {
        entity.isCasting = update.isCasting;
        entity.castingAbilityId = update.castingAbilityId;
        entity.castProgress = update.castProgress;
    }
}

void GameClient::HandleEntitySpawn(ReadBuffer& buf) {
    S_EntitySpawn spawn;
    spawn.Deserialize(buf);

    if (spawn.id == m_LocalPlayer.entityId) return;  // Don't add ourselves

    RemoteEntity entity;
    entity.id = spawn.id;
    entity.type = spawn.type;
    entity.name = spawn.name;
    entity.characterClass = spawn.characterClass;
    entity.position = spawn.position;
    entity.previousPosition = spawn.position;
    entity.targetPosition = spawn.position;
    entity.rotation = spawn.rotation;
    entity.health = spawn.health;
    entity.maxHealth = spawn.maxHealth;
    entity.level = spawn.level;
    entity.moveState = MoveState::IDLE;
    entity.interpolationTime = 1.0f;
    entity.isCasting = false;

    m_Entities[spawn.id] = entity;
    std::cout << "Entity spawned: " << spawn.name << " (ID: " << spawn.id << ")" << std::endl;
}

void GameClient::HandleEntityDespawn(ReadBuffer& buf) {
    S_EntityDespawn despawn;
    despawn.Deserialize(buf);

    auto it = m_Entities.find(despawn.id);
    if (it != m_Entities.end()) {
        std::cout << "Entity despawned: " << it->second.name << std::endl;
        m_Entities.erase(it);
    }

    // Clear target if it was the despawned entity
    if (m_LocalPlayer.targetId == despawn.id) {
        m_LocalPlayer.targetId = INVALID_ENTITY_ID;
    }
}

void GameClient::HandleEvent(ReadBuffer& buf) {
    S_Event sEvent;
    sEvent.Deserialize(buf);

    // Store event for UI display
    GameEvent event;
    event.type = sEvent.type;
    event.sourceId = sEvent.sourceId;
    event.targetId = sEvent.targetId;
    event.abilityId = sEvent.abilityId;
    event.value = sEvent.value;
    event.position = sEvent.position;
    event.timestamp = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count() / 1000.0f
    );
    m_PendingEvents.push_back(event);

    // Handle specific events
    switch (sEvent.type) {
        case GameEventType::DAMAGE:
            std::cout << "Damage: " << sEvent.value << " to " << sEvent.targetId << std::endl;
            break;
        case GameEventType::HEAL:
            std::cout << "Heal: " << sEvent.value << " to " << sEvent.targetId << std::endl;
            break;
        case GameEventType::DEATH:
            std::cout << "Death: " << sEvent.targetId << std::endl;
            break;
        case GameEventType::PROJECTILE_SPAWN: {
            // Spawn a new projectile
            ClientProjectile proj;
            proj.id = static_cast<EntityId>(sEvent.value);  // value contains projectile ID
            proj.sourceId = sEvent.sourceId;
            proj.targetId = sEvent.targetId;
            proj.abilityId = sEvent.abilityId;
            proj.position = sEvent.position;
            proj.spawnTime = event.timestamp;
            m_Projectiles.push_back(proj);
            std::cout << "Projectile spawned: " << proj.id << std::endl;
            break;
        }
        case GameEventType::PROJECTILE_HIT: {
            // Remove projectile
            EntityId projId = static_cast<EntityId>(sEvent.value);
            m_Projectiles.erase(
                std::remove_if(m_Projectiles.begin(), m_Projectiles.end(),
                    [projId](const ClientProjectile& p) { return p.id == projId; }),
                m_Projectiles.end()
            );
            std::cout << "Projectile hit: " << projId << std::endl;
            break;
        }
        default:
            break;
    }
}

void GameClient::HandleYourStats(ReadBuffer& buf) {
    S_YourStats stats;
    stats.Deserialize(buf);

    m_LocalPlayer.health = stats.health;
    m_LocalPlayer.maxHealth = stats.maxHealth;
    m_LocalPlayer.mana = stats.mana;
    m_LocalPlayer.maxMana = stats.maxMana;
    m_LocalPlayer.level = stats.level;
    m_LocalPlayer.experience = stats.experience;
    m_LocalPlayer.experienceToNext = stats.experienceToNext;
}

void GameClient::HandleZoneData(ReadBuffer& buf) {
    S_ZoneData zoneData;
    zoneData.Deserialize(buf);

    // Clear old portals and add new ones
    m_Portals.clear();
    for (const auto& portal : zoneData.portals) {
        ClientPortal cp;
        cp.id = portal.id;
        cp.position = portal.position;
        cp.size = portal.size;
        cp.destMapName = portal.destMapName;
        m_Portals.push_back(cp);
    }

    std::cout << "[Client] Received " << m_Portals.size() << " portals for zone" << std::endl;
}

// ============================================================
// LOOT HANDLING
// ============================================================

void GameClient::OpenLoot(EntityId corpseId) {
    if (m_State != ClientState::IN_GAME) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_OPEN_LOOT));
    C_OpenLoot request;
    request.targetId = corpseId;
    request.Serialize(packet);
    m_WorldConnection.Send(packet);
}

void GameClient::TakeLootMoney() {
    if (m_State != ClientState::IN_GAME) return;
    if (!m_LootState.isOpen || m_LootState.corpseId == INVALID_ENTITY_ID) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_TAKE_LOOT_MONEY));
    C_TakeLootMoney request;
    request.corpseId = m_LootState.corpseId;
    request.Serialize(packet);
    m_WorldConnection.Send(packet);
}

void GameClient::TakeLootItem(uint8_t visualSlot) {
    std::cout << "[Client] TakeLootItem called, visualSlot=" << (int)visualSlot << std::endl;

    if (m_State != ClientState::IN_GAME) {
        std::cout << "[Client] TakeLootItem: Not in game" << std::endl;
        return;
    }
    if (!m_LootState.isOpen || m_LootState.corpseId == INVALID_ENTITY_ID) {
        std::cout << "[Client] TakeLootItem: Loot not open or invalid corpse" << std::endl;
        return;
    }
    if (visualSlot >= m_LootState.items.size()) {
        std::cout << "[Client] TakeLootItem: visualSlot " << (int)visualSlot
                  << " >= items.size() " << m_LootState.items.size() << std::endl;
        return;
    }

    // Get the server-assigned slot ID from the item (AzerothCore style)
    uint8_t serverSlotId = m_LootState.items[visualSlot].slotId;
    std::cout << "[Client] TakeLootItem: Sending request for corpse=" << m_LootState.corpseId
              << " serverSlotId=" << (int)serverSlotId << std::endl;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_TAKE_LOOT_ITEM));
    C_TakeLootItem request;
    request.corpseId = m_LootState.corpseId;
    request.lootSlot = serverSlotId;  // Send back the server-assigned slot ID
    request.Serialize(packet);
    m_WorldConnection.Send(packet);
}

void GameClient::CloseLoot() {
    m_LootState.isOpen = false;
    m_LootState.corpseId = INVALID_ENTITY_ID;
    m_LootState.money = 0;
    m_LootState.items.clear();
}

void GameClient::HandleLootContents(ReadBuffer& buf) {
    S_LootContents contents;
    contents.Deserialize(buf);

    m_LootState.isOpen = true;
    m_LootState.corpseId = contents.corpseId;
    m_LootState.money = contents.money;
    m_LootState.items = contents.items;

    std::cout << "[Client] Loot window opened, money: " << contents.money
              << " copper, items: " << contents.items.size() << std::endl;
}

void GameClient::HandleLootTaken(ReadBuffer& buf) {
    S_LootTaken taken;
    taken.Deserialize(buf);

    if (taken.moneyTaken > 0) {
        std::cout << "[Client] Looted " << taken.moneyTaken << " copper" << std::endl;
        m_LootState.money = 0;
    } else if (taken.itemSlotTaken != 255) {
        // Find and remove item by server-assigned slot ID (not array index)
        for (auto it = m_LootState.items.begin(); it != m_LootState.items.end(); ++it) {
            if (it->slotId == taken.itemSlotTaken) {
                std::cout << "[Client] Looted item with slot ID " << (int)taken.itemSlotTaken << std::endl;
                m_LootState.items.erase(it);
                break;
            }
        }
    }

    if (taken.lootEmpty) {
        CloseLoot();
    }
}

void GameClient::HandleMoneyUpdate(ReadBuffer& buf) {
    S_MoneyUpdate update;
    update.Deserialize(buf);

    m_LocalPlayer.money = update.totalCopper;
    std::cout << "[Client] Money updated: " << update.totalCopper << " copper" << std::endl;
}

// ============================================================
// INVENTORY & EQUIPMENT HANDLERS
// ============================================================

void GameClient::HandleInventoryData(ReadBuffer& buf) {
    S_InventoryData data;
    data.Deserialize(buf);

    for (uint8_t i = 0; i < INVENTORY_SIZE; i++) {
        m_Inventory.slots[i] = data.slots[i];
    }

    std::cout << "[Client] Received full inventory data" << std::endl;
}

void GameClient::HandleEquipmentData(ReadBuffer& buf) {
    S_EquipmentData data;
    data.Deserialize(buf);

    for (uint8_t i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
        m_Inventory.equipment[i] = data.slots[i];
    }

    std::cout << "[Client] Received full equipment data" << std::endl;
}

void GameClient::HandleInventoryUpdate(ReadBuffer& buf) {
    S_InventoryUpdate update;
    update.Deserialize(buf);

    if (update.slot < INVENTORY_SIZE) {
        m_Inventory.slots[update.slot] = update.item;
        std::cout << "[Client] Inventory slot " << (int)update.slot << " updated" << std::endl;
    }
}

void GameClient::HandleEquipmentUpdate(ReadBuffer& buf) {
    S_EquipmentUpdate update;
    update.Deserialize(buf);

    uint8_t slot = static_cast<uint8_t>(update.slot);
    if (slot < EQUIPMENT_SLOT_COUNT) {
        m_Inventory.equipment[slot] = update.item;
        std::cout << "[Client] Equipment slot " << (int)slot << " updated" << std::endl;
    }
}

void GameClient::HandleCharacterStats(ReadBuffer& buf) {
    S_CharacterStats stats;
    stats.Deserialize(buf);

    m_Inventory.stats.strength = stats.strength;
    m_Inventory.stats.agility = stats.agility;
    m_Inventory.stats.stamina = stats.stamina;
    m_Inventory.stats.intellect = stats.intellect;
    m_Inventory.stats.spirit = stats.spirit;
    m_Inventory.stats.attackPower = stats.attackPower;
    m_Inventory.stats.spellPower = stats.spellPower;
    m_Inventory.stats.armor = stats.armor;
    m_Inventory.stats.meleeDamageMin = stats.meleeDamageMin;
    m_Inventory.stats.meleeDamageMax = stats.meleeDamageMax;
    m_Inventory.stats.weaponSpeed = stats.weaponSpeed;

    std::cout << "[Client] Character stats updated" << std::endl;
}

// ============================================================
// INVENTORY & EQUIPMENT ACTIONS
// ============================================================

void GameClient::EquipItem(uint8_t inventorySlot, EquipmentSlot equipSlot) {
    if (m_State != ClientState::IN_GAME) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_EQUIP_ITEM));
    C_EquipItem request;
    request.inventorySlot = inventorySlot;
    request.equipSlot = equipSlot;
    request.Serialize(packet);
    m_WorldConnection.Send(packet);
}

void GameClient::UnequipItem(EquipmentSlot equipSlot) {
    if (m_State != ClientState::IN_GAME) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_UNEQUIP_ITEM));
    C_UnequipItem request;
    request.equipSlot = equipSlot;
    request.Serialize(packet);
    m_WorldConnection.Send(packet);
}

void GameClient::SwapInventorySlots(uint8_t slotA, uint8_t slotB) {
    if (m_State != ClientState::IN_GAME) return;

    WriteBuffer packet;
    packet.WriteU8(static_cast<uint8_t>(WorldPacketType::C_SWAP_INVENTORY));
    C_SwapInventory request;
    request.slotA = slotA;
    request.slotB = slotB;
    request.Serialize(packet);
    m_WorldConnection.Send(packet);
}

// ============================================================
// AURA HANDLERS
// ============================================================

void GameClient::HandleAuraUpdate(ReadBuffer& buf) {
    S_AuraUpdate update;
    update.Deserialize(buf);

    // Find the target entity's aura list
    std::vector<ClientAura>* auraList = nullptr;

    if (update.targetId == m_LocalPlayer.entityId) {
        auraList = &m_LocalPlayer.auras;
    } else {
        auto it = m_Entities.find(update.targetId);
        if (it != m_Entities.end()) {
            auraList = &it->second.auras;
        }
    }

    if (!auraList) return;

    switch (update.updateType) {
        case AuraUpdateType::ADD: {
            // Add new aura
            ClientAura aura;
            aura.auraId = update.aura.auraId;
            aura.sourceAbility = update.aura.sourceAbility;
            aura.auraType = update.aura.auraType;
            aura.value = update.aura.value;
            aura.duration = update.aura.duration;
            aura.maxDuration = update.aura.maxDuration;
            aura.casterId = update.aura.casterId;
            auraList->push_back(aura);

            std::cout << "[Client] Aura added: ID=" << aura.auraId
                      << " on entity " << update.targetId << std::endl;
            break;
        }

        case AuraUpdateType::REMOVE: {
            // Remove aura by ID
            auto it = std::find_if(auraList->begin(), auraList->end(),
                [&](const ClientAura& a) { return a.auraId == update.aura.auraId; });
            if (it != auraList->end()) {
                std::cout << "[Client] Aura removed: ID=" << update.aura.auraId
                          << " from entity " << update.targetId << std::endl;
                auraList->erase(it);
            }
            break;
        }

        case AuraUpdateType::REFRESH: {
            // Update duration
            auto it = std::find_if(auraList->begin(), auraList->end(),
                [&](const ClientAura& a) { return a.auraId == update.aura.auraId; });
            if (it != auraList->end()) {
                it->duration = update.aura.duration;
                it->maxDuration = update.aura.maxDuration;
            }
            break;
        }

        default:
            break;
    }
}

void GameClient::HandleAuraUpdateAll(ReadBuffer& buf) {
    S_AuraUpdateAll update;
    update.Deserialize(buf);

    // Find the target entity's aura list
    std::vector<ClientAura>* auraList = nullptr;

    if (update.targetId == m_LocalPlayer.entityId) {
        auraList = &m_LocalPlayer.auras;
    } else {
        auto it = m_Entities.find(update.targetId);
        if (it != m_Entities.end()) {
            auraList = &it->second.auras;
        }
    }

    if (!auraList) return;

    // Clear and replace with new list
    auraList->clear();
    for (const auto& auraInfo : update.auras) {
        ClientAura aura;
        aura.auraId = auraInfo.auraId;
        aura.sourceAbility = auraInfo.sourceAbility;
        aura.auraType = auraInfo.auraType;
        aura.value = auraInfo.value;
        aura.duration = auraInfo.duration;
        aura.maxDuration = auraInfo.maxDuration;
        aura.casterId = auraInfo.casterId;
        auraList->push_back(aura);
    }

    std::cout << "[Client] Received " << update.auras.size()
              << " auras for entity " << update.targetId << std::endl;
}

void GameClient::UpdateAuras(float dt) {
    // Update local player aura durations
    for (auto& aura : m_LocalPlayer.auras) {
        aura.duration -= dt;
    }

    // Remove expired auras (client-side prediction)
    m_LocalPlayer.auras.erase(
        std::remove_if(m_LocalPlayer.auras.begin(), m_LocalPlayer.auras.end(),
            [](const ClientAura& a) { return a.duration <= 0.0f; }),
        m_LocalPlayer.auras.end()
    );

    // Update entity aura durations
    for (auto& [id, entity] : m_Entities) {
        for (auto& aura : entity.auras) {
            aura.duration -= dt;
        }

        // Remove expired auras
        entity.auras.erase(
            std::remove_if(entity.auras.begin(), entity.auras.end(),
                [](const ClientAura& a) { return a.duration <= 0.0f; }),
            entity.auras.end()
        );
    }
}

} // namespace MMO
