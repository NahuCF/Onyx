#pragma once

#include "../../../Shared/Source/Scripting/ScriptObject.h"
#include "../../../Shared/Source/Scripting/ScriptRegistry.h"

namespace MMO {

	class IEntity;
	class IMapContext;
	struct ServerTriggerVolume;

	// ============================================================
	// TRIGGER SCRIPT — singleton callback for trigger volumes.
	//
	// One instance per named script lives in
	// ScriptRegistry<TriggerScript>. Multiple volumes can reference
	// the same script name; dispatch goes through the cached
	// resolvedScript pointer on ServerTriggerVolume (no hash lookup
	// per tick).
	//
	// OnEnter / OnExit fire on edges (always).
	// OnStay fires every movement tick while inside, only when the
	// volume's TriggerEventKind is ON_STAY.
	// ============================================================

	class TriggerScript : public ScriptObject
	{
	public:
		using ScriptObject::ScriptObject;

		virtual void OnEnter(IMapContext& map, IEntity& entity, const ServerTriggerVolume& trigger) {}
		virtual void OnExit(IMapContext& map, IEntity& entity, const ServerTriggerVolume& trigger) {}
		virtual void OnStay(IMapContext& map, IEntity& entity, const ServerTriggerVolume& trigger) {}
	};

	// Called once at WorldServer startup before any MapInstance is created.
	void RegisterAllTriggerScripts();

} // namespace MMO
