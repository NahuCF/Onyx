#pragma once

#include "../AI/ScriptedAI.h"
#include "../AI/ScriptRegistry.h"
#include "../Entity/AuraComponent.h"
#include "../../../Shared/Source/Spells/SpellDefines.h"
#include <iostream>

namespace MMO {

// ============================================================
// SHADOW LORD AI - Example custom boss script
// ============================================================
//
// This demonstrates the AzerothCore approach to boss mechanics:
// - Use AURAS for temporary effects like invulnerability
// - Track summons via SummonList
// - Remove auras when conditions are met (all adds dead)
//
// ============================================================

class ShadowLordAI : public ScriptedAI {
public:
    ShadowLordAI(Entity* owner, const CreatureTemplate* tmpl)
        : ScriptedAI(owner, tmpl) {}

    // Custom events (beyond template abilities)
    enum CustomEvents {
        EVENT_INVULNERABILITY_CHECK = 100,
        EVENT_ENRAGE_WARNING = 101
    };

    void OnEnterCombat(Entity* target) override {
        ScriptedAI::OnEnterCombat(target);

        // Clear any leftover immunity from previous attempts
        RemoveInvulnerabilityAura();

        std::cout << "[ShadowLord] Entering combat!" << std::endl;
    }

    void OnPhaseTransition(uint32_t oldPhase, uint32_t newPhase) override {
        ScriptedAI::OnPhaseTransition(oldPhase, newPhase);

        if (newPhase == 2) {
            // Phase 2: Apply DAMAGE_IMMUNITY aura (AzerothCore style)
            // The aura system handles all immunity checks automatically
            ApplyInvulnerabilityAura();
            std::cout << "[ShadowLord] Phase 2: INVULNERABLE until adds die!" << std::endl;
        }
        else if (newPhase == 3) {
            // Phase 3: Remove invulnerability, enrage
            RemoveInvulnerabilityAura();

            // Could also apply an enrage aura here:
            // ApplyEnrageAura();  // e.g., +50% damage dealt
            std::cout << "[ShadowLord] Phase 3: ENRAGED! Attacks faster!" << std::endl;
        }
    }

    void OnSummonDied(Entity* summon) override {
        std::cout << "[ShadowLord] A Shadow Servant has fallen! ("
                  << m_Summons.Count() - 1 << " remaining)" << std::endl;

        ScriptedAI::OnSummonDied(summon);

        // AzerothCore pattern: Check if all adds are dead
        // If so, remove invulnerability and transition phase
        if (m_Summons.Count() == 0) {
            std::cout << "[ShadowLord] All adds defeated! Removing invulnerability!" << std::endl;
            RemoveInvulnerabilityAura();

            // Could trigger phase 3 here:
            // SetPhase(3);
        }
    }

    void OnEvade() override {
        ScriptedAI::OnEvade();

        // Clean up auras when boss resets
        RemoveInvulnerabilityAura();
    }

    void OnDeath(Entity* killer) override {
        ScriptedAI::OnDeath(killer);

        // Clean up auras on death
        RemoveInvulnerabilityAura();
    }

private:
    // ============================================================
    // AURA HELPERS (AzerothCore style)
    // ============================================================

    // Apply DAMAGE_IMMUNITY aura - boss takes no damage
    void ApplyInvulnerabilityAura() {
        auto auras = m_Owner->GetAuras();
        if (!auras) return;

        // Check if already has immunity
        if (auras->HasAuraType(AuraType::DAMAGE_IMMUNITY)) {
            return;
        }

        // Create immunity aura with very long duration (until manually removed)
        Aura immunityAura;
        immunityAura.sourceAbility = AbilityId::NONE;  // Applied by script, not ability
        immunityAura.casterId = m_Owner->GetId();
        immunityAura.type = AuraType::DAMAGE_IMMUNITY;
        immunityAura.value = 0;
        immunityAura.duration = 600.0f;  // 10 minutes (effectively permanent until removed)
        immunityAura.maxDuration = 600.0f;
        immunityAura.tickInterval = 0.0f;  // No periodic effect

        m_InvulnerabilityAuraId = auras->AddAura(immunityAura);
        std::cout << "[ShadowLord] Applied DAMAGE_IMMUNITY aura (ID: " << m_InvulnerabilityAuraId << ")" << std::endl;
    }

    // Remove DAMAGE_IMMUNITY aura
    void RemoveInvulnerabilityAura() {
        auto auras = m_Owner->GetAuras();
        if (!auras) return;

        if (m_InvulnerabilityAuraId != 0) {
            auras->RemoveAura(m_InvulnerabilityAuraId);
            std::cout << "[ShadowLord] Removed DAMAGE_IMMUNITY aura" << std::endl;
            m_InvulnerabilityAuraId = 0;
        } else {
            // Fallback: remove all immunity auras
            auras->RemoveAurasByType(AuraType::DAMAGE_IMMUNITY);
        }
    }

    uint32_t m_InvulnerabilityAuraId = 0;
};

// Register the Shadow Lord script
REGISTER_CREATURE_SCRIPT("shadow_lord", ShadowLordAI)

} // namespace MMO
