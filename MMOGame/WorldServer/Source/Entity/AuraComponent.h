#pragma once

#include "../../../Shared/Source/Spells/SpellDefines.h"
#include <vector>
#include <algorithm>

namespace MMO {

// ============================================================
// AURA COMPONENT - Tracks active auras on an entity
// ============================================================

class AuraComponent {
public:
    // Add a new aura (returns aura ID)
    uint32_t AddAura(const Aura& aura) {
        Aura newAura = aura;
        newAura.id = m_NextAuraId++;
        m_Auras.push_back(newAura);
        m_Dirty = true;
        return newAura.id;
    }

    // Remove aura by ID
    bool RemoveAura(uint32_t auraId) {
        auto it = std::find_if(m_Auras.begin(), m_Auras.end(),
            [auraId](const Aura& a) { return a.id == auraId; });
        if (it != m_Auras.end()) {
            m_Auras.erase(it);
            m_Dirty = true;
            return true;
        }
        return false;
    }

    // Remove all auras of a specific type
    void RemoveAurasByType(AuraType type) {
        auto newEnd = std::remove_if(m_Auras.begin(), m_Auras.end(),
            [type](const Aura& a) { return a.type == type; });
        if (newEnd != m_Auras.end()) {
            m_Auras.erase(newEnd, m_Auras.end());
            m_Dirty = true;
        }
    }

    // Remove all auras from a specific ability
    void RemoveAurasByAbility(AbilityId ability) {
        auto newEnd = std::remove_if(m_Auras.begin(), m_Auras.end(),
            [ability](const Aura& a) { return a.sourceAbility == ability; });
        if (newEnd != m_Auras.end()) {
            m_Auras.erase(newEnd, m_Auras.end());
            m_Dirty = true;
        }
    }

    // Remove all auras from a specific caster
    void RemoveAurasByCaster(EntityId caster) {
        auto newEnd = std::remove_if(m_Auras.begin(), m_Auras.end(),
            [caster](const Aura& a) { return a.casterId == caster; });
        if (newEnd != m_Auras.end()) {
            m_Auras.erase(newEnd, m_Auras.end());
            m_Dirty = true;
        }
    }

    // Clear all auras
    void ClearAllAuras() {
        if (!m_Auras.empty()) {
            m_Auras.clear();
            m_Dirty = true;
        }
    }

    // Check if entity has a specific aura type
    bool HasAuraType(AuraType type) const {
        return std::any_of(m_Auras.begin(), m_Auras.end(),
            [type](const Aura& a) { return a.type == type; });
    }

    // Check if entity is immune to damage
    bool IsImmune() const {
        return HasAuraType(AuraType::DAMAGE_IMMUNITY);
    }

    // Check if entity is immune to a damage type
    bool IsImmuneToSchool(DamageType type) const {
        for (const auto& aura : m_Auras) {
            if (aura.type == AuraType::DAMAGE_IMMUNITY) return true;
            if (aura.type == AuraType::SCHOOL_IMMUNITY &&
                aura.damageType == type) return true;
        }
        return false;
    }

    // Get speed modifier (multiplicative)
    float GetSpeedModifier() const {
        float modifier = 1.0f;
        for (const auto& aura : m_Auras) {
            if (aura.type == AuraType::MOD_SPEED_PCT) {
                modifier *= (100.0f + aura.value) / 100.0f;
            }
        }
        return std::max(0.0f, modifier);  // Can't go negative
    }

    // Get damage taken modifier
    float GetDamageTakenModifier() const {
        float modifier = 1.0f;
        for (const auto& aura : m_Auras) {
            if (aura.type == AuraType::MOD_DAMAGE_TAKEN_PCT) {
                modifier *= (100.0f + aura.value) / 100.0f;
            }
        }
        return std::max(0.0f, modifier);
    }

    // Check for CC
    bool IsStunned() const { return HasAuraType(AuraType::STUN); }
    bool IsRooted() const { return HasAuraType(AuraType::ROOT); }
    bool IsSilenced() const { return HasAuraType(AuraType::SILENCE); }

    // Get all auras (for iteration)
    std::vector<Aura>& GetAuras() { return m_Auras; }
    const std::vector<Aura>& GetAuras() const { return m_Auras; }

    // Dirty flag for network sync
    bool IsDirty() const { return m_Dirty; }
    void ClearDirty() { m_Dirty = false; }

private:
    std::vector<Aura> m_Auras;
    uint32_t m_NextAuraId = 1;
    bool m_Dirty = false;
};

} // namespace MMO
