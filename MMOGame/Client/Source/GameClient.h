#pragma once

#include "../../Shared/Source/Network/ENetWrapper.h"
#include "../../Shared/Source/Packets/Packets.h"
#include "../../Shared/Source/Types/Types.h"
#include "../../Shared/Source/Items/Items.h"
#include "../../Shared/Source/Spells/AbilityData.h"
#include <unordered_map>
#include <string>
#include <functional>
#include <deque>
#include <array>

namespace MMO {

// ============================================================
// CLIENT STATE
// ============================================================

enum class ClientState {
    DISCONNECTED,
    CONNECTING_LOGIN,
    LOGIN_SCREEN,
    CHARACTER_SELECT,
    CONNECTING_WORLD,
    IN_GAME
};

// ============================================================
// CLIENT AURA (for buff/debuff display)
// ============================================================

struct ClientAura {
    uint32_t auraId;
    AbilityId sourceAbility;
    uint8_t auraType;          // AuraType enum
    int32_t value;
    float duration;
    float maxDuration;
    EntityId casterId;

    // Returns true if this is a debuff (negative effect)
    bool IsDebuff() const {
        // MOD_SPEED_PCT with negative value = slow = debuff
        // PERIODIC_DAMAGE = debuff
        // STUN, ROOT, SILENCE = debuff
        return value < 0 || auraType == 5 || auraType >= 8;  // PERIODIC_DAMAGE=5, STUN=8
    }
};

// ============================================================
// REMOTE ENTITY (other players/mobs)
// ============================================================

struct RemoteEntity {
    EntityId id;
    EntityType type;
    std::string name;
    CharacterClass characterClass;
    Vec2 position;
    Vec2 velocity;
    float rotation;
    MoveState moveState;
    int32_t health;
    int32_t maxHealth;
    uint32_t level;
    bool isCasting;
    AbilityId castingAbilityId;
    float castProgress;

    // Interpolation
    Vec2 previousPosition;
    Vec2 targetPosition;
    float interpolationTime;

    // Auras (buffs/debuffs)
    std::vector<ClientAura> auras;
};

// ============================================================
// LOCAL PLAYER
// ============================================================

struct LocalPlayer {
    EntityId entityId;
    std::string name;
    CharacterClass characterClass;
    Vec2 position;
    float rotation;
    int32_t health;
    int32_t maxHealth;
    int32_t mana;
    int32_t maxMana;
    uint32_t level;
    uint64_t experience = 0;
    uint64_t experienceToNext = 400;
    EntityId targetId;
    uint32_t money = 0;  // Total copper

    // Abilities
    std::vector<AbilityId> abilities;
    std::unordered_map<AbilityId, float> cooldowns;
    bool isCasting;
    AbilityId castingAbilityId;
    float castProgress;

    // Input prediction
    uint32_t inputSequence;
    std::deque<std::pair<uint32_t, Vec2>> pendingInputs;

    // Auras (buffs/debuffs on self)
    std::vector<ClientAura> auras;
};

// ============================================================
// LOOT STATE
// ============================================================

struct LootWindowState {
    bool isOpen = false;
    EntityId corpseId = INVALID_ENTITY_ID;
    uint32_t money = 0;
    std::vector<LootItemInfo> items;  // Items available to loot
};

// ============================================================
// CHARACTER STATS (synced from server)
// ============================================================

struct CharacterStats {
    float strength = 0.0f;
    float agility = 0.0f;
    float stamina = 0.0f;
    float intellect = 0.0f;
    float spirit = 0.0f;

    float attackPower = 0.0f;
    float spellPower = 0.0f;
    int32_t armor = 0;
    float meleeDamageMin = 0.0f;
    float meleeDamageMax = 0.0f;
    float weaponSpeed = 2.0f;
};

// ============================================================
// CLIENT INVENTORY/EQUIPMENT
// ============================================================

struct ClientInventory {
    std::array<ItemNetData, INVENTORY_SIZE> slots;
    std::array<ItemNetData, EQUIPMENT_SLOT_COUNT> equipment;
    CharacterStats stats;
};

// ============================================================
// GAME CLIENT
// ============================================================

class GameClient {
public:
    GameClient();
    ~GameClient();

    // Connection
    bool ConnectToLoginServer(const std::string& host, uint16_t port);
    bool ConnectToWorldServer(const std::string& host, uint16_t port);
    void Disconnect();

    // Login flow
    void Register(const std::string& username, const std::string& password, const std::string& email);
    void Login(const std::string& username, const std::string& password);
    void CreateCharacter(const std::string& name, CharacterClass charClass);
    void DeleteCharacter(CharacterId id);
    void SelectCharacter(CharacterId id);

    // Game input
    void SendInput(int8_t moveX, int8_t moveY, float rotation);
    void CastAbility(AbilityId abilityId);
    void SelectTarget(EntityId targetId);
    void UsePortal(uint32_t portalId);

