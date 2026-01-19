#pragma once

#include "AIDefines.h"
#include "AICallbacks.h"
#include "EventMap.h"
#include "SummonList.h"
#include "ConditionEvaluator.h"
#include "CreatureTemplate.h"
#include "../Entity/Entity.h"
#include <unordered_set>

namespace MMO {

// Forward declarations
class MapInstance;

// ============================================================
// SCRIPTED AI - Base class for all mob AI
// ============================================================

class ScriptedAI {
public:
    ScriptedAI(Entity* owner, const CreatureTemplate* tmpl)
        : m_Owner(owner), m_Template(tmpl) {}

    virtual ~ScriptedAI() = default;

    // ========================================
    // Event handlers (override in scripts)
    // ========================================

    virtual void OnEnterCombat(Entity* target);
    virtual void OnEvade();
    virtual void OnDeath(Entity* killer);
    virtual void OnDamageTaken(Entity* attacker, int32_t damage);
    virtual void OnSummonDied(Entity* summon);
    virtual void OnPhaseTransition(uint32_t oldPhase, uint32_t newPhase);

    // ========================================
    // Main update
    // ========================================

    virtual void Update(float dt, Entity* target,
                        CastAbilityFn castAbility,
                        SummonCreatureFn summonCreature,
                        DespawnCreatureFn despawnCreature);

    // ========================================
    // Accessors
    // ========================================

    bool IsInCombat() const { return m_InCombat; }
    uint32_t GetCurrentPhase() const { return m_CurrentPhase; }
    float GetCombatTime() const { return m_CombatTime; }
    Entity* GetOwner() { return m_Owner; }
    const CreatureTemplate* GetTemplate() const { return m_Template; }
    SummonList& GetSummons() { return m_Summons; }
    EventMap& GetEvents() { return m_Events; }

protected:
    // ========================================
    // Phase management
    // ========================================

    void SetPhase(uint32_t phase);
    void CheckPhaseTriggers();
    void TransitionToPhase(uint32_t newPhase, const PhaseTrigger& trigger);
    void TriggerPhase(uint32_t newPhase);

    // ========================================
    // Member variables
    // ========================================

    Entity* m_Owner;
    const CreatureTemplate* m_Template;
    EventMap m_Events;
    SummonList m_Summons;

    bool m_InCombat = false;
    float m_CombatTime = 0.0f;
    uint32_t m_CurrentPhase = 1;

    // Track which phase transitions have been triggered (prevent re-triggering)
    std::unordered_set<uint64_t> m_TriggeredPhases;

    // Cached callbacks for use in phase transitions
    CastAbilityFn m_CastAbility;
    SummonCreatureFn m_SummonCreature;
    DespawnCreatureFn m_DespawnCreature;
};

} // namespace MMO
