#pragma once

#include "AIDefines.h"
#include <vector>
#include <string>

namespace MMO {

// ============================================================
// CREATURE TEMPLATE
// ============================================================

struct CreatureTemplate {
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
    float corpseDecayTime = 0.0f;   // 0 = use rank default
    float respawnTime = 0.0f;       // 0 = use rank default

    // Phase-aware abilities
    std::vector<AbilityRule> abilities;
    std::vector<PhaseTrigger> phaseTriggers;
    uint32_t initialPhase = 1;

    // Loot
    std::vector<LootTableEntry> lootTable;

    // Custom script (empty = use default AI)
    std::string scriptName;

    // Get effective corpse decay time (template override or rank default)
    float GetCorpseDecayTime() const {
        return corpseDecayTime > 0 ? corpseDecayTime : GetDefaultCorpseDecayTime(rank);
    }

    // Get effective respawn time (template override or rank default)
    float GetRespawnTime() const {
        return respawnTime > 0 ? respawnTime : GetDefaultRespawnTime(rank);
    }
};

} // namespace MMO
