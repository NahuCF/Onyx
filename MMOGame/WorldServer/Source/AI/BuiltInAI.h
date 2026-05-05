#pragma once

#include "../../../Shared/Source/Scripting/ScriptObject.h"
#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include <memory>

namespace MMO {

	class CreatureAI;
	class IMapContext;
	class IEntity;

	// ============================================================
	// BUILT-IN AI — data-driven archetype registry.
	//
	// Implementations like AggressorAI, PassiveAI, GuardAI are
	// registered here and selected by the creature_template.ai_name
	// DB column. This is distinct from CreatureScript (which is
	// engineer-authored C++ for boss fights, etc.).
	//
	// Status: registry defined; concrete implementations TBD.
	// ============================================================

	class BuiltInAI : public ScriptObject
	{
	public:
		using ScriptObject::ScriptObject;

		virtual std::unique_ptr<CreatureAI> CreateAI(IMapContext& map, IEntity& mob,
		                                              const CreatureTemplate* tmpl) const = 0;
	};

	// Convenience alias — one registry instance for all built-in archetypes.
	inline ScriptRegistry<BuiltInAI>& BuiltInAIRegistry()
	{
		return ScriptRegistry<BuiltInAI>::Instance();
	}

} // namespace MMO
