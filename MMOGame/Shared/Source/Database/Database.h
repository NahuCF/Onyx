#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include "../Types/Types.h"

namespace MMO {

// ============================================================
// DATABASE STRUCTURES
// ============================================================

struct AccountData {
    AccountId id;
    std::string username;
    std::string passwordHash;
    std::string salt;
    bool isBanned;
    std::string banReason;
};

struct CharacterData {
    CharacterId id;
    AccountId accountId;
    std::string name;
    CharacterRace characterRace;
    CharacterClass characterClass;
    uint32_t level;
    uint64_t experience;
    uint32_t money;          // Total copper (1g = 10000c, 1s = 100c)
    uint32_t mapId;          // Map template ID
    float positionX;
    float positionY;
    float positionZ = 0.0f;
    float orientation = 0.0f;
    int32_t maxHealth;
    int32_t maxMana;
    int32_t currentHealth;
    int32_t currentMana;
    uint64_t lastPlayed;
};

// Map export/load structures
struct MapTemplateData {
    uint32_t id;
    std::string name;
    float width, height;
    float spawnX, spawnY, spawnZ;
};

struct PortalData {
    uint32_t id;
    float positionX, positionY;
    float sizeX, sizeY;
    uint32_t destMapId;
    float destX, destY;
};

struct CreatureSpawnData {
    std::string guid;
    uint32_t creatureTemplateId;
    float positionX, positionY, positionZ;
    float orientation;
    float respawnTime;
    float wanderRadius;
    uint32_t maxCount;
};

struct CooldownData {
    AbilityId abilityId;
    float remaining;
};

// ============================================================
// DATABASE WRAPPER
// ============================================================

class Database {
public:
    Database();
    ~Database();

    bool Connect(const std::string& connectionString);
    void Disconnect();
    bool IsConnected() const { return m_Connection != nullptr; }

    pqxx::connection& GetRawConnection() { return *m_Connection; }

    // Account operations
    std::optional<AccountData> GetAccountByUsername(const std::string& username);
    bool CreateAccount(const std::string& username, const std::string& email,
                       const std::string& passwordHash, const std::string& salt);
    bool UpdateLastLogin(AccountId accountId);

    // Character operations
    std::vector<CharacterData> GetCharactersByAccountId(AccountId accountId);
    std::optional<CharacterData> GetCharacterById(CharacterId characterId);
    bool CreateCharacter(AccountId accountId, const std::string& name,
                         CharacterRace characterRace, CharacterClass characterClass,
                         uint32_t mapId, float posX, float posY, float posZ, float orientation,
                         int32_t maxHealth, int32_t maxMana,
                         CharacterId& outId);
    bool DeleteCharacter(CharacterId characterId);
    bool SaveCharacter(const CharacterData& character);
    bool IsNameTaken(const std::string& name);

    // Map loading (server reads from DB)
    std::vector<MapTemplateData> LoadAllMapTemplates();
    std::vector<PortalData> LoadPortals(uint32_t mapId);
    std::vector<CreatureSpawnData> LoadCreatureSpawns(uint32_t mapId);

    // Race/Class template loading
    std::vector<RaceTemplate> LoadRaceTemplates();
    std::vector<ClassTemplate> LoadClassTemplates();
    struct PlayerCreateInfoRow {
        uint8_t race;
        uint8_t cls;
        PlayerCreateInfo info;
    };
    std::vector<PlayerCreateInfoRow> LoadPlayerCreateInfo();

    // Cooldown operations
    std::vector<CooldownData> GetCooldowns(CharacterId characterId);
    bool SaveCooldowns(CharacterId characterId, const std::vector<CooldownData>& cooldowns);
    bool ClearCooldowns(CharacterId characterId);

    // Session operations
    bool CreateSession(const std::string& token, AccountId accountId,
                       const std::string& ipAddress, int expiresInHours = 24);
    bool ValidateSession(const std::string& token, AccountId& outAccountId);
    bool DeleteSession(const std::string& token);
    void CleanupExpiredSessions();

    // Inventory operations
    struct InventoryItemData {
        ItemInstanceId instanceId;
        ItemId templateId;
        uint8_t slot;
        uint32_t stackCount;
    };

    struct EquipmentItemData {
        ItemInstanceId instanceId;
        ItemId templateId;
        uint8_t slot;
    };

    std::vector<InventoryItemData> GetInventory(CharacterId characterId);
    std::vector<EquipmentItemData> GetEquipment(CharacterId characterId);
    bool SaveInventory(CharacterId characterId, const std::vector<InventoryItemData>& items);
    bool SaveEquipment(CharacterId characterId, const std::vector<EquipmentItemData>& items);

private:
    std::unique_ptr<pqxx::connection> m_Connection;
};

} // namespace MMO
