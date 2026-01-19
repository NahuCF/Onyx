#pragma once

#include "../../../Shared/Source/Types/Types.h"
#include "EventMap.h"
#include <vector>
#include <cstdlib>

namespace MMO {

class Entity;

// ============================================================
// CONDITION TYPES
// ============================================================

enum class ConditionType : uint8_t {
    NONE = 0,

    // Self conditions
    HEALTH_BELOW,           // Self HP% < value
    HEALTH_ABOVE,           // Self HP% > value
    MANA_BELOW,             // Self Mana% < value
    MANA_ABOVE,             // Self Mana% > value

    // Target conditions
    TARGET_HEALTH_BELOW,    // Target HP% < value
    TARGET_HEALTH_ABOVE,    // Target HP% > value

    // Range conditions
    IN_RANGE,               // Distance to target <= value
    OUT_OF_RANGE,           // Distance to target > value

    // Combat conditions
    COMBAT_TIME_ABOVE,      // Combat time > value seconds
    COMBAT_TIME_BELOW,      // Combat time < value seconds

    // Summon conditions
    SUMMONS_ALIVE,          // Has living summons
    NO_SUMMONS_ALIVE,       // All summons dead

    // Buff conditions
    HAS_BUFF,               // Self has buff (param = buff ID)
    NOT_HAS_BUFF,           // Self doesn't have buff

    // Random
    RANDOM_CHANCE           // value = percentage (0-100)
};

// ============================================================
// CONDITION
// ============================================================

struct Condition {
    ConditionType type = ConditionType::NONE;
    float value = 0.0f;
    uint32_t param = 0;

    Condition() = default;
    Condition(ConditionType t, float v, uint32_t p = 0) : type(t), value(v), param(p) {}
};

// ============================================================
// ABILITY RULE (Phase-aware)
// ============================================================

struct AbilityRule {
    AbilityId ability = AbilityId::NONE;
    float cooldown = 0.0f;
    float initialDelay = 0.0f;          // Delay before first use (0 = cooldown * 0.3)
    uint32_t phaseMask = PHASE_ALL;     // Which phases this ability is active in
    std::vector<Condition> conditions;

    AbilityRule() = default;
    AbilityRule(AbilityId ab, float cd, uint32_t phase = PHASE_ALL)
        : ability(ab), cooldown(cd), phaseMask(phase) {}

    AbilityRule& WithCondition(ConditionType type, float value, uint32_t param = 0) {
        conditions.push_back(Condition(type, value, param));
        return *this;
    }

    AbilityRule& WithInitialDelay(float delay) {
        initialDelay = delay;
        return *this;
    }
};

// ============================================================
// PHASE TRIGGER TYPES
// ============================================================

enum class PhaseTriggerType : uint8_t {
    HEALTH_BELOW,       // HP% drops below value
    HEALTH_ABOVE,       // HP% rises above value
    COMBAT_TIME,        // Combat time exceeds value
    SUMMONS_DEAD,       // All summoned adds died
    MANUAL              // Script-triggered only
};

// ============================================================
// PHASE TRIGGER
// ============================================================

struct PhaseTrigger {
    uint32_t fromPhase = 1;             // Current phase required
    uint32_t toPhase = 2;               // Phase to transition to
    PhaseTriggerType type = PhaseTriggerType::HEALTH_BELOW;
    float value = 50.0f;                // HP%, time, etc.

    // Actions on transition
    AbilityId castOnTransition = AbilityId::NONE;
    uint32_t summonCreatureId = 0;
    uint8_t summonCount = 0;
    float summonSpacing = 3.0f;         // Distance between summoned adds

    PhaseTrigger() = default;
    PhaseTrigger(uint32_t from, uint32_t to, PhaseTriggerType t, float v)
        : fromPhase(from), toPhase(to), type(t), value(v) {}

    PhaseTrigger& WithCast(AbilityId ability) {
        castOnTransition = ability;
        return *this;
    }

    PhaseTrigger& WithSummon(uint32_t creatureId, uint8_t count, float spacing = 3.0f) {
        summonCreatureId = creatureId;
        summonCount = count;
        summonSpacing = spacing;
        return *this;
    }
};

// ============================================================
// CREATURE RANK
// ============================================================

enum class CreatureRank : uint8_t {
    NORMAL = 0,     // Regular trash mobs
    ELITE = 1,      // Stronger mobs (dungeons, group content)
    RARE = 2,       // Rare spawns with long respawn
    BOSS = 3        // World bosses, raid bosses
};

// Default timers by rank (in seconds)
inline float GetDefaultCorpseDecayTime(CreatureRank rank) {
    switch (rank) {
        case CreatureRank::NORMAL: return 60.0f;      // 1 minute
        case CreatureRank::ELITE:  return 300.0f;     // 5 minutes
        case CreatureRank::RARE:   return 300.0f;     // 5 minutes
        case CreatureRank::BOSS:   return 7200.0f;    // 2 hours
        default: return 60.0f;
    }
}

inline float GetDefaultRespawnTime(CreatureRank rank) {
    switch (rank) {
        case CreatureRank::NORMAL: return 120.0f;     // 2 minutes
        case CreatureRank::ELITE:  return 300.0f;     // 5 minutes
        case CreatureRank::RARE:   return 14400.0f;   // 4 hours
        case CreatureRank::BOSS:   return 86400.0f;   // 24 hours
        default: return 120.0f;
    }
}

// ============================================================
// LOOT TABLE ENTRY
// ============================================================

struct LootTableEntry {
    ItemId itemId;
    float dropChance;           // 0.0 - 100.0 (percentage)
    uint8_t minCount = 1;
    uint8_t maxCount = 1;
};

} // namespace MMO
