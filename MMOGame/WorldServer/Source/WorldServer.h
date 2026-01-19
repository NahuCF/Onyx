#pragma once

#include "Map/Map.h"
#include "../../Shared/Source/Network/ENetWrapper.h"
#include "../../Shared/Source/Packets/Packets.h"
#include "../../Shared/Source/Database/Database.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <chrono>

namespace MMO {

// ============================================================
// PENDING AUTH
// ============================================================

struct PendingAuth {
    uint32_t peerId;
    std::string token;
    CharacterId characterId;
    AccountId accountId;
    std::chrono::steady_clock::time_point expiresAt;
};

// ============================================================
// CONNECTED PLAYER (tracks player across map transfers)
// ============================================================

struct ConnectedPlayer {
    uint32_t peerId;
    CharacterId characterId;
    AccountId accountId;
    uint32_t mapInstanceId;
    EntityId entityId;
};

// ============================================================
// WORLD SERVER
// ============================================================

class WorldServer {
public:
    WorldServer();
    ~WorldServer();

    bool Initialize(uint16_t port = 7001, const std::string& dbConnectionString = "");
    void Run();
    void Stop();

    // For inter-server communication
    void AddPendingAuth(const std::string& token, CharacterId characterId, AccountId accountId);

private:
    void ProcessPacket(uint32_t peerId, const std::vector<uint8_t>& data);

    void HandleAuthToken(uint32_t peerId, ReadBuffer& buf);
    void HandleInput(uint32_t peerId, ReadBuffer& buf);
    void HandleCastAbility(uint32_t peerId, ReadBuffer& buf);
    void HandleSelectTarget(uint32_t peerId, ReadBuffer& buf);
    void HandleUsePortal(uint32_t peerId, ReadBuffer& buf);
    void HandleOpenLoot(uint32_t peerId, ReadBuffer& buf);
    void HandleTakeLootMoney(uint32_t peerId, ReadBuffer& buf);
    void HandleTakeLootItem(uint32_t peerId, ReadBuffer& buf);
    void HandleEquipItem(uint32_t peerId, ReadBuffer& buf);
    void HandleUnequipItem(uint32_t peerId, ReadBuffer& buf);
    void HandleSwapInventory(uint32_t peerId, ReadBuffer& buf);

    void SendEnterWorld(uint32_t peerId, EntityId entityId, MapInstance* map);
    void SendZoneData(uint32_t peerId, MapInstance* map);
    void SendWorldState(MapInstance* map);
    void SendEvents(MapInstance* map);
    void SendAuraUpdates(MapInstance* map);
    void SendSpawnsAndDespawns(MapInstance* map);
    void SendEntitySpawn(uint32_t peerId, Entity* entity);
    void SendEntityDespawn(uint32_t peerId, EntityId entityId);
    void SendYourStats(uint32_t peerId, Entity* player);
    void SendError(uint32_t peerId, ErrorCode code);
    void SendMapChange(uint32_t peerId, EntityId newEntityId, const std::string& mapName, Vec2 position, MapInstance* map);
    void SendLootContents(uint32_t peerId, EntityId corpseId, const LootData* loot);
    void SendLootTaken(uint32_t peerId, EntityId corpseId, uint32_t money, uint8_t itemSlot, bool isEmpty);
    void SendMoneyUpdate(uint32_t peerId, uint32_t totalCopper);
    void SendInventoryData(uint32_t peerId, Entity* player);
    void SendEquipmentData(uint32_t peerId, Entity* player);
    void SendInventoryUpdate(uint32_t peerId, uint8_t slot, const ItemInstance* item);
    void SendEquipmentUpdate(uint32_t peerId, EquipmentSlot slot, const ItemInstance* item);
    void SendCharacterStats(uint32_t peerId, Entity* player);

    // Inventory/Equipment loading and saving
    void LoadPlayerInventory(Entity* player, CharacterId characterId);
    void LoadPlayerEquipment(Entity* player, CharacterId characterId);
    void SavePlayerInventory(Entity* player, CharacterId characterId);
    void SavePlayerEquipment(Entity* player, CharacterId characterId);

    void OnPlayerConnect(uint32_t peerId);
    void OnPlayerDisconnect(uint32_t peerId);

    // Portal handling (click-based)
    void TransferPlayer(uint32_t peerId, const Portal* portal, MapInstance* fromMap);

    // Persistence
    void SavePlayer(const ConnectedPlayer& player);
    CharacterData LoadCharacter(CharacterId characterId);

    NetworkServer m_Network;
    Database m_Database;

    std::unordered_map<std::string, PendingAuth> m_PendingAuths;
    std::unordered_map<uint32_t, ConnectedPlayer> m_ConnectedPlayers;

    // Per-player visibility tracking (AzerothCore-style)
    // Maps peerId -> set of EntityIds the player has been told about
    std::unordered_map<uint32_t, std::unordered_set<EntityId>> m_PlayerKnownEntities;

    bool m_Running;
    uint32_t m_ServerTick;
    std::chrono::steady_clock::time_point m_LastTick;

    static constexpr float TICK_RATE = 20.0f;  // 20 Hz
    static constexpr float TICK_INTERVAL = 1.0f / TICK_RATE;
};

} // namespace MMO
