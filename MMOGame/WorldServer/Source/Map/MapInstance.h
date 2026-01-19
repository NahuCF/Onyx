#pragma once

#include "MapDefines.h"
#include "../Entity/Entity.h"
#include "../Grid/Grid.h"
#include "../../../Shared/Source/Types/Types.h"
#include "../../../Shared/Source/Packets/Packets.h"
#include "../../../Shared/Source/Spells/AbilityData.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace MMO {

// Forward declarations
class ScriptedAI;

// ============================================================
// MAP INSTANCE
// ============================================================

class MapInstance {
public:
    MapInstance(uint32_t instanceId, const MapTemplate* tmpl);
    ~MapInstance();

    // Getters
    uint32_t GetInstanceId() const { return m_InstanceId; }
    uint32_t GetTemplateId() const { return m_Template->id; }
    const MapTemplate* GetTemplate() const { return m_Template; }
    const std::string& GetName() const { return m_Template->name; }
    Vec2 GetSpawnPoint() const { return m_Template->spawnPoint; }
    float GetTime() const { return m_Time; }

    // Entity management
    Entity* CreatePlayer(CharacterId characterId, const std::string& name,
                         CharacterClass charClass, uint32_t level, Vec2 position,
                         int32_t currentHealth, int32_t currentMana);
    Entity* CreateMob(uint32_t creatureTemplateId, Vec2 position, uint32_t spawnPointId = 0);
    Entity* SummonCreature(uint32_t creatureTemplateId, Vec2 position, EntityId summonerId);
    void RemoveEntity(EntityId id);
    Entity* GetEntity(EntityId id);

    // Entity transfer (AzerothCore style - move instead of recreate)
    std::unique_ptr<Entity> ReleaseEntity(EntityId id);  // Remove and return ownership
    Entity* AdoptEntity(std::unique_ptr<Entity> entity); // Take ownership of existing entity

    // Player management
    void RegisterPlayer(EntityId entityId, uint32_t peerId, CharacterId characterId, AccountId accountId);
    void UnregisterPlayer(EntityId entityId);
    EntityId GetEntityIdByPeerId(uint32_t peerId) const;
    const PlayerInfo* GetPlayerInfo(EntityId entityId) const;
    const std::unordered_map<EntityId, PlayerInfo>& GetAllPlayers() const { return m_Players; }
    bool IsEmpty() const { return m_Players.empty(); }

    // Queries
    std::vector<Entity*> GetEntitiesInRadius(Vec2 center, float radius);
    std::vector<Entity*> GetPlayersInRadius(Vec2 center, float radius);
    const std::unordered_map<EntityId, std::unique_ptr<Entity>>& GetAllEntities() const { return m_Entities; }

    // Game logic
    void Update(float dt);
    void ProcessInput(EntityId playerId, const C_Input& input);
    void ProcessAbility(EntityId sourceId, EntityId targetId, AbilityId abilityId);
    void ProcessTargetSelection(EntityId playerId, EntityId targetId);

    // Portal checking - returns portal if player is in one, nullptr otherwise
    const Portal* CheckPortal(Vec2 position);

    // Events
    void BroadcastEvent(const GameEvent& event);
    std::vector<GameEvent>& GetPendingEvents() { return m_PendingEvents; }
    void ClearEvents() { m_PendingEvents.clear(); }

    // Aura updates (for WorldServer to send to nearby players)
    struct PendingAuraUpdate {
        EntityId targetId;
        Vec2 targetPosition;
        AuraUpdateType updateType;
        Aura aura;
    };
    std::vector<PendingAuraUpdate>& GetPendingAuraUpdates() { return m_PendingAuraUpdates; }
    void ClearAuraUpdates() { m_PendingAuraUpdates.clear(); }

    // World state
    std::vector<EntityState> GetWorldStateForPlayer(EntityId playerId);

    // Projectiles
    const std::vector<Projectile>& GetProjectiles() const { return m_Projectiles; }

    // Spawn initial mobs
    void SpawnInitialMobs();

    // Loot system
    void GenerateLoot(Entity* mob, EntityId killerEntityId);
    LootData* GetLoot(EntityId corpseId);
    bool TakeLootMoney(EntityId corpseId, EntityId playerId);
    bool TakeLootItem(EntityId corpseId, EntityId playerId, uint8_t lootSlot, ItemInstance& outItem, uint8_t* outInventorySlot = nullptr);
    bool HasLoot(EntityId corpseId) const;

