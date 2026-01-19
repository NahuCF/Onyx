#pragma once

#include "../Types/Types.h"
#include "SpellDefines.h"
#include <string>
#include <vector>

namespace MMO {

// ============================================================
// ABILITY DATA - Complete definition of an ability/spell
// ============================================================

struct AbilityData {
    AbilityId id = AbilityId::NONE;
    std::string name;
    float cooldown = 0.0f;       // seconds
    float castTime = 0.0f;       // 0 for instant
    float range = 0.0f;
    int32_t manaCost = 0;

    // Spell effects (the new system)
    std::vector<SpellEffect> effects;

    // Helper to check if ability has a specific effect type
    bool HasEffect(SpellEffectType type) const {
        for (const auto& effect : effects) {
            if (effect.type == type) return true;
        }
        return false;
    }

    // Helper to get first effect of a type
    const SpellEffect* GetEffect(SpellEffectType type) const {
        for (const auto& effect : effects) {
            if (effect.type == type) return &effect;
        }
        return nullptr;
    }

    // Check if this is a projectile ability
    bool IsProjectile() const {
        return HasEffect(SpellEffectType::SPAWN_PROJECTILE);
    }

    // Check if this is a healing ability
    bool IsHeal() const {
        for (const auto& effect : effects) {
            if (effect.type == SpellEffectType::DIRECT_HEAL) return true;
            if (effect.type == SpellEffectType::SPAWN_PROJECTILE && effect.auraValue == 1) return true;
            if (effect.type == SpellEffectType::APPLY_AURA &&
                effect.auraType == AuraType::PERIODIC_HEAL) return true;
        }
        return false;
    }

    // Get total direct damage (for display/tooltips)
    int32_t GetDirectDamage() const {
        int32_t total = 0;
        for (const auto& effect : effects) {
            if (effect.type == SpellEffectType::DIRECT_DAMAGE ||
                effect.type == SpellEffectType::SPAWN_PROJECTILE) {
                total += effect.value;
            }
        }
        return total;
    }

    // Get direct heal amount
    int32_t GetDirectHeal() const {
        int32_t total = 0;
        for (const auto& effect : effects) {
            if (effect.type == SpellEffectType::DIRECT_HEAL) {
                total += effect.value;
            }
            if (effect.type == SpellEffectType::SPAWN_PROJECTILE && effect.auraValue == 1) {
                total += effect.value;
            }
        }
        return total;
    }

    // Static factory for getting ability data
    static AbilityData GetAbilityData(AbilityId id);
};

} // namespace MMO
