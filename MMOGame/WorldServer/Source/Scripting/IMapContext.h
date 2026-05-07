#pragma once

#include "../../../Shared/Source/Types/Types.h"
#include <string_view>
#include <vector>

namespace MMO {

	class IEntity;
	struct GameEvent;

	// ============================================================
	// IMAPCONTEXT — minimal interface for map-level operations.
	//
	// MapInstance implements this. A lightweight stub can implement
	// it in unit tests. Scripts receive it by reference in callbacks
	// so they never depend on the concrete MapInstance type.
	//
	// Rule: expose only what scripts ACTUALLY do. Don't mirror the
	// full MapInstance API here.
	// ============================================================

	class IMapContext
	{
	public:
		virtual ~IMapContext() = default;

		virtual std::string_view GetMapName() const = 0;
		virtual float GetTime() const = 0;

		// Entity access
		virtual IEntity* GetEntity(EntityId id) = 0;

		// Script actions
		virtual IEntity* SummonCreature(uint32_t templateId, Vec2 position, EntityId summonerId) = 0;
		virtual void RemoveEntity(EntityId id) = 0;
		virtual void ProcessAbility(EntityId sourceId, EntityId targetId, AbilityId abilityId) = 0;

		// Navmesh path query. Populates outPath with waypoint corners (start … end)
		// and returns true on success. Returns false if there is no navmesh loaded
		// or no path exists; scripts should fall back to straight-line motion.
		virtual bool FindPath(Vec2 start, Vec2 end, std::vector<Vec2>& outPath) const = 0;
	};

} // namespace MMO
