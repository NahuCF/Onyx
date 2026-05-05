#pragma once

#include "../../../Shared/Source/Scripting/ScriptObject.h"
#include "../../../Shared/Source/Types/Types.h"
#include "IEntity.h"
#include "IMapContext.h"

namespace MMO {

	// ============================================================
	// SPELL SCRIPT — singleton callback for ability execution hooks.
	//
	// Status: registered; callbacks TBD when spell hook system is built.
	// ============================================================

	class SpellScript : public ScriptObject
	{
	public:
		using ScriptObject::ScriptObject;

		virtual void OnCast(IMapContext& map, IEntity& caster, IEntity* target, AbilityId ability) {}
		virtual void OnHit(IMapContext& map, IEntity& caster, IEntity& target, AbilityId ability, int32_t& damage) {}
		virtual int32_t OnCalculateDamage(IEntity& caster, IEntity& target, AbilityId ability, int32_t baseDamage) { return baseDamage; }
	};

	void RegisterAllSpellScripts();

} // namespace MMO
