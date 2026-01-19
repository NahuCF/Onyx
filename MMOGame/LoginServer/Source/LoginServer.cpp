#include "LoginServer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace MMO {

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================

LoginServer::LoginServer()
    : m_Running(false)
    , m_WorldServerHost("127.0.0.1")
    , m_WorldServerPort(7001)
    , m_Rng(std::random_device{}()) {
}

LoginServer::~LoginServer() {
    Stop();
}

// ============================================================
// INITIALIZATION
// ============================================================

bool LoginServer::Initialize(const std::string& dbConnectionString, uint16_t port) {
    // Connect to database
    if (!m_Database.Connect(dbConnectionString)) {
        std::cerr << "Failed to connect to database" << std::endl;
        return false;
    }

    // Start network server
    if (!m_Network.Start(port)) {
        std::cerr << "Failed to start network server on port " << port << std::endl;
        return false;
    }

    std::cout << "Login Server initialized on port " << port << std::endl;
    return true;
}

void LoginServer::SetWorldServerInfo(const std::string& host, uint16_t port) {
    m_WorldServerHost = host;
    m_WorldServerPort = port;
}

// ============================================================
// MAIN LOOP
// ============================================================

void LoginServer::Run() {
    m_Running = true;
    std::cout << "Login Server running..." << std::endl;

    auto lastCleanup = std::chrono::steady_clock::now();

    while (m_Running) {
        // Poll network events
        std::vector<NetworkEvent> events;
        m_Network.Poll(events, 10);

        for (const auto& event : events) {
            switch (event.type) {
                case NetworkEventType::CONNECTED: {
                    LoginClient client;
                    client.peerId = event.peerId;
                    client.state = LoginClientState::CONNECTED;
                    client.accountId = 0;
                    client.ipAddress = "unknown"; // TODO: Get from ENet
                    m_Clients[event.peerId] = client;
                    break;
                }

                case NetworkEventType::DISCONNECTED:
                    m_Clients.erase(event.peerId);
                    break;

                case NetworkEventType::DATA_RECEIVED:
                    ProcessPacket(event.peerId, event.data);
                    break;
            }
        }

        // Periodic cleanup (every 5 minutes)
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::minutes>(now - lastCleanup).count() >= 5) {
            m_Database.CleanupExpiredSessions();
            lastCleanup = now;
        }
    }
}

void LoginServer::Stop() {
    m_Running = false;
    m_Network.Stop();
    m_Database.Disconnect();
}

// ============================================================
// PACKET PROCESSING
// ============================================================

void LoginServer::ProcessPacket(uint32_t peerId, const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    ReadBuffer buf(data);
    auto packetType = static_cast<LoginPacketType>(buf.ReadU8());

    switch (packetType) {
        case LoginPacketType::C_REGISTER_REQUEST:
            HandleRegisterRequest(peerId, buf);
            break;
        case LoginPacketType::C_LOGIN_REQUEST:
            HandleLoginRequest(peerId, buf);
            break;
        case LoginPacketType::C_CREATE_CHARACTER:
            HandleCreateCharacter(peerId, buf);
            break;
        case LoginPacketType::C_DELETE_CHARACTER:
            HandleDeleteCharacter(peerId, buf);
            break;
        case LoginPacketType::C_SELECT_CHARACTER:
            HandleSelectCharacter(peerId, buf);
            break;
        default:
            std::cerr << "Unknown packet type: " << static_cast<int>(packetType) << std::endl;
            break;
    }
}

// ============================================================
// PACKET HANDLERS
// ============================================================

void LoginServer::HandleRegisterRequest(uint32_t peerId, ReadBuffer& buf) {
    C_RegisterRequest request;
    request.Deserialize(buf);

    std::cout << "Register request from " << request.username << std::endl;

    // Validate username
    if (!ValidateUsername(request.username)) {
        SendError(peerId, ErrorCode::INVALID_USERNAME,
                  "Username must be 3-16 alphanumeric characters");
        return;
    }

    // Validate password
    if (!ValidatePassword(request.password)) {
        SendError(peerId, ErrorCode::INVALID_PASSWORD,
                  "Password must be at least 6 characters");
        return;
    }

    // Check if username exists
    auto existing = m_Database.GetAccountByUsername(request.username);
    if (existing.has_value()) {
        SendError(peerId, ErrorCode::ACCOUNT_EXISTS, "Username already taken");
        return;
    }

    // Create account
    std::string salt = GenerateSalt();
    std::string passwordHash = HashPassword(request.password, salt);

    if (!m_Database.CreateAccount(request.username, request.email, passwordHash, salt)) {
        SendError(peerId, ErrorCode::UNKNOWN_ERROR, "Failed to create account");
        return;
    }

    // Send success response
    WriteBuffer response;
    response.WriteU8(static_cast<uint8_t>(LoginPacketType::S_REGISTER_RESPONSE));
    S_RegisterResponse resp;
    resp.success = true;
    resp.errorCode = ErrorCode::NONE;
    resp.message = "Account created successfully";
    resp.Serialize(response);
    m_Network.Send(peerId, response);

    std::cout << "Account created: " << request.username << std::endl;
}

