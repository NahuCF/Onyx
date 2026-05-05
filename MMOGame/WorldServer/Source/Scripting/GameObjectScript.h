#pragma once

#include "../../../Shared/Source/Scripting/ScriptObject.h"
#include "IEntity.h"
#include "IMapContext.h"

namespace MMO {

	// ============================================================
	// GAME OBJECT SCRIPT — singleton callback for GO interactions.
	//
	// Status: registered; callbacks TBD when GO system is built.
	// ============================================================

	class GameObjectScript : public ScriptObject
	{
	public:
		using ScriptObject::ScriptObject;

		virtual void OnActivate(IMapContext& map, IEntity& activator) {}
		virtual void OnDeactivate(IMapContext& map) {}
	};

	void RegisterAllGameObjectScripts();

} // namespace MMO
