#pragma once

#include "AIDefines.h"
#include <string>
#include <vector>

namespace MMO {

	class CreatureScript;

	// ============================================================
	// CREATURE TEMPLATE
	// ============================================================

	struct CreatureTemplate
	{
		uint32_t id = 0;
		std::string name;
		uint8_t level = 1;
		int32_t maxHealth = 100;
		int32_t maxMana = 0;
		float speed = 3.5f;
		float aggroRadius = 8.0f;
		float leashRadius = 20.0f;
		uint32_t baseXP = 50;
		uint32_t minMoney = 0;
		uint32_t maxMoney = 0;
		CreatureRank rank = CreatureRank::NORMAL;
		float corpseDecayTime = 0.0f;
		float respawnTime = 0.0f;

		// Phase-aware abilities
		std::vector<AbilityRule> abilities;
		std::vector<PhaseTrigger> phaseTriggers;
		uint32_t initialPhase = 1;

		// Loot
		std::vector<LootTableEntry> lootTable;

		// AI selection (DB columns: scripts > ai_name > default)
		// scriptName maps to the 'scripts' DB column: hand-coded CreatureScript.
		// aiName maps to the 'ai_name' DB column: data-driven BuiltInAI archetype.
		std::string scriptName;
		std::string aiName;

		// Resolved at boot from ScriptRegistry<CreatureScript> — no lookup per spawn.
		CreatureScript* resolvedScript = nullptr;

		float GetCorpseDecayTime() const
		{
			return corpseDecayTime > 0 ? corpseDecayTime : GetDefaultCorpseDecayTime(rank);
		}

		float GetRespawnTime() const
		{
			return respawnTime > 0 ? respawnTime : GetDefaultRespawnTime(rank);
		}
	};

} // namespace MMO
