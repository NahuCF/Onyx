#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include "../../Shared/Source/Types/Types.h"

namespace MMO {

// ============================================================
// DATABASE WRAPPER
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
    CharacterClass characterClass;
    uint32_t level;
    uint64_t experience;
    std::string zoneId;
    float positionX;
    float positionY;
    int32_t maxHealth;
    int32_t maxMana;
    int32_t currentHealth;
    int32_t currentMana;
    uint64_t lastPlayed;
};

class Database {
public:
    Database();
    ~Database();

    bool Connect(const std::string& connectionString);
    void Disconnect();
    bool IsConnected() const { return m_Connection != nullptr; }

    // Account operations
    std::optional<AccountData> GetAccountByUsername(const std::string& username);
    bool CreateAccount(const std::string& username, const std::string& email,
                       const std::string& passwordHash, const std::string& salt);
    bool UpdateLastLogin(AccountId accountId);

    // Character operations
    std::vector<CharacterData> GetCharactersByAccountId(AccountId accountId);
    std::optional<CharacterData> GetCharacterById(CharacterId characterId);
    bool CreateCharacter(AccountId accountId, const std::string& name,
                         CharacterClass characterClass, CharacterId& outId);
    bool DeleteCharacter(CharacterId characterId);
    bool SaveCharacter(const CharacterData& character);
    bool IsNameTaken(const std::string& name);

    // Session operations
    bool CreateSession(const std::string& token, AccountId accountId,
                       const std::string& ipAddress, int expiresInHours = 24);
    bool ValidateSession(const std::string& token, AccountId& outAccountId);
    bool DeleteSession(const std::string& token);
    void CleanupExpiredSessions();

private:
    std::unique_ptr<pqxx::connection> m_Connection;
};

} // namespace MMO
