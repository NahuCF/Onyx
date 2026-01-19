#pragma once

#include "../Types/Types.h"
#include "../Network/Buffer.h"
#include <vector>
#include <array>

namespace MMO {

// ============================================================
// PACKET TYPE ENUMS
// ============================================================

enum class LoginPacketType : uint8_t {
    // Client -> Login Server
    C_REGISTER_REQUEST = 0x01,
    C_LOGIN_REQUEST = 0x02,
    C_CREATE_CHARACTER = 0x03,
    C_DELETE_CHARACTER = 0x04,
    C_SELECT_CHARACTER = 0x05,

    // Login Server -> Client
    S_REGISTER_RESPONSE = 0x10,
    S_LOGIN_RESPONSE = 0x11,
    S_CHARACTER_LIST = 0x12,
    S_CHARACTER_CREATED = 0x13,
    S_CHARACTER_DELETED = 0x14,
    S_WORLD_SERVER_INFO = 0x15,
    S_ERROR = 0x1F
};

enum class WorldPacketType : uint8_t {
    // Client -> World Server
    C_AUTH_TOKEN = 0x01,
    C_INPUT = 0x02,
    C_CAST_ABILITY = 0x03,
    C_SELECT_TARGET = 0x04,
    C_CHAT_MESSAGE = 0x05,
    C_USE_PORTAL = 0x06,
    C_OPEN_LOOT = 0x07,
    C_TAKE_LOOT_MONEY = 0x08,
    C_EQUIP_ITEM = 0x09,
    C_UNEQUIP_ITEM = 0x0A,
    C_SWAP_INVENTORY = 0x0B,
    C_TAKE_LOOT_ITEM = 0x0C,

    // World Server -> Client
    S_AUTH_RESULT = 0x10,
    S_ENTER_WORLD = 0x11,
    S_WORLD_STATE = 0x12,
    S_ENTITY_SPAWN = 0x13,
    S_ENTITY_DESPAWN = 0x14,
    S_EVENT = 0x15,
    S_YOUR_STATS = 0x16,
    S_CHAT_MESSAGE = 0x17,
    S_ZONE_DATA = 0x18,
    S_LOOT_CONTENTS = 0x19,
    S_LOOT_TAKEN = 0x1A,
    S_MONEY_UPDATE = 0x1B,
    S_INVENTORY_DATA = 0x1C,
    S_EQUIPMENT_DATA = 0x1D,
    S_INVENTORY_UPDATE = 0x1E,
    S_EQUIPMENT_UPDATE = 0x1F,
    S_CHARACTER_STATS = 0x20,
    S_ENTITY_UPDATE = 0x21,      // Entity-centric update (only changed fields)
    S_PLAYER_POSITION = 0x22,   // Your authoritative position (client prediction reconciliation)
    S_AURA_UPDATE = 0x23,       // Single aura add/update/remove
    S_AURA_UPDATE_ALL = 0x24    // Full aura list (on login, zone change)
};

enum class GameEventType : uint8_t {
    DAMAGE = 1,
    HEAL = 2,
    DEATH = 3,
    RESPAWN = 4,
    CAST_START = 5,
    CAST_CANCEL = 6,
    CAST_END = 7,
    ABILITY_EFFECT = 8,
    BUFF_APPLIED = 9,
    BUFF_REMOVED = 10,
    LEVEL_UP = 11,
    PROJECTILE_SPAWN = 12,
    PROJECTILE_HIT = 13,
    XP_GAIN = 14
};

enum class AuraUpdateType : uint8_t {
    ADD = 0,      // New aura applied
    REMOVE = 1,   // Aura expired or dispelled
    REFRESH = 2,  // Duration refreshed (same aura reapplied)
    STACK = 3     // Stack count changed
};

enum class ErrorCode : uint8_t {
    NONE = 0,
    INVALID_CREDENTIALS = 1,
    ACCOUNT_EXISTS = 2,
    INVALID_USERNAME = 3,
    INVALID_PASSWORD = 4,
    NAME_TAKEN = 5,
    INVALID_NAME = 6,
    MAX_CHARACTERS = 7,
    CHARACTER_NOT_FOUND = 8,
    SERVER_FULL = 9,
    AUTH_FAILED = 10,
    UNKNOWN_ERROR = 255
};

// ============================================================
// LOGIN PACKETS - CLIENT TO SERVER
// ============================================================

struct C_RegisterRequest {
    std::string username;
    std::string password;  // Will be hashed before storage
    std::string email;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteString(username);
        buf.WriteString(password);
        buf.WriteString(email);
    }

