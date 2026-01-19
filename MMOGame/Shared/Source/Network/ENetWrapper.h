#pragma once

#include <enet/enet.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include "Buffer.h"

namespace MMO {

// ============================================================
// NETWORK EVENTS
// ============================================================

enum class NetworkEventType {
    CONNECTED,
    DISCONNECTED,
    DATA_RECEIVED
};

struct NetworkEvent {
    NetworkEventType type;
    uint32_t peerId;
    std::vector<uint8_t> data;
};

// ============================================================
// NETWORK SERVER
// ============================================================

class NetworkServer {
public:
    NetworkServer();
    ~NetworkServer();

    bool Start(uint16_t port, size_t maxClients = 32);
    void Stop();
    bool IsRunning() const { return m_Host != nullptr; }

    void Poll(std::vector<NetworkEvent>& outEvents, uint32_t timeoutMs = 0);
    void Send(uint32_t peerId, const uint8_t* data, size_t size, bool reliable = true);
    void Send(uint32_t peerId, const WriteBuffer& buffer, bool reliable = true);
    void Broadcast(const uint8_t* data, size_t size, bool reliable = true);
    void Broadcast(const WriteBuffer& buffer, bool reliable = true);
    void DisconnectPeer(uint32_t peerId);

    size_t GetConnectedPeerCount() const { return m_Peers.size(); }

private:
    ENetHost* m_Host;
    std::unordered_map<uint32_t, ENetPeer*> m_Peers;
    uint32_t m_NextPeerId;
};

// ============================================================
// NETWORK CLIENT
// ============================================================

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool Connect(const std::string& host, uint16_t port, uint32_t timeoutMs = 5000);
    void Disconnect();
    bool IsConnected() const { return m_Peer != nullptr && m_Connected; }

    void Poll(std::vector<NetworkEvent>& outEvents, uint32_t timeoutMs = 0);
    void Send(const uint8_t* data, size_t size, bool reliable = true);
    void Send(const WriteBuffer& buffer, bool reliable = true);

private:
    ENetHost* m_Host;
    ENetPeer* m_Peer;
    bool m_Connected;
};

// ============================================================
// ENET INITIALIZER (RAII)
// ============================================================

class ENetInitializer {
public:
    static bool Initialize();
    static void Shutdown();
    static bool IsInitialized();

private:
    static bool s_Initialized;
};

} // namespace MMO