void LoginServer::HandleLoginRequest(uint32_t peerId, ReadBuffer& buf) {
    C_LoginRequest request;
    request.Deserialize(buf);

    std::cout << "Login request from " << request.username << std::endl;

    // Get account
    auto account = m_Database.GetAccountByUsername(request.username);
    if (!account.has_value()) {
        SendError(peerId, ErrorCode::INVALID_CREDENTIALS, "Invalid username or password");
        return;
    }

    // Check if banned
    if (account->isBanned) {
        SendError(peerId, ErrorCode::INVALID_CREDENTIALS,
                  "Account banned: " + account->banReason);
        return;
    }

    // Verify password
    std::string hashedInput = HashPassword(request.password, account->salt);
    if (hashedInput != account->passwordHash) {
        SendError(peerId, ErrorCode::INVALID_CREDENTIALS, "Invalid username or password");
        return;
    }

    // Generate session token
    std::string sessionToken = GenerateToken();

    // Create session in database
    auto& client = m_Clients[peerId];
    m_Database.CreateSession(sessionToken, account->id, client.ipAddress);
    m_Database.UpdateLastLogin(account->id);

    // Update client state
    client.state = LoginClientState::AUTHENTICATED;
    client.accountId = account->id;
    client.sessionToken = sessionToken;

    // Send success response
    WriteBuffer response;
    response.WriteU8(static_cast<uint8_t>(LoginPacketType::S_LOGIN_RESPONSE));
    S_LoginResponse resp;
    resp.success = true;
    resp.errorCode = ErrorCode::NONE;
    resp.sessionToken = sessionToken;
    resp.accountId = account->id;
    resp.Serialize(response);
    m_Network.Send(peerId, response);

    // Send character list
    SendCharacterList(peerId);

    std::cout << "Login successful: " << request.username << std::endl;
}

void LoginServer::HandleCreateCharacter(uint32_t peerId, ReadBuffer& buf) {
    auto it = m_Clients.find(peerId);
    if (it == m_Clients.end() || it->second.state != LoginClientState::AUTHENTICATED) {
        SendError(peerId, ErrorCode::AUTH_FAILED, "Not authenticated");
        return;
    }

    C_CreateCharacter request;
    request.Deserialize(buf);

    std::cout << "Create character: " << request.name << std::endl;

    // Validate name
    if (!ValidateCharacterName(request.name)) {
        SendError(peerId, ErrorCode::INVALID_NAME,
                  "Name must be 2-12 alphabetic characters");
        return;
    }

    // Check if name taken
    if (m_Database.IsNameTaken(request.name)) {
        SendError(peerId, ErrorCode::NAME_TAKEN, "Character name already taken");
        return;
    }

    // Check character count
    auto characters = m_Database.GetCharactersByAccountId(it->second.accountId);
    if (characters.size() >= 4) {
        SendError(peerId, ErrorCode::MAX_CHARACTERS, "Maximum characters reached");
        return;
    }

    // Create character
    CharacterId newId;
    if (!m_Database.CreateCharacter(it->second.accountId, request.name,
                                    request.characterClass, newId)) {
        SendError(peerId, ErrorCode::UNKNOWN_ERROR, "Failed to create character");
        return;
    }

    // Send success response
    WriteBuffer response;
    response.WriteU8(static_cast<uint8_t>(LoginPacketType::S_CHARACTER_CREATED));
    S_CharacterCreated resp;
    resp.success = true;
    resp.errorCode = ErrorCode::NONE;
    resp.character.id = newId;
    resp.character.name = request.name;
    resp.character.characterClass = request.characterClass;
    resp.character.level = 1;
    resp.Serialize(response);
    m_Network.Send(peerId, response);

    // Send updated character list
    SendCharacterList(peerId);

    std::cout << "Character created: " << request.name << std::endl;
}