    void Deserialize(ReadBuffer& buf) {
        username = buf.ReadString();
        password = buf.ReadString();
        email = buf.ReadString();
    }
};

struct C_LoginRequest {
    std::string username;
    std::string password;
    uint32_t clientVersion;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteString(username);
        buf.WriteString(password);
        buf.WriteU32(clientVersion);
    }

    void Deserialize(ReadBuffer& buf) {
        username = buf.ReadString();
        password = buf.ReadString();
        clientVersion = buf.ReadU32();
    }
};

struct C_CreateCharacter {
    std::string name;
    CharacterClass characterClass;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteString(name);
        buf.WriteU8(static_cast<uint8_t>(characterClass));
    }

    void Deserialize(ReadBuffer& buf) {
        name = buf.ReadString();
        characterClass = static_cast<CharacterClass>(buf.ReadU8());
    }
};

struct C_DeleteCharacter {
    CharacterId characterId;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU64(characterId);
    }

    void Deserialize(ReadBuffer& buf) {
        characterId = buf.ReadU64();
    }
};

struct C_SelectCharacter {
    CharacterId characterId;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU64(characterId);
    }

    void Deserialize(ReadBuffer& buf) {
        characterId = buf.ReadU64();
    }
};

// ============================================================
// LOGIN PACKETS - SERVER TO CLIENT
// ============================================================

struct S_RegisterResponse {
    bool success;
    ErrorCode errorCode;
    std::string message;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteBool(success);
        buf.WriteU8(static_cast<uint8_t>(errorCode));
        buf.WriteString(message);
    }

    void Deserialize(ReadBuffer& buf) {
        success = buf.ReadBool();
        errorCode = static_cast<ErrorCode>(buf.ReadU8());
        message = buf.ReadString();
    }
};

struct S_LoginResponse {
    bool success;
    ErrorCode errorCode;
    std::string sessionToken;
    AccountId accountId;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteBool(success);
        buf.WriteU8(static_cast<uint8_t>(errorCode));
        buf.WriteString(sessionToken);
        buf.WriteU64(accountId);
    }

    void Deserialize(ReadBuffer& buf) {
        success = buf.ReadBool();
        errorCode = static_cast<ErrorCode>(buf.ReadU8());
        sessionToken = buf.ReadString();
        accountId = buf.ReadU64();
    }
};

struct S_CharacterList {
    std::vector<CharacterInfo> characters;
    uint32_t maxCharacters;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(static_cast<uint8_t>(characters.size()));
        for (const auto& c : characters) {
            buf.WriteU64(c.id);
            buf.WriteString(c.name);
            buf.WriteU8(static_cast<uint8_t>(c.characterClass));
            buf.WriteU32(c.level);
            buf.WriteString(c.zoneName);
            buf.WriteU64(c.lastPlayed);
        }
        buf.WriteU32(maxCharacters);
    }

    void Deserialize(ReadBuffer& buf) {
        uint8_t count = buf.ReadU8();
        characters.resize(count);
        for (auto& c : characters) {
            c.id = buf.ReadU64();
            c.name = buf.ReadString();
            c.characterClass = static_cast<CharacterClass>(buf.ReadU8());
            c.level = buf.ReadU32();
            c.zoneName = buf.ReadString();
            c.lastPlayed = buf.ReadU64();
        }
        maxCharacters = buf.ReadU32();
    }
};

