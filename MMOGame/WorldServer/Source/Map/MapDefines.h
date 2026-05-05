#pragma once

#include "../../../Shared/Source/Packets/Packets.h"
#include "../../../Shared/Source/Spells/SpellDefines.h"
#include "../../../Shared/Source/Types/Types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace MMO {

	// Forward declarations
	class CreatureTemplate;
	class TriggerScript;

	// ============================================================
	// PORTAL
	// ============================================================

	struct Portal
	{
		uint32_t id;
		Vec2 position;		// Center of portal
		Vec2 size;			// Width/Height of portal area
		uint32_t destMapId; // Destination map template ID
		Vec2 destPosition;	// Position in destination map

		bool Contains(Vec2 point) const
		{
			float halfW = size.x / 2.0f;
			float halfH = size.y / 2.0f;
			return point.x >= position.x - halfW && point.x <= position.x + halfW &&
				   point.y >= position.y - halfH && point.y <= position.y + halfH;
		}
	};

	// ============================================================
	// MOB SPAWN POINT
	// ============================================================

	struct MobSpawnPoint
	{
		uint32_t id;	  // Runtime index (assigned by MapManager)
		std::string guid; // DB-stable identity (creature_spawn.guid)
		uint32_t creatureTemplateId;
		Vec2 position;					  // 2D position for grid placement
		float positionZ = 0.0f;			  // Vertical placement
		float orientation = 0.0f;		  // Facing angle (radians)
		float respawnTimeOverride = 0.0f; // 0 = use creature template/rank default
		float wanderRadius = 0.0f;
		uint32_t maxCount = 1;

		// Get effective respawn time: spawn override > template override > rank default
		float GetRespawnTime(const CreatureTemplate* tmpl) const;

		// Corpse decay falls back to template/rank default (no per-spawn override).
		float GetCorpseDecayTime(const CreatureTemplate* tmpl) const;
	};

	// ============================================================
	// SERVER TRIGGER VOLUME
	// ============================================================
	//
	// Server-side runtime form of an editor TriggerVolume. Stored in MapTemplate
	// (immutable for the lifetime of an instance) and indexed by MapInstance
	// into per-cell buckets for cheap movement-driven testing.

	enum class TriggerShapeKind : uint8_t
	{
		BOX = 0,
		SPHERE = 1,
		CAPSULE = 2,
	};

	enum class TriggerEventKind : uint8_t
	{
		ON_ENTER = 0,
		ON_EXIT = 1,
		ON_STAY = 2,
	};

	struct ServerTriggerVolume
	{
		std::string guid;
		TriggerShapeKind shape = TriggerShapeKind::BOX;

		Vec2 position;
		float positionZ = 0.0f;
		float orientation = 0.0f;

		float halfExtentX = 1.0f;
		float halfExtentY = 1.0f;
		float halfExtentZ = 1.0f;
		float radius = 1.0f;

		TriggerEventKind triggerEvent = TriggerEventKind::ON_ENTER;
		bool triggerOnce = false;
		bool triggerPlayers = true;
		bool triggerCreatures = false;

		std::string scriptName;
		uint32_t eventId = 0;

		// Resolved at MapManager::Initialize — no name lookup per dispatch.
		TriggerScript* resolvedScript = nullptr;

		// XY-only (sphere or oriented-box) — used by the cell-bucket fast path.
		bool ContainsXY(Vec2 point) const;

		// Full 3D containment for entities that carry a vertical position.
		bool Contains(Vec2 ground, float vertical) const;

		// Worst-case XY radius — used when bucketing the volume into grid cells.
		float BoundingRadiusXY() const;
	};

	// ============================================================
	// MAP TEMPLATE (Static Definition)
	// ============================================================

	struct MapTemplate
	{
		uint32_t id;
		std::string name;
		float width;
		float height;
		Vec2 spawnPoint; // Default player spawn
		std::string instanceScriptName; // Bound InstanceScript (empty = none)
		std::vector<Portal> portals;
		std::vector<MobSpawnPoint> mobSpawns;
		std::vector<ServerTriggerVolume> triggerVolumes;
	};

	// ============================================================
	// PENDING RESPAWN
	// ============================================================

	struct PendingRespawn
	{
		uint32_t spawnPointId;
		float respawnAt; // World time when respawn should happen
	};

	// ============================================================
	// GAME EVENT
	// ============================================================

	struct GameEvent
	{
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

	struct Projectile
	{
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

	struct LootItem
	{
		uint8_t slotId = 255; // Server-assigned slot ID (stable, like AzerothCore)
		ItemId templateId = 0;
		uint32_t stackCount = 1;
		bool looted = false;
	};

	// ============================================================
	// LOOT DATA
	// ============================================================

	struct LootData
	{
		EntityId corpseId;
		uint32_t money = 0;			 // Copper available to loot
		EntityId killerEntityId;	 // Who can loot (the killer)
		float despawnTimer = 0.0f;	 // Time until corpse despawns
		bool moneyLooted = false;	 // Has money been taken?
		std::vector<LootItem> items; // Items available to loot

		bool IsEmpty() const
		{
			if (!moneyLooted && money > 0)
				return false;
			for (const auto& item : items)
			{
				if (!item.looted)
					return false;
			}
			return true;
		}

		int GetUnlootedItemCount() const
		{
			int count = 0;
			for (const auto& item : items)
			{
				if (!item.looted)
					count++;
			}
			return count;
		}

		// Find item by server-assigned slot ID (AzerothCore style)
		LootItem* GetItemBySlot(uint8_t slotId)
		{
			for (auto& item : items)
			{
				if (item.slotId == slotId && !item.looted)
				{
					return &item;
				}
			}
			return nullptr;
		}
	};

	// ============================================================
	// PLAYER INFO
	// ============================================================

	struct PlayerInfo
	{
		uint32_t peerId;
		CharacterId characterId;
		AccountId accountId;
		uint32_t lastInputSeq;
	};

} // namespace MMO
