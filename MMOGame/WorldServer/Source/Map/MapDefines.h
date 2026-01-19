#pragma once

#include "../../../Shared/Source/Types/Types.h"
#include "../../../Shared/Source/Packets/Packets.h"
#include "../../../Shared/Source/Spells/SpellDefines.h"
#include <string>
#include <vector>
#include <cstdint>

namespace MMO {

// Forward declarations
class CreatureTemplate;

// ============================================================
// PORTAL
// ============================================================

struct Portal {
    uint32_t id;
    Vec2 position;          // Center of portal
    Vec2 size;              // Width/Height of portal area
    uint32_t destMapId;     // Destination map template ID
    Vec2 destPosition;      // Position in destination map

    bool Contains(Vec2 point) const {
        float halfW = size.x / 2.0f;
        float halfH = size.y / 2.0f;
        return point.x >= position.x - halfW && point.x <= position.x + halfW &&
               point.y >= position.y - halfH && point.y <= position.y + halfH;
    }
};

// ============================================================
// MOB SPAWN POINT
// ============================================================

struct MobSpawnPoint {
    uint32_t id;
    uint32_t creatureTemplateId;
    Vec2 position;
    float respawnTimeOverride = 0.0f;      // 0 = use creature template/rank default
    float corpseDecayTimeOverride = 0.0f;  // 0 = use creature template/rank default

    // Get effective respawn time: spawn override > template override > rank default
    float GetRespawnTime(const CreatureTemplate* tmpl) const;

    // Get effective corpse decay time: spawn override > template override > rank default
    float GetCorpseDecayTime(const CreatureTemplate* tmpl) const;
};

// ============================================================
// MAP TEMPLATE (Static Definition)
// ============================================================

struct MapTemplate {
    uint32_t id;
    std::string name;
    float width;
    float height;
    Vec2 spawnPoint;        // Default player spawn
    std::vector<Portal> portals;
    std::vector<MobSpawnPoint> mobSpawns;
};

// ============================================================
// PENDING RESPAWN
// ============================================================

struct PendingRespawn {
    uint32_t spawnPointId;
    float respawnAt;        // World time when respawn should happen
};

// ============================================================
// GAME EVENT
// ============================================================

struct GameEvent {
    GameEventType type;
    EntityId sourceId;
    EntityId targetId;
    AbilityId abilityId;
    int32_t value;
    Vec2 position;
};

// ============================================================
// PROJECTILE
// ============================================================

struct Projectile {
    EntityId id;
    EntityId sourceId;
    EntityId targetId;
    AbilityId abilityId;
    Vec2 position;
    Vec2 targetPosition;
    float speed;
    int32_t damage;
    DamageType damageType = DamageType::PHYSICAL;
    bool isHeal = false;

    // Aura effect to apply on hit (if any)
    AuraType auraType = AuraType::NONE;
    int32_t auraValue = 0;
    float auraDuration = 0.0f;
};

// ============================================================
// LOOT ITEM
// ============================================================

struct LootItem {
    uint8_t slotId = 255;       // Server-assigned slot ID (stable, like AzerothCore)
    ItemId templateId = 0;
    uint32_t stackCount = 1;
    bool looted = false;
};

// ============================================================
// LOOT DATA
// ============================================================

struct LootData {
    EntityId corpseId;
    uint32_t money = 0;             // Copper available to loot
    EntityId killerEntityId;        // Who can loot (the killer)
    float despawnTimer = 0.0f;      // Time until corpse despawns
    bool moneyLooted = false;       // Has money been taken?
    std::vector<LootItem> items;    // Items available to loot

    bool IsEmpty() const {
        if (!moneyLooted && money > 0) return false;
        for (const auto& item : items) {
            if (!item.looted) return false;
        }
        return true;
    }

    int GetUnlootedItemCount() const {
        int count = 0;
        for (const auto& item : items) {
            if (!item.looted) count++;
        }
        return count;
    }

    // Find item by server-assigned slot ID (AzerothCore style)
    LootItem* GetItemBySlot(uint8_t slotId) {
        for (auto& item : items) {
            if (item.slotId == slotId && !item.looted) {
                return &item;
            }
        }
        return nullptr;
    }
};

// ============================================================
// PLAYER INFO
// ============================================================

struct PlayerInfo {
    uint32_t peerId;
    CharacterId characterId;
    AccountId accountId;
    uint32_t lastInputSeq;
};

} // namespace MMO