struct S_CharacterCreated {
    bool success;
    ErrorCode errorCode;
    CharacterInfo character;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteBool(success);
        buf.WriteU8(static_cast<uint8_t>(errorCode));
        if (success) {
            buf.WriteU64(character.id);
            buf.WriteString(character.name);
            buf.WriteU8(static_cast<uint8_t>(character.characterClass));
            buf.WriteU32(character.level);
        }
    }

    void Deserialize(ReadBuffer& buf) {
        success = buf.ReadBool();
        errorCode = static_cast<ErrorCode>(buf.ReadU8());
        if (success) {
            character.id = buf.ReadU64();
            character.name = buf.ReadString();
            character.characterClass = static_cast<CharacterClass>(buf.ReadU8());
            character.level = buf.ReadU32();
        }
    }
};

struct S_WorldServerInfo {
    std::string host;
    uint16_t port;
    std::string authToken;
    CharacterId characterId;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteString(host);
        buf.WriteU16(port);
        buf.WriteString(authToken);
        buf.WriteU64(characterId);
    }

    void Deserialize(ReadBuffer& buf) {
        host = buf.ReadString();
        port = buf.ReadU16();
        authToken = buf.ReadString();
        characterId = buf.ReadU64();
    }
};

struct S_Error {
    ErrorCode code;
    std::string message;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(static_cast<uint8_t>(code));
        buf.WriteString(message);
    }

    void Deserialize(ReadBuffer& buf) {
        code = static_cast<ErrorCode>(buf.ReadU8());
        message = buf.ReadString();
    }
};

// ============================================================
// WORLD PACKETS - CLIENT TO SERVER
// ============================================================

struct C_AuthToken {
    std::string token;
    CharacterId characterId;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteString(token);
        buf.WriteU64(characterId);
    }

    void Deserialize(ReadBuffer& buf) {
        token = buf.ReadString();
        characterId = buf.ReadU64();
    }
};

struct C_Input {
    uint32_t sequence;
    int8_t moveX;  // -1, 0, 1
    int8_t moveY;  // -1, 0, 1
    float rotation;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(sequence);
        buf.WriteI8(moveX);
        buf.WriteI8(moveY);
        buf.WriteF32(rotation);
    }

    void Deserialize(ReadBuffer& buf) {
        sequence = buf.ReadU32();
        moveX = buf.ReadI8();
        moveY = buf.ReadI8();
        rotation = buf.ReadF32();
    }
};

struct C_CastAbility {
    AbilityId abilityId;
    EntityId targetId;
    Vec2 targetPosition;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU16(static_cast<uint16_t>(abilityId));
        buf.WriteU32(targetId);
        buf.WriteVec2(targetPosition);
    }

    void Deserialize(ReadBuffer& buf) {
        abilityId = static_cast<AbilityId>(buf.ReadU16());
        targetId = buf.ReadU32();
        targetPosition = buf.ReadVec2();
    }
};

struct C_SelectTarget {
    EntityId targetId;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(targetId);
    }

    void Deserialize(ReadBuffer& buf) {
        targetId = buf.ReadU32();
    }
};

struct C_UsePortal {
    uint32_t portalId;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(portalId);
    }

    void Deserialize(ReadBuffer& buf) {
        portalId = buf.ReadU32();
    }
};

// ============================================================
// WORLD PACKETS - SERVER TO CLIENT
// ============================================================

struct S_AuthResult {
    bool success;
    ErrorCode errorCode;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteBool(success);
        buf.WriteU8(static_cast<uint8_t>(errorCode));
    }

    void Deserialize(ReadBuffer& buf) {
        success = buf.ReadBool();
        errorCode = static_cast<ErrorCode>(buf.ReadU8());
    }
};

struct S_EnterWorld {
    EntityId yourEntityId;
    Vec2 spawnPosition;
    std::string zoneName;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(yourEntityId);
        buf.WriteVec2(spawnPosition);
        buf.WriteString(zoneName);
    }

    void Deserialize(ReadBuffer& buf) {
        yourEntityId = buf.ReadU32();
        spawnPosition = buf.ReadVec2();
        zoneName = buf.ReadString();
    }
};