    // XP system
    void AwardXP(Entity* player, Entity* mob);

    // Grid system for spatial queries and dirty tracking
    Grid& GetGrid() { return m_Grid; }
    const Grid& GetGrid() const { return m_Grid; }

    // AI access (for summoner notification)
    ScriptedAI* GetMobAI(EntityId entityId);

    // Dirty flag helpers
    void MarkPositionDirty(EntityId id);
    void MarkHealthDirty(EntityId id);
    void MarkManaDirty(EntityId id);
    void MarkTargetDirty(EntityId id);
    void MarkCastingDirty(EntityId id);
    void MarkMoveStateDirty(EntityId id);

    // For immediate broadcasting - callback when entity changes
    using BroadcastCallback = std::function<void(EntityId entityId, const DirtyFlags& flags)>;
    void SetBroadcastCallback(BroadcastCallback callback) { m_BroadcastCallback = callback; }


private:
    EntityId GenerateEntityId();
    void UpdateMobAI(Entity* mob, float dt);
    void UpdateCasts(float dt);
    void UpdateProjectiles(float dt);
    void UpdateGridActivation(float dt);  // AzerothCore-style grid loading

    // Grid-based mob spawning (AzerothCore-style)
    void SpawnCellMobs(CellCoord coord);   // Spawn mobs for activated cell
    void DespawnCellMobs(CellCoord coord); // Despawn mobs for deactivated cell
    void UpdateRespawns(float dt);
    void UpdateLoot(float dt);
    void UpdateRegeneration(float dt);  // Health/mana regeneration
    void UpdateAuras(float dt);         // Aura tick processing
    void ExecuteAbility(EntityId sourceId, EntityId targetId, AbilityId abilityId, Vec2 targetPosition);
    int32_t CalculateEffectDamage(Entity* source, const SpellEffect& effect);
    void ProcessSpellEffect(Entity* source, Entity* target, const SpellEffect& effect, AbilityId abilityId);
    void SpawnProjectile(Entity* source, Entity* target, const SpellEffect& effect, AbilityId abilityId);
    void ApplyDamage(Entity* source, Entity* target, int32_t damage, AbilityId abilityId, DamageType damageType = DamageType::PHYSICAL);
    void ApplyHeal(Entity* source, Entity* target, int32_t amount, AbilityId abilityId);
    void ApplyAura(Entity* source, Entity* target, const SpellEffect& effect, AbilityId abilityId);
    void BroadcastAuraUpdate(EntityId targetId, const Aura& aura, AuraUpdateType updateType);
    void SendAllAuras(EntityId targetId, uint32_t peerId);  // Send full aura list to specific player

    uint32_t m_InstanceId;
    const MapTemplate* m_Template;

    std::unordered_map<EntityId, std::unique_ptr<Entity>> m_Entities;
    std::unordered_map<EntityId, PlayerInfo> m_Players;
    std::unordered_map<uint32_t, EntityId> m_PeerToEntity;
    std::unordered_map<EntityId, std::unique_ptr<ScriptedAI>> m_MobAIs;
    std::unordered_map<EntityId, uint32_t> m_EntityToSpawnPoint;  // Maps entity to spawn point ID

    std::vector<GameEvent> m_PendingEvents;
    std::vector<PendingAuraUpdate> m_PendingAuraUpdates;
    std::vector<Projectile> m_Projectiles;
    std::vector<PendingRespawn> m_PendingRespawns;
    std::unordered_map<EntityId, LootData> m_Lootables;

    // Grid system for spatial partitioning and dirty tracking
    Grid m_Grid;
    BroadcastCallback m_BroadcastCallback;

    EntityId m_NextProjectileId = 10000;
    float m_Time = 0.0f;

    // Regeneration system (AzerothCore-style tick every 2 seconds)
    float m_RegenTimer = 0.0f;
    static constexpr float REGEN_TICK_INTERVAL = 2.0f;  // Every 2 seconds
    static constexpr float PLAYER_HEALTH_REGEN_PCT = 0.02f;  // 2% per tick out of combat
    static constexpr float PLAYER_MANA_REGEN_PCT = 0.03f;    // 3% per tick (if 5s rule passed)
    static constexpr float MOB_EVADE_REGEN_PCT = 0.05f;      // 5% per tick while evading
};

} // namespace MMO
