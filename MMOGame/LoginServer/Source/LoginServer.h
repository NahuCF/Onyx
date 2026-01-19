#pragma once

#include "../../Shared/Source/Database/Database.h"
#include "../../Shared/Source/Network/ENetWrapper.h"
#include "../../Shared/Source/Packets/Packets.h"
#include <unordered_map>
#include <string>
#include <random>

namespace MMO {

// ============================================================
// LOGIN CLIENT STATE
// ============================================================

enum class LoginClientState {
    CONNECTED,
    AUTHENTICATED,
    CHARACTER_SELECT
};

struct LoginClient {
    uint32_t peerId;
    LoginClientState state;
    AccountId accountId;
    std::string sessionToken;
    std::string ipAddress;
};

// ============================================================
// LOGIN SERVER
// ============================================================

class LoginServer {
public:
    LoginServer();
    ~LoginServer();

    bool Initialize(const std::string& dbConnectionString, uint16_t port = 7000);
    void Run();
    void Stop();

    // Configuration
    void SetWorldServerInfo(const std::string& host, uint16_t port);

private:
    void ProcessPacket(uint32_t peerId, const std::vector<uint8_t>& data);

    void HandleRegisterRequest(uint32_t peerId, ReadBuffer& buf);
    void HandleLoginRequest(uint32_t peerId, ReadBuffer& buf);
    void HandleCreateCharacter(uint32_t peerId, ReadBuffer& buf);
    void HandleDeleteCharacter(uint32_t peerId, ReadBuffer& buf);
    void HandleSelectCharacter(uint32_t peerId, ReadBuffer& buf);

    void SendCharacterList(uint32_t peerId);
    void SendError(uint32_t peerId, ErrorCode code, const std::string& message);

    // Utility
    std::string GenerateToken(size_t length = 32);
    std::string GenerateSalt(size_t length = 16);
    std::string HashPassword(const std::string& password, const std::string& salt);
    bool ValidateUsername(const std::string& username);
    bool ValidatePassword(const std::string& password);
    bool ValidateCharacterName(const std::string& name);

    NetworkServer m_Network;
    Database m_Database;
    std::unordered_map<uint32_t, LoginClient> m_Clients;

    bool m_Running;
    std::string m_WorldServerHost;
    uint16_t m_WorldServerPort;

    std::mt19937 m_Rng;
};

} // namespace MMO