struct EntityState {
    EntityId id;
    EntityType type;
    Vec2 position;
    float rotation;
    MoveState moveState;
    int32_t health;
    int32_t maxHealth;
    int32_t mana;
    int32_t maxMana;
    EntityId targetId;
    bool isCasting;
    AbilityId castingAbilityId;
    float castProgress;  // 0-1

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(id);
        buf.WriteU8(static_cast<uint8_t>(type));
        buf.WriteVec2(position);
        buf.WriteF32(rotation);
        buf.WriteU8(static_cast<uint8_t>(moveState));
        buf.WriteI32(health);
        buf.WriteI32(maxHealth);
        buf.WriteI32(mana);
        buf.WriteI32(maxMana);
        buf.WriteU32(targetId);
        buf.WriteBool(isCasting);
        if (isCasting) {
            buf.WriteU16(static_cast<uint16_t>(castingAbilityId));
            buf.WriteF32(castProgress);
        }
    }

    void Deserialize(ReadBuffer& buf) {
        id = buf.ReadU32();
        type = static_cast<EntityType>(buf.ReadU8());
        position = buf.ReadVec2();
        rotation = buf.ReadF32();
        moveState = static_cast<MoveState>(buf.ReadU8());
        health = buf.ReadI32();
        maxHealth = buf.ReadI32();
        mana = buf.ReadI32();
        maxMana = buf.ReadI32();
        targetId = buf.ReadU32();
        isCasting = buf.ReadBool();
        if (isCasting) {
            castingAbilityId = static_cast<AbilityId>(buf.ReadU16());
            castProgress = buf.ReadF32();
        }
    }
};

struct S_WorldState {
    uint32_t serverTick;
    uint32_t yourLastInputSeq;
    Vec2 yourPosition;
    std::vector<EntityState> entities;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(serverTick);
        buf.WriteU32(yourLastInputSeq);
        buf.WriteVec2(yourPosition);
        buf.WriteU16(static_cast<uint16_t>(entities.size()));
        for (const auto& e : entities) {
            e.Serialize(buf);
        }
    }

    void Deserialize(ReadBuffer& buf) {
        serverTick = buf.ReadU32();
        yourLastInputSeq = buf.ReadU32();
        yourPosition = buf.ReadVec2();
        uint16_t count = buf.ReadU16();
        entities.resize(count);
        for (auto& e : entities) {
            e.Deserialize(buf);
        }
    }
};

// Update mask bits (matches DirtyFlags in Grid.h)
enum EntityUpdateMask : uint8_t {
    UPDATE_POSITION = 0x01,
    UPDATE_HEALTH = 0x02,
    UPDATE_MANA = 0x04,
    UPDATE_TARGET = 0x08,
    UPDATE_CASTING = 0x10,
    UPDATE_MOVE_STATE = 0x20
};

// Entity-centric update packet - only sends changed fields
struct S_EntityUpdate {
    EntityId id;
    uint8_t updateMask;

    // Optional fields (only present if bit is set in updateMask)
    Vec2 position;
    float rotation;
    MoveState moveState;
    int32_t health;
    int32_t maxHealth;
    int32_t mana;
    int32_t maxMana;
    EntityId targetId;
    bool isCasting;
    AbilityId castingAbilityId;
    float castProgress;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(id);
        buf.WriteU8(updateMask);

