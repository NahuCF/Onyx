#pragma once

#include "../../../Shared/Source/Scripting/ScriptObject.h"
#include "IEntity.h"
#include "IMapContext.h"

namespace MMO {

	// ============================================================
	// QUEST SCRIPT — singleton callback for quest lifecycle hooks.
	//
	// Status: registered; callbacks TBD when quest system is built.
	// ============================================================

	class QuestScript : public ScriptObject
	{
	public:
		using ScriptObject::ScriptObject;

		virtual void OnAccept(IMapContext& map, IEntity& player, uint32_t questId) {}
		virtual void OnComplete(IMapContext& map, IEntity& player, uint32_t questId) {}
		virtual void OnAbandon(IMapContext& map, IEntity& player, uint32_t questId) {}
	};

	void RegisterAllQuestScripts();

} // namespace MMO
