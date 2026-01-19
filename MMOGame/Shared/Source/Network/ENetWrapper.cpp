#include "ENetWrapper.h"
#include <iostream>

namespace MMO {

// ============================================================
// ENET INITIALIZER
// ============================================================

bool ENetInitializer::s_Initialized = false;

bool ENetInitializer::Initialize() {
    if (s_Initialized) {
        return true;
    }

    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet" << std::endl;
        return false;
    }

    s_Initialized = true;
    return true;
}

void ENetInitializer::Shutdown() {
    if (s_Initialized) {
        enet_deinitialize();
        s_Initialized = false;
    }
}

bool ENetInitializer::IsInitialized() {
    return s_Initialized;
}

// ============================================================
// NETWORK SERVER
// ============================================================

NetworkServer::NetworkServer()
    : m_Host(nullptr), m_NextPeerId(1) {
}

NetworkServer::~NetworkServer() {
    Stop();
}

bool NetworkServer::Start(uint16_t port, size_t maxClients) {
    if (!ENetInitializer::IsInitialized()) {
        if (!ENetInitializer::Initialize()) {
            return false;
        }
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    m_Host = enet_host_create(&address, maxClients, 2, 0, 0);
    if (!m_Host) {
        std::cerr << "Failed to create ENet server on port " << port << std::endl;
        return false;
    }

    std::cout << "Server started on port " << port << std::endl;
    return true;
}

void NetworkServer::Stop() {
    if (m_Host) {
        // Disconnect all peers
        for (auto& [id, peer] : m_Peers) {
            enet_peer_disconnect_now(peer, 0);
        }
        m_Peers.clear();

        enet_host_destroy(m_Host);
        m_Host = nullptr;
    }
}

void NetworkServer::Poll(std::vector<NetworkEvent>& outEvents, uint32_t timeoutMs) {
    if (!m_Host) return;

    ENetEvent event;
    while (enet_host_service(m_Host, &event, timeoutMs) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                uint32_t peerId = m_NextPeerId++;
                m_Peers[peerId] = event.peer;
                event.peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(peerId));

                NetworkEvent netEvent;
                netEvent.type = NetworkEventType::CONNECTED;
                netEvent.peerId = peerId;
                outEvents.push_back(netEvent);

                std::cout << "Client " << peerId << " connected" << std::endl;
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                uint32_t peerId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event.peer->data));
                m_Peers.erase(peerId);

                NetworkEvent netEvent;
                netEvent.type = NetworkEventType::DISCONNECTED;
                netEvent.peerId = peerId;
                outEvents.push_back(netEvent);

                std::cout << "Client " << peerId << " disconnected" << std::endl;
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                uint32_t peerId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event.peer->data));

                NetworkEvent netEvent;
                netEvent.type = NetworkEventType::DATA_RECEIVED;
                netEvent.peerId = peerId;
                netEvent.data.assign(event.packet->data, event.packet->data + event.packet->dataLength);
                outEvents.push_back(netEvent);

                enet_packet_destroy(event.packet);
                break;
            }

            default:
                break;
        }
        timeoutMs = 0; // Only wait on first call
    }
}

void NetworkServer::Send(uint32_t peerId, const uint8_t* data, size_t size, bool reliable) {
    auto it = m_Peers.find(peerId);
    if (it == m_Peers.end()) return;

    ENetPacket* packet = enet_packet_create(data, size,
        reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_peer_send(it->second, 0, packet);
}

void NetworkServer::Send(uint32_t peerId, const WriteBuffer& buffer, bool reliable) {
    Send(peerId, buffer.Data(), buffer.Size(), reliable);
}

void NetworkServer::Broadcast(const uint8_t* data, size_t size, bool reliable) {
    if (!m_Host) return;

    ENetPacket* packet = enet_packet_create(data, size,
        reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_host_broadcast(m_Host, 0, packet);
}

void NetworkServer::Broadcast(const WriteBuffer& buffer, bool reliable) {
    Broadcast(buffer.Data(), buffer.Size(), reliable);
}

void NetworkServer::DisconnectPeer(uint32_t peerId) {
    auto it = m_Peers.find(peerId);
    if (it != m_Peers.end()) {
        enet_peer_disconnect(it->second, 0);
    }
}

// ============================================================
// NETWORK CLIENT
// ============================================================

NetworkClient::NetworkClient()
    : m_Host(nullptr), m_Peer(nullptr), m_Connected(false) {
}

NetworkClient::~NetworkClient() {
    Disconnect();
}

bool NetworkClient::Connect(const std::string& host, uint16_t port, uint32_t timeoutMs) {
    if (!ENetInitializer::IsInitialized()) {
        if (!ENetInitializer::Initialize()) {
            return false;
        }
    }

    // Create client host
    m_Host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!m_Host) {
        std::cerr << "Failed to create ENet client host" << std::endl;
        return false;
    }

    // Resolve address
    ENetAddress address;
    enet_address_set_host(&address, host.c_str());
    address.port = port;

    // Initiate connection
    m_Peer = enet_host_connect(m_Host, &address, 2, 0);
    if (!m_Peer) {
        std::cerr << "No available peers for connection" << std::endl;
        enet_host_destroy(m_Host);
        m_Host = nullptr;
        return false;
    }

    // Wait for connection
    ENetEvent event;
    if (enet_host_service(m_Host, &event, timeoutMs) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        m_Connected = true;
        std::cout << "Connected to " << host << ":" << port << std::endl;
        return true;
    }

    std::cerr << "Connection to " << host << ":" << port << " failed" << std::endl;
    enet_peer_reset(m_Peer);
    enet_host_destroy(m_Host);
    m_Host = nullptr;
    m_Peer = nullptr;
    return false;
}

void NetworkClient::Disconnect() {
    if (m_Peer) {
        enet_peer_disconnect(m_Peer, 0);

        // Wait for disconnect acknowledgment
        ENetEvent event;
        while (enet_host_service(m_Host, &event, 1000) > 0) {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                break;
            }
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                enet_packet_destroy(event.packet);
            }
        }

        m_Peer = nullptr;
        m_Connected = false;
    }

    if (m_Host) {
        enet_host_destroy(m_Host);
        m_Host = nullptr;
    }
}

void NetworkClient::Poll(std::vector<NetworkEvent>& outEvents, uint32_t timeoutMs) {
    if (!m_Host) return;

    ENetEvent event;
    while (enet_host_service(m_Host, &event, timeoutMs) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                m_Connected = true;
                NetworkEvent netEvent;
                netEvent.type = NetworkEventType::CONNECTED;
                netEvent.peerId = 0;
                outEvents.push_back(netEvent);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                m_Connected = false;
                NetworkEvent netEvent;
                netEvent.type = NetworkEventType::DISCONNECTED;
                netEvent.peerId = 0;
                outEvents.push_back(netEvent);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                NetworkEvent netEvent;
                netEvent.type = NetworkEventType::DATA_RECEIVED;
                netEvent.peerId = 0;
                netEvent.data.assign(event.packet->data, event.packet->data + event.packet->dataLength);
                outEvents.push_back(netEvent);

                enet_packet_destroy(event.packet);
                break;
            }

            default:
                break;
        }
        timeoutMs = 0;
    }
}

void NetworkClient::Send(const uint8_t* data, size_t size, bool reliable) {
    if (!m_Peer || !m_Connected) return;

    ENetPacket* packet = enet_packet_create(data, size,
        reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_peer_send(m_Peer, 0, packet);
}

void NetworkClient::Send(const WriteBuffer& buffer, bool reliable) {
    Send(buffer.Data(), buffer.Size(), reliable);
}

} // namespace MMO