        if (updateMask & UPDATE_POSITION) {
            buf.WriteVec2(position);
            buf.WriteF32(rotation);
        }
        if (updateMask & UPDATE_MOVE_STATE) {
            buf.WriteU8(static_cast<uint8_t>(moveState));
        }
        if (updateMask & UPDATE_HEALTH) {
            buf.WriteI32(health);
            buf.WriteI32(maxHealth);
        }
        if (updateMask & UPDATE_MANA) {
            buf.WriteI32(mana);
            buf.WriteI32(maxMana);
        }
        if (updateMask & UPDATE_TARGET) {
            buf.WriteU32(targetId);
        }
        if (updateMask & UPDATE_CASTING) {
            buf.WriteBool(isCasting);
            if (isCasting) {
                buf.WriteU16(static_cast<uint16_t>(castingAbilityId));
                buf.WriteF32(castProgress);
            }
        }
    }

    void Deserialize(ReadBuffer& buf) {
        id = buf.ReadU32();
        updateMask = buf.ReadU8();

        if (updateMask & UPDATE_POSITION) {
            position = buf.ReadVec2();
            rotation = buf.ReadF32();
        }
        if (updateMask & UPDATE_MOVE_STATE) {
            moveState = static_cast<MoveState>(buf.ReadU8());
        }
        if (updateMask & UPDATE_HEALTH) {
            health = buf.ReadI32();
            maxHealth = buf.ReadI32();
        }
        if (updateMask & UPDATE_MANA) {
            mana = buf.ReadI32();
            maxMana = buf.ReadI32();
        }
        if (updateMask & UPDATE_TARGET) {
            targetId = buf.ReadU32();
        }
        if (updateMask & UPDATE_CASTING) {
            isCasting = buf.ReadBool();
            if (isCasting) {
                castingAbilityId = static_cast<AbilityId>(buf.ReadU16());
                castProgress = buf.ReadF32();
            }
        }
    }
};

// Player's authoritative position (for client prediction reconciliation)
struct S_PlayerPosition {
    uint32_t serverTick;
    uint32_t lastInputSeq;
    Vec2 position;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(serverTick);
        buf.WriteU32(lastInputSeq);
        buf.WriteVec2(position);
    }

    void Deserialize(ReadBuffer& buf) {
        serverTick = buf.ReadU32();
        lastInputSeq = buf.ReadU32();
        position = buf.ReadVec2();
    }
};

struct S_EntitySpawn {
    EntityId id;
    EntityType type;
    std::string name;
    CharacterClass characterClass;  // For players
    Vec2 position;
    float rotation;
    int32_t health;
    int32_t maxHealth;
    int32_t level;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(id);
        buf.WriteU8(static_cast<uint8_t>(type));
        buf.WriteString(name);
        buf.WriteU8(static_cast<uint8_t>(characterClass));
        buf.WriteVec2(position);
        buf.WriteF32(rotation);
        buf.WriteI32(health);
        buf.WriteI32(maxHealth);
        buf.WriteI32(level);
    }

    void Deserialize(ReadBuffer& buf) {
        id = buf.ReadU32();
        type = static_cast<EntityType>(buf.ReadU8());
        name = buf.ReadString();
        characterClass = static_cast<CharacterClass>(buf.ReadU8());
        position = buf.ReadVec2();
        rotation = buf.ReadF32();
        health = buf.ReadI32();
        maxHealth = buf.ReadI32();
        level = buf.ReadI32();
    }
};

struct S_EntityDespawn {
    EntityId id;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(id);
    }

    void Deserialize(ReadBuffer& buf) {
        id = buf.ReadU32();
    }
};

struct S_Event {
    GameEventType type;
    EntityId sourceId;
    EntityId targetId;
    AbilityId abilityId;
    int32_t value;
    Vec2 position;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(static_cast<uint8_t>(type));
        buf.WriteU32(sourceId);
        buf.WriteU32(targetId);
        buf.WriteU16(static_cast<uint16_t>(abilityId));
        buf.WriteI32(value);
        buf.WriteVec2(position);
    }

    void Deserialize(ReadBuffer& buf) {
        type = static_cast<GameEventType>(buf.ReadU8());
        sourceId = buf.ReadU32();
        targetId = buf.ReadU32();
        abilityId = static_cast<AbilityId>(buf.ReadU16());
        value = buf.ReadI32();
        position = buf.ReadVec2();
    }
};

struct S_YourStats {
    int32_t health;
    int32_t maxHealth;
    int32_t mana;
    int32_t maxMana;
    uint32_t level;
    uint64_t experience;
    uint64_t experienceToNext;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteI32(health);
        buf.WriteI32(maxHealth);
        buf.WriteI32(mana);
        buf.WriteI32(maxMana);
        buf.WriteU32(level);
        buf.WriteU64(experience);
        buf.WriteU64(experienceToNext);
    }

    void Deserialize(ReadBuffer& buf) {
        health = buf.ReadI32();
        maxHealth = buf.ReadI32();
        mana = buf.ReadI32();
        maxMana = buf.ReadI32();
        level = buf.ReadU32();
        experience = buf.ReadU64();
        experienceToNext = buf.ReadU64();
    }
};

