#include "ScriptedAI.h"
#include <iostream>

namespace MMO {

// ============================================================
// EVENT HANDLERS
// ============================================================

void ScriptedAI::OnEnterCombat(Entity* target) {
    m_InCombat = true;
    m_CombatTime = 0.0f;
    m_Events.Reset();
    m_Events.SetPhase(m_Template->initialPhase);
    m_CurrentPhase = m_Template->initialPhase;
    m_Summons.Clear();
    m_TriggeredPhases.clear();

    // Schedule all abilities
    for (size_t i = 0; i < m_Template->abilities.size(); i++) {
        const auto& ability = m_Template->abilities[i];
        float delay = ability.initialDelay > 0
            ? ability.initialDelay
            : ability.cooldown * 0.3f;
        m_Events.ScheduleEvent(static_cast<uint32_t>(i), delay, ability.phaseMask);
    }
}

void ScriptedAI::OnEvade() {
    m_InCombat = false;
    m_Events.Reset();
    m_Summons.Clear();
    m_TriggeredPhases.clear();
}

void ScriptedAI::OnDeath(Entity* killer) {
    m_InCombat = false;
    m_Events.Reset();
}

void ScriptedAI::OnDamageTaken(Entity* attacker, int32_t damage) {
    // Default: check phase triggers based on HP
    // Can be overridden for custom behavior
}

void ScriptedAI::OnSummonDied(Entity* summon) {
    m_Summons.Remove(summon->GetId());

    // Check if we should transition phase due to all summons dead
    for (const auto& trigger : m_Template->phaseTriggers) {
        if (trigger.type == PhaseTriggerType::SUMMONS_DEAD &&
            m_CurrentPhase == trigger.fromPhase &&
            m_Summons.IsEmpty()) {
            TransitionToPhase(trigger.toPhase, trigger);
        }
    }
}

void ScriptedAI::OnPhaseTransition(uint32_t oldPhase, uint32_t newPhase) {
    // Override for custom phase transition behavior
    std::cout << "[AI] " << m_Owner->GetName() << " transitioned from phase "
              << oldPhase << " to phase " << newPhase << std::endl;
}

// ============================================================
// MAIN UPDATE
// ============================================================

void ScriptedAI::Update(float dt, Entity* target,
                        CastAbilityFn castAbility,
                        SummonCreatureFn summonCreature,
                        DespawnCreatureFn despawnCreature) {
    if (!m_InCombat || !target) return;

    m_CombatTime += dt;
    m_Events.Update(dt);

    // Store callbacks for use in phase transitions
    m_CastAbility = castAbility;
    m_SummonCreature = summonCreature;
    m_DespawnCreature = despawnCreature;

    // Check phase triggers
    CheckPhaseTriggers();

    // Execute scheduled abilities
    while (uint32_t rawEventId = m_Events.ExecuteEvent()) {
        uint32_t eventId = rawEventId - 1;
        if (eventId >= m_Template->abilities.size()) continue;

        const auto& rule = m_Template->abilities[eventId];

        // Check conditions
        bool hasSummons = !m_Summons.IsEmpty();
        if (ConditionEvaluator::EvaluateAll(rule.conditions, m_Owner, target,
                                             m_CombatTime, hasSummons)) {
            castAbility(m_Owner->GetId(), target->GetId(), rule.ability);
        }

        // Reschedule
        m_Events.ScheduleEvent(eventId, rule.cooldown, rule.phaseMask);
    }
}

// ============================================================
// PHASE MANAGEMENT
// ============================================================

void ScriptedAI::SetPhase(uint32_t phase) {
    uint32_t oldPhase = m_CurrentPhase;
    m_CurrentPhase = phase;
    m_Events.SetPhase(phase);
    OnPhaseTransition(oldPhase, phase);
}

void ScriptedAI::CheckPhaseTriggers() {
    for (const auto& trigger : m_Template->phaseTriggers) {
        // Skip if not in source phase
        if (m_CurrentPhase != trigger.fromPhase) continue;

        // Skip if already triggered this phase transition
        uint64_t triggerKey = (static_cast<uint64_t>(trigger.fromPhase) << 32) | trigger.toPhase;
        if (m_TriggeredPhases.count(triggerKey)) continue;

        bool shouldTransition = false;

        switch (trigger.type) {
            case PhaseTriggerType::HEALTH_BELOW: {
                auto health = m_Owner->GetHealth();
                if (health && health->Percent() * 100.0f < trigger.value) {
                    shouldTransition = true;
                }
                break;
            }
            case PhaseTriggerType::HEALTH_ABOVE: {
                auto health = m_Owner->GetHealth();
                if (health && health->Percent() * 100.0f > trigger.value) {
                    shouldTransition = true;
                }
                break;
            }
            case PhaseTriggerType::COMBAT_TIME:
                if (m_CombatTime >= trigger.value) {
                    shouldTransition = true;
                }
                break;
            case PhaseTriggerType::SUMMONS_DEAD:
                if (m_Summons.IsEmpty()) {
                    shouldTransition = true;
                }
                break;
            case PhaseTriggerType::MANUAL:
                // Only triggered by script
                break;
        }

        if (shouldTransition) {
            m_TriggeredPhases.insert(triggerKey);
            TransitionToPhase(trigger.toPhase, trigger);
        }
    }
}

void ScriptedAI::TransitionToPhase(uint32_t newPhase, const PhaseTrigger& trigger) {
    // Execute transition actions
    if (trigger.castOnTransition != AbilityId::NONE && m_CastAbility) {
        m_CastAbility(m_Owner->GetId(), m_Owner->GetId(), trigger.castOnTransition);
    }

    // Summon creatures
    if (trigger.summonCreatureId > 0 && trigger.summonCount > 0 && m_SummonCreature) {
        auto movement = m_Owner->GetMovement();
        if (movement) {
            Vec2 basePos = movement->position;
            float totalWidth = (trigger.summonCount - 1) * trigger.summonSpacing;
            float startX = basePos.x - totalWidth / 2.0f;

            for (uint8_t i = 0; i < trigger.summonCount; i++) {
                Vec2 spawnPos = {
                    startX + i * trigger.summonSpacing,
                    basePos.y + 2.0f  // Slightly in front
                };
                Entity* summon = m_SummonCreature(trigger.summonCreatureId, spawnPos, m_Owner->GetId());
                if (summon) {
                    m_Summons.Add(summon->GetId());
                }
            }
        }
    }

    // Change phase
    SetPhase(newPhase);
}

void ScriptedAI::TriggerPhase(uint32_t newPhase) {
    // Find the trigger for this transition (if any)
    for (const auto& trigger : m_Template->phaseTriggers) {
        if (trigger.fromPhase == m_CurrentPhase && trigger.toPhase == newPhase) {
            TransitionToPhase(newPhase, trigger);
            return;
        }
    }
    // No trigger found, just change phase
    SetPhase(newPhase);
}

} // namespace MMO
