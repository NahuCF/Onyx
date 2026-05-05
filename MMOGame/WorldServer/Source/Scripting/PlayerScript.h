#pragma once

#include "../../../Shared/Source/Scripting/ScriptObject.h"
#include "IEntity.h"
#include "IMapContext.h"

namespace MMO {

	// ============================================================
	// PLAYER SCRIPT — global hook subscriber for player events.
	//
	// Unlike CreatureScript / TriggerScript (one script per data row),
	// PlayerScript uses HookRegistry so MULTIPLE scripts can receive
	// the same event (e.g. an achievements script + a welcome script
	// both hook OnLogin).
	//
	// Registration pattern (in RegisterAllPlayerScripts):
	//   auto* s = ScriptRegistry<PlayerScript>::Instance().Register<WelcomeScript>();
	//   HookRegistry<PlayerScript>::Instance().Subscribe(s);
	//
	// Dispatch (from WorldServer on login):
	//   HookRegistry<PlayerScript>::Instance().Dispatch<&PlayerScript::OnLogin>(*entity);
	//
	// Status: registered; no concrete scripts yet.
	// ============================================================

	class PlayerScript : public ScriptObject
	{
	public:
		using ScriptObject::ScriptObject;

		virtual void OnLogin(IEntity& player) {}
		virtual void OnLogout(IEntity& player) {}
		virtual void OnLevelUp(IEntity& player, uint32_t oldLevel, uint32_t newLevel) {}
		virtual void OnDeath(IEntity& player, IEntity* killer) {}
		virtual void OnQuestComplete(IEntity& player, uint32_t questId) {}
	};

	void RegisterAllPlayerScripts();

} // namespace MMO