// Portal info for client rendering
struct PortalInfo {
    uint32_t id;
    Vec2 position;
    Vec2 size;
    std::string destMapName;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(id);
        buf.WriteVec2(position);
        buf.WriteVec2(size);
        buf.WriteString(destMapName);
    }

    void Deserialize(ReadBuffer& buf) {
        id = buf.ReadU32();
        position = buf.ReadVec2();
        size = buf.ReadVec2();
        destMapName = buf.ReadString();
    }
};

struct S_ZoneData {
    std::vector<PortalInfo> portals;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(static_cast<uint8_t>(portals.size()));
        for (const auto& portal : portals) {
            portal.Serialize(buf);
        }
    }

    void Deserialize(ReadBuffer& buf) {
        uint8_t count = buf.ReadU8();
        portals.resize(count);
        for (auto& portal : portals) {
            portal.Deserialize(buf);
        }
    }
};

// ============================================================
// AURA PACKETS
// ============================================================

// Single aura info (used in both S_AuraUpdate and S_AuraUpdateAll)
struct AuraInfo {
    uint32_t auraId;           // Unique instance ID
    AbilityId sourceAbility;   // Which ability applied it (for icon/name)
    uint8_t auraType;          // AuraType enum
    int32_t value;             // Effect value (e.g., -40 for slow)
    float duration;            // Time remaining
    float maxDuration;         // Total duration (for progress bar)
    EntityId casterId;         // Who applied it

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(auraId);
        buf.WriteU16(static_cast<uint16_t>(sourceAbility));
        buf.WriteU8(auraType);
        buf.WriteI32(value);
        buf.WriteF32(duration);
        buf.WriteF32(maxDuration);
        buf.WriteU32(casterId);
    }

    void Deserialize(ReadBuffer& buf) {
        auraId = buf.ReadU32();
        sourceAbility = static_cast<AbilityId>(buf.ReadU16());
        auraType = buf.ReadU8();
        value = buf.ReadI32();
        duration = buf.ReadF32();
        maxDuration = buf.ReadF32();
        casterId = buf.ReadU32();
    }
};

// Single aura update (add/remove/refresh)
struct S_AuraUpdate {
    EntityId targetId;         // Entity that has the aura
    AuraUpdateType updateType; // ADD, REMOVE, REFRESH
    AuraInfo aura;             // Aura details (ignored for REMOVE except auraId)

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(targetId);
        buf.WriteU8(static_cast<uint8_t>(updateType));
        aura.Serialize(buf);
    }

    void Deserialize(ReadBuffer& buf) {
        targetId = buf.ReadU32();
        updateType = static_cast<AuraUpdateType>(buf.ReadU8());
        aura.Deserialize(buf);
    }
};

// Full aura list for an entity (sent on login, zone change)
struct S_AuraUpdateAll {
    EntityId targetId;
    std::vector<AuraInfo> auras;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(targetId);
        buf.WriteU8(static_cast<uint8_t>(auras.size()));
        for (const auto& aura : auras) {
            aura.Serialize(buf);
        }
    }

    void Deserialize(ReadBuffer& buf) {
        targetId = buf.ReadU32();
        uint8_t count = buf.ReadU8();
        auras.resize(count);
        for (auto& aura : auras) {
            aura.Deserialize(buf);
        }
    }
};

// ============================================================
// LOOT PACKETS
// ============================================================

struct C_OpenLoot {
    EntityId targetId;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(targetId);
    }

    void Deserialize(ReadBuffer& buf) {
        targetId = buf.ReadU32();
    }
};

struct C_TakeLootMoney {
    EntityId corpseId;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(corpseId);
    }

    void Deserialize(ReadBuffer& buf) {
        corpseId = buf.ReadU32();
    }
};

