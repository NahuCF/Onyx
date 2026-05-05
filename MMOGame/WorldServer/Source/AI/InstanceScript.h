#pragma once

#include "../../../Shared/Source/Scripting/ScriptObject.h"
#include "../Scripting/IMapContext.h"
#include "../Scripting/IEntity.h"
#include "../Map/MapDefines.h"
#include <memory>

namespace MMO {

	class InstanceState;

	// ============================================================
	// INSTANCE SCRIPT — registered factory singleton.
	//
	// One instance lives in ScriptRegistry<InstanceScript> keyed
	// by map_template.instance_script. MapInstance calls CreateState()
	// at construction and owns the returned InstanceState.
	// ============================================================

	class InstanceScript : public ScriptObject
	{
	public:
		using ScriptObject::ScriptObject;

		virtual std::unique_ptr<InstanceState> CreateState(IMapContext& map) const = 0;
	};

	// ============================================================
	// INSTANCE STATE — per-MapInstance runtime encounter object.
	//
	// NOT a ScriptObject. Created by InstanceScript::CreateState().
	// Tracks boss kill order, door states, lockout data, etc.
	// MapInstance forwards relevant events to it.
	// ============================================================

	class InstanceState
	{
	public:
		virtual ~InstanceState() = default;

		// Lifecycle
		virtual void OnInstanceCreate(IMapContext& map) {}
		virtual void OnInstanceDestroy() {}

		// Player events
		virtual void OnPlayerEnter(IEntity& player) {}
		virtual void OnPlayerLeave(IEntity& player) {}

		// Creature events
		virtual void OnCreatureCreate(IEntity& creature) {}
		virtual void OnCreatureDeath(IEntity& creature, IEntity* killer) {}

		// Trigger event (forwarded from MapInstance::FireTriggerScript)
		virtual void OnAreaTrigger(IEntity& entity, const ServerTriggerVolume& volume) {}

		// Tick
		virtual void Update(float dt) {}
	};

	// Called once at WorldServer startup.
	void RegisterAllInstanceScripts();

} // namespace MMO
