#pragma once

#include "../../../Shared/Source/Scripting/ScriptObject.h"
#include <memory>

namespace MMO {

	class CreatureAI;
	class IMapContext;
	class IEntity;

	// ============================================================
	// CREATURE SCRIPT — registered singleton factory.
	//
	// One instance per named script (e.g. "shadow_lord") lives in
	// ScriptRegistry<CreatureScript>. At mob spawn, the registry
	// calls CreateAI() to produce a per-mob CreatureAI instance.
	//
	// Distinguish from BuiltInAI (data-driven archetypes, ai_name
	// column). CreatureScript is engineer-coded; BuiltInAI is
	// designer-configured.
	// ============================================================

	struct CreatureTemplate;

	class CreatureScript : public ScriptObject
	{
	public:
		using ScriptObject::ScriptObject;

		// tmpl is the creature_template DB row that triggered this spawn.
		// Passed through so the AI can use data-driven abilities/phase triggers
		// even for hand-coded scripts that extend the base CreatureAI behavior.
		virtual std::unique_ptr<CreatureAI> CreateAI(IMapContext& map, IEntity& mob,
													  const CreatureTemplate* tmpl) const = 0;
	};

	// Called once at WorldServer startup before any MapInstance is created.
	void RegisterAllCreatureScripts();

} // namespace MMO