// Loot item info for network transfer
struct LootItemInfo {
    uint8_t slotId = 255;       // Server-assigned slot ID (like AzerothCore)
    ItemId templateId = INVALID_ITEM_ID;
    uint32_t stackCount = 1;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(slotId);
        buf.WriteU32(templateId);
        buf.WriteU32(stackCount);
    }

    void Deserialize(ReadBuffer& buf) {
        slotId = buf.ReadU8();
        templateId = buf.ReadU32();
        stackCount = buf.ReadU32();
    }
};

struct S_LootContents {
    EntityId corpseId;
    uint32_t money;         // Copper
    std::vector<LootItemInfo> items;  // Items available to loot

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(corpseId);
        buf.WriteU32(money);
        buf.WriteU8(static_cast<uint8_t>(items.size()));
        for (const auto& item : items) {
            item.Serialize(buf);
        }
    }

    void Deserialize(ReadBuffer& buf) {
        corpseId = buf.ReadU32();
        money = buf.ReadU32();
        uint8_t itemCount = buf.ReadU8();
        items.resize(itemCount);
        for (auto& item : items) {
            item.Deserialize(buf);
        }
    }
};

struct S_LootTaken {
    EntityId corpseId;
    uint32_t moneyTaken;    // Copper taken (0 if item was taken)
    uint8_t itemSlotTaken;  // Slot of item taken (255 if money was taken)
    bool lootEmpty;         // Should close loot window?

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(corpseId);
        buf.WriteU32(moneyTaken);
        buf.WriteU8(itemSlotTaken);
        buf.WriteU8(lootEmpty ? 1 : 0);
    }

    void Deserialize(ReadBuffer& buf) {
        corpseId = buf.ReadU32();
        moneyTaken = buf.ReadU32();
        itemSlotTaken = buf.ReadU8();
        lootEmpty = buf.ReadU8() != 0;
    }
};

struct S_MoneyUpdate {
    uint32_t totalCopper;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(totalCopper);
    }

    void Deserialize(ReadBuffer& buf) {
        totalCopper = buf.ReadU32();
    }
};

// ============================================================
// INVENTORY & EQUIPMENT PACKETS
// ============================================================

// Item instance data for network transfer
struct ItemNetData {
    ItemInstanceId instanceId = INVALID_ITEM_INSTANCE_ID;
    ItemId templateId = INVALID_ITEM_ID;
    uint32_t stackCount = 1;

    bool IsEmpty() const { return templateId == INVALID_ITEM_ID; }

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU64(instanceId);
        buf.WriteU32(templateId);
        buf.WriteU32(stackCount);
    }

    void Deserialize(ReadBuffer& buf) {
        instanceId = buf.ReadU64();
        templateId = buf.ReadU32();
        stackCount = buf.ReadU32();
    }
};

// Full inventory sent on login
struct S_InventoryData {
    std::array<ItemNetData, INVENTORY_SIZE> slots;

    void Serialize(WriteBuffer& buf) const {
        for (uint8_t i = 0; i < INVENTORY_SIZE; i++) {
            buf.WriteBool(!slots[i].IsEmpty());
            if (!slots[i].IsEmpty()) {
                slots[i].Serialize(buf);
            }
        }
    }

    void Deserialize(ReadBuffer& buf) {
        for (uint8_t i = 0; i < INVENTORY_SIZE; i++) {
            bool hasItem = buf.ReadBool();
            if (hasItem) {
                slots[i].Deserialize(buf);
            } else {
                slots[i] = ItemNetData{};
            }
        }
    }
};

// Full equipment sent on login
struct S_EquipmentData {
    std::array<ItemNetData, EQUIPMENT_SLOT_COUNT> slots;

    void Serialize(WriteBuffer& buf) const {
        for (uint8_t i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
            buf.WriteBool(!slots[i].IsEmpty());
            if (!slots[i].IsEmpty()) {
                slots[i].Serialize(buf);
            }
        }
    }

    void Deserialize(ReadBuffer& buf) {
        for (uint8_t i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
            bool hasItem = buf.ReadBool();
            if (hasItem) {
                slots[i].Deserialize(buf);
            } else {
                slots[i] = ItemNetData{};
            }
        }
    }
};

