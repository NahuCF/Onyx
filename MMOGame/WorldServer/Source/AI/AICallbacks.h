#pragma once

#include "../../../Shared/Source/Types/Types.h"
#include <functional>

namespace MMO {

class Entity;

// ============================================================
// AI CALLBACKS - Function types for AI actions
// ============================================================

using CastAbilityFn = std::function<void(EntityId source, EntityId target, AbilityId ability)>;
using SummonCreatureFn = std::function<Entity*(uint32_t creatureId, Vec2 position, EntityId summoner)>;
using DespawnCreatureFn = std::function<void(EntityId entityId)>;

} // namespace MMO