void LoginServer::HandleDeleteCharacter(uint32_t peerId, ReadBuffer& buf) {
    auto it = m_Clients.find(peerId);
    if (it == m_Clients.end() || it->second.state != LoginClientState::AUTHENTICATED) {
        SendError(peerId, ErrorCode::AUTH_FAILED, "Not authenticated");
        return;
    }

    C_DeleteCharacter request;
    request.Deserialize(buf);

    // Verify character belongs to account
    auto character = m_Database.GetCharacterById(request.characterId);
    if (!character.has_value() || character->accountId != it->second.accountId) {
        SendError(peerId, ErrorCode::CHARACTER_NOT_FOUND, "Character not found");
        return;
    }

    // Delete character
    m_Database.DeleteCharacter(request.characterId);

    // Send updated character list
    SendCharacterList(peerId);

    std::cout << "Character deleted: " << character->name << std::endl;
}

void LoginServer::HandleSelectCharacter(uint32_t peerId, ReadBuffer& buf) {
    auto it = m_Clients.find(peerId);
    if (it == m_Clients.end() || it->second.state != LoginClientState::AUTHENTICATED) {
        SendError(peerId, ErrorCode::AUTH_FAILED, "Not authenticated");
        return;
    }

    C_SelectCharacter request;
    request.Deserialize(buf);

    // Verify character belongs to account
    auto character = m_Database.GetCharacterById(request.characterId);
    if (!character.has_value() || character->accountId != it->second.accountId) {
        SendError(peerId, ErrorCode::CHARACTER_NOT_FOUND, "Character not found");
        return;
    }

    // Generate world auth token
    std::string authToken = GenerateToken();

    // Send world server info
    WriteBuffer response;
    response.WriteU8(static_cast<uint8_t>(LoginPacketType::S_WORLD_SERVER_INFO));
    S_WorldServerInfo info;
    info.host = m_WorldServerHost;
    info.port = m_WorldServerPort;
    info.authToken = authToken;
    info.characterId = request.characterId;
    info.Serialize(response);
    m_Network.Send(peerId, response);

    std::cout << "Character selected: " << character->name << " -> World Server" << std::endl;
}

// ============================================================
// HELPER METHODS
// ============================================================

void LoginServer::SendCharacterList(uint32_t peerId) {
    auto it = m_Clients.find(peerId);
    if (it == m_Clients.end()) return;

    auto characters = m_Database.GetCharactersByAccountId(it->second.accountId);

    WriteBuffer response;
    response.WriteU8(static_cast<uint8_t>(LoginPacketType::S_CHARACTER_LIST));

    S_CharacterList list;
    list.maxCharacters = 4;
    for (const auto& c : characters) {
        CharacterInfo info;
        info.id = c.id;
        info.name = c.name;
        info.characterClass = c.characterClass;
        info.level = c.level;
        info.zoneName = std::to_string(c.mapId);
        info.lastPlayed = c.lastPlayed;
        list.characters.push_back(info);
    }
    list.Serialize(response);

    m_Network.Send(peerId, response);
}

void LoginServer::SendError(uint32_t peerId, ErrorCode code, const std::string& message) {
    WriteBuffer response;
    response.WriteU8(static_cast<uint8_t>(LoginPacketType::S_ERROR));
    S_Error err;
    err.code = code;
    err.message = message;
    err.Serialize(response);
    m_Network.Send(peerId, response);
}

std::string LoginServer::GenerateToken(size_t length) {
    static const char chars[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);
    std::string token;
    token.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        token += chars[dist(m_Rng)];
    }
    return token;
}

std::string LoginServer::GenerateSalt(size_t length) {
    return GenerateToken(length);
}

std::string LoginServer::HashPassword(const std::string& password, const std::string& salt) {
    std::string combined = password + salt;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()),
           combined.length(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool LoginServer::ValidateUsername(const std::string& username) {
    if (username.length() < 3 || username.length() > 16) {
        return false;
    }
    for (char c : username) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}

bool LoginServer::ValidatePassword(const std::string& password) {
    return password.length() >= 6;
}

bool LoginServer::ValidateCharacterName(const std::string& name) {
    if (name.length() < 2 || name.length() > 12) {
        return false;
    }
    for (char c : name) {
        if (!std::isalpha(c)) {
            return false;
        }
    }
    return true;
}

} // namespace MMO