// Single inventory slot update
struct S_InventoryUpdate {
    uint8_t slot;
    ItemNetData item;  // Empty if slot cleared

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(slot);
        buf.WriteBool(!item.IsEmpty());
        if (!item.IsEmpty()) {
            item.Serialize(buf);
        }
    }

    void Deserialize(ReadBuffer& buf) {
        slot = buf.ReadU8();
        bool hasItem = buf.ReadBool();
        if (hasItem) {
            item.Deserialize(buf);
        } else {
            item = ItemNetData{};
        }
    }
};

// Single equipment slot update
struct S_EquipmentUpdate {
    EquipmentSlot slot;
    ItemNetData item;  // Empty if slot cleared

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(static_cast<uint8_t>(slot));
        buf.WriteBool(!item.IsEmpty());
        if (!item.IsEmpty()) {
            item.Serialize(buf);
        }
    }

    void Deserialize(ReadBuffer& buf) {
        slot = static_cast<EquipmentSlot>(buf.ReadU8());
        bool hasItem = buf.ReadBool();
        if (hasItem) {
            item.Deserialize(buf);
        } else {
            item = ItemNetData{};
        }
    }
};

// Character stats (sent when gear changes)
struct S_CharacterStats {
    // Primary stats
    float strength;
    float agility;
    float stamina;
    float intellect;
    float spirit;

    // Derived stats
    float attackPower;
    float spellPower;
    int32_t armor;
    float meleeDamageMin;
    float meleeDamageMax;
    float weaponSpeed;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteF32(strength);
        buf.WriteF32(agility);
        buf.WriteF32(stamina);
        buf.WriteF32(intellect);
        buf.WriteF32(spirit);
        buf.WriteF32(attackPower);
        buf.WriteF32(spellPower);
        buf.WriteI32(armor);
        buf.WriteF32(meleeDamageMin);
        buf.WriteF32(meleeDamageMax);
        buf.WriteF32(weaponSpeed);
    }

    void Deserialize(ReadBuffer& buf) {
        strength = buf.ReadF32();
        agility = buf.ReadF32();
        stamina = buf.ReadF32();
        intellect = buf.ReadF32();
        spirit = buf.ReadF32();
        attackPower = buf.ReadF32();
        spellPower = buf.ReadF32();
        armor = buf.ReadI32();
        meleeDamageMin = buf.ReadF32();
        meleeDamageMax = buf.ReadF32();
        weaponSpeed = buf.ReadF32();
    }
};

// Client requests to equip item from inventory
struct C_EquipItem {
    uint8_t inventorySlot;
    EquipmentSlot equipSlot;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(inventorySlot);
        buf.WriteU8(static_cast<uint8_t>(equipSlot));
    }

    void Deserialize(ReadBuffer& buf) {
        inventorySlot = buf.ReadU8();
        equipSlot = static_cast<EquipmentSlot>(buf.ReadU8());
    }
};

// Client requests to unequip item to inventory
struct C_UnequipItem {
    EquipmentSlot equipSlot;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(static_cast<uint8_t>(equipSlot));
    }

    void Deserialize(ReadBuffer& buf) {
        equipSlot = static_cast<EquipmentSlot>(buf.ReadU8());
    }
};

// Client requests to swap inventory slots
struct C_SwapInventory {
    uint8_t slotA;
    uint8_t slotB;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU8(slotA);
        buf.WriteU8(slotB);
    }

    void Deserialize(ReadBuffer& buf) {
        slotA = buf.ReadU8();
        slotB = buf.ReadU8();
    }
};

// Client requests to loot an item from corpse
struct C_TakeLootItem {
    EntityId corpseId;
    uint8_t lootSlot;

    void Serialize(WriteBuffer& buf) const {
        buf.WriteU32(corpseId);
        buf.WriteU8(lootSlot);
    }

    void Deserialize(ReadBuffer& buf) {
        corpseId = buf.ReadU32();
        lootSlot = buf.ReadU8();
    }
};

} // namespace MMO