    // Loot
    void OpenLoot(EntityId corpseId);
    void TakeLootMoney();
    void TakeLootItem(uint8_t lootSlot);
    void CloseLoot();
    const LootWindowState& GetLootState() const { return m_LootState; }

    // Inventory/Equipment
    void EquipItem(uint8_t inventorySlot, EquipmentSlot equipSlot);
    void UnequipItem(EquipmentSlot equipSlot);
    void SwapInventorySlots(uint8_t slotA, uint8_t slotB);
    const ClientInventory& GetInventory() const { return m_Inventory; }
    const CharacterStats& GetCharacterStats() const { return m_Inventory.stats; }

    // Update
    void Update(float dt);

    // State access
    ClientState GetState() const { return m_State; }
    const LocalPlayer& GetLocalPlayer() const { return m_LocalPlayer; }
    LocalPlayer& GetLocalPlayer() { return m_LocalPlayer; }
    const std::unordered_map<EntityId, RemoteEntity>& GetEntities() const { return m_Entities; }
    const std::vector<CharacterInfo>& GetCharacterList() const { return m_CharacterList; }
    const std::string& GetLastError() const { return m_LastError; }
    const std::string& GetZoneName() const { return m_ZoneName; }

    // Pending events for UI
    struct GameEvent {
        GameEventType type;
        EntityId sourceId;
        EntityId targetId;
        AbilityId abilityId;
        int32_t value;
        Vec2 position;
        float timestamp;
    };
    const std::vector<GameEvent>& GetPendingEvents() const { return m_PendingEvents; }
    void ClearEvents() { m_PendingEvents.clear(); }

    // Projectiles for rendering
    struct ClientProjectile {
        EntityId id;
        EntityId sourceId;
        EntityId targetId;
        AbilityId abilityId;
        Vec2 position;
        float spawnTime;
    };
    const std::vector<ClientProjectile>& GetProjectiles() const { return m_Projectiles; }

    // Portals for rendering
    struct ClientPortal {
        uint32_t id;
        Vec2 position;
        Vec2 size;
        std::string destMapName;
    };
    const std::vector<ClientPortal>& GetPortals() const { return m_Portals; }

private:
    void ProcessLoginPacket(const std::vector<uint8_t>& data);
    void ProcessWorldPacket(const std::vector<uint8_t>& data);

    void HandleLoginResponse(ReadBuffer& buf);
    void HandleRegisterResponse(ReadBuffer& buf);
    void HandleCharacterList(ReadBuffer& buf);
    void HandleCharacterCreated(ReadBuffer& buf);
    void HandleWorldServerInfo(ReadBuffer& buf);
    void HandleError(ReadBuffer& buf);

    void HandleAuthResult(ReadBuffer& buf);
    void HandleEnterWorld(ReadBuffer& buf);
    void HandleWorldState(ReadBuffer& buf);
    void HandlePlayerPosition(ReadBuffer& buf);
    void HandleEntityUpdate(ReadBuffer& buf);
    void HandleEntitySpawn(ReadBuffer& buf);
    void HandleEntityDespawn(ReadBuffer& buf);
    void HandleEvent(ReadBuffer& buf);
    void HandleYourStats(ReadBuffer& buf);
    void HandleZoneData(ReadBuffer& buf);
    void HandleLootContents(ReadBuffer& buf);
    void HandleLootTaken(ReadBuffer& buf);
    void HandleMoneyUpdate(ReadBuffer& buf);
    void HandleInventoryData(ReadBuffer& buf);
    void HandleEquipmentData(ReadBuffer& buf);
    void HandleInventoryUpdate(ReadBuffer& buf);
    void HandleEquipmentUpdate(ReadBuffer& buf);
    void HandleCharacterStats(ReadBuffer& buf);
    void HandleAuraUpdate(ReadBuffer& buf);
    void HandleAuraUpdateAll(ReadBuffer& buf);

    void InterpolateEntities(float dt);
    void UpdateAuras(float dt);

    NetworkClient m_LoginConnection;
    NetworkClient m_WorldConnection;

    ClientState m_State;
    std::string m_SessionToken;
    AccountId m_AccountId;

    // Character selection
    std::vector<CharacterInfo> m_CharacterList;
    CharacterId m_SelectedCharacterId;

    // World connection info
    std::string m_WorldHost;
    uint16_t m_WorldPort;
    std::string m_WorldAuthToken;

    // Game state
    LocalPlayer m_LocalPlayer;
    std::unordered_map<EntityId, RemoteEntity> m_Entities;
    std::string m_ZoneName;

    // Events
    std::vector<GameEvent> m_PendingEvents;

    // Projectiles
    std::vector<ClientProjectile> m_Projectiles;

    // Portals
    std::vector<ClientPortal> m_Portals;

    // Loot
    LootWindowState m_LootState;

    // Inventory/Equipment
    ClientInventory m_Inventory;

    // Error handling
    std::string m_LastError;

    // Timing
    float m_TimeSinceLastInput;
    static constexpr float INPUT_SEND_RATE = 1.0f / 60.0f;  // 60 Hz
};

} // namespace MMO
