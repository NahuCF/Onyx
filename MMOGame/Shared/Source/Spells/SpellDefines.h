#pragma once

#include "../Types/Types.h"
#include <cstdint>
#include <vector>

namespace MMO {

// ============================================================
// SPELL EFFECT TYPES
// ============================================================

enum class SpellEffectType : uint8_t {
    NONE = 0,
    DIRECT_DAMAGE,          // Instant damage
    DIRECT_HEAL,            // Instant heal
    APPLY_AURA,             // Apply a buff/debuff
    REMOVE_AURA,            // Remove a specific aura
    SPAWN_PROJECTILE,       // Create a projectile (damage/heal on hit)
};

// ============================================================
// AURA TYPES
// ============================================================

enum class AuraType : uint8_t {
    NONE = 0,

    // Movement
    MOD_SPEED_PCT,              // Modify speed by percent (-50 = 50% slow)

    // Damage modification
    DAMAGE_IMMUNITY,            // Immune to all damage
    SCHOOL_IMMUNITY,            // Immune to specific damage type
    MOD_DAMAGE_TAKEN_PCT,       // Modify damage taken by percent

    // Periodic effects
    PERIODIC_DAMAGE,            // Damage over time (DoT)
    PERIODIC_HEAL,              // Heal over time (HoT)
    PERIODIC_MANA,              // Mana regen over time

    // Stats
    MOD_STAT,                   // Modify a stat (strength, etc.)
    MOD_HEALTH_PCT,             // Modify max health by percent
    MOD_MANA_PCT,               // Modify max mana by percent

    // Combat
    MOD_ATTACK_POWER,           // Modify attack power
    MOD_SPELL_POWER,            // Modify spell power

    // CC
    STUN,                       // Cannot move or act
    ROOT,                       // Cannot move, can act
    SILENCE,                    // Cannot cast spells
};

// ============================================================
// SPELL EFFECT
// ============================================================

struct SpellEffect {
    SpellEffectType type = SpellEffectType::NONE;

    // For DIRECT_DAMAGE, DIRECT_HEAL, PERIODIC_DAMAGE, PERIODIC_HEAL
    int32_t value = 0;
    DamageType damageType = DamageType::PHYSICAL;

    // For APPLY_AURA
    AuraType auraType = AuraType::NONE;
    float auraDuration = 0.0f;          // Seconds (0 = permanent until removed)
    float auraTickInterval = 0.0f;      // For periodic effects
    int32_t auraValue = 0;              // Aura-specific value (slow %, damage per tick, etc.)

    // For SPAWN_PROJECTILE
    float projectileSpeed = 15.0f;

    // Helpers for building effects
    static SpellEffect Damage(int32_t amount, DamageType type = DamageType::PHYSICAL) {
        SpellEffect e;
        e.type = SpellEffectType::DIRECT_DAMAGE;
        e.value = amount;
        e.damageType = type;
        return e;
    }

    static SpellEffect Heal(int32_t amount) {
        SpellEffect e;
        e.type = SpellEffectType::DIRECT_HEAL;
        e.value = amount;
        return e;
    }

    static SpellEffect ProjectileDamage(int32_t amount, DamageType type = DamageType::PHYSICAL, float speed = 15.0f) {
        SpellEffect e;
        e.type = SpellEffectType::SPAWN_PROJECTILE;
        e.value = amount;
        e.damageType = type;
        e.projectileSpeed = speed;
        return e;
    }

    static SpellEffect ProjectileHeal(int32_t amount, float speed = 15.0f) {
        SpellEffect e;
        e.type = SpellEffectType::SPAWN_PROJECTILE;
        e.value = amount;
        e.auraValue = 1;  // Flag for heal projectile
        e.projectileSpeed = speed;
        return e;
    }

    static SpellEffect ApplyAura(AuraType aura, int32_t value, float duration, float tickInterval = 0.0f) {
        SpellEffect e;
        e.type = SpellEffectType::APPLY_AURA;
        e.auraType = aura;
        e.auraValue = value;
        e.auraDuration = duration;
        e.auraTickInterval = tickInterval;
        return e;
    }

    static SpellEffect Slow(int32_t percent, float duration) {
        return ApplyAura(AuraType::MOD_SPEED_PCT, -percent, duration);
    }

    static SpellEffect DamageOverTime(int32_t damagePerTick, float duration, float tickInterval, DamageType type = DamageType::PHYSICAL) {
        SpellEffect e;
        e.type = SpellEffectType::APPLY_AURA;
        e.auraType = AuraType::PERIODIC_DAMAGE;
        e.auraValue = damagePerTick;
        e.auraDuration = duration;
        e.auraTickInterval = tickInterval;
        e.damageType = type;
        return e;
    }

    static SpellEffect HealOverTime(int32_t healPerTick, float duration, float tickInterval) {
        SpellEffect e;
        e.type = SpellEffectType::APPLY_AURA;
        e.auraType = AuraType::PERIODIC_HEAL;
        e.auraValue = healPerTick;
        e.auraDuration = duration;
        e.auraTickInterval = tickInterval;
        return e;
    }

    static SpellEffect Immunity(float duration = 0.0f) {
        return ApplyAura(AuraType::DAMAGE_IMMUNITY, 0, duration);
    }
};

// ============================================================
// AURA INSTANCE (Active aura on an entity)
// ============================================================

struct Aura {
    uint32_t id = 0;                    // Unique aura instance ID
    AbilityId sourceAbility = AbilityId::NONE;  // What ability applied this
    EntityId casterId = INVALID_ENTITY_ID;      // Who applied this

    AuraType type = AuraType::NONE;
    int32_t value = 0;                  // Effect value (slow %, damage per tick, etc.)
    DamageType damageType = DamageType::PHYSICAL;

    float duration = 0.0f;              // Remaining duration (0 = permanent)
    float maxDuration = 0.0f;           // Original duration
    float tickInterval = 0.0f;          // For periodic effects
    float tickTimer = 0.0f;             // Time until next tick

    bool IsExpired() const { return maxDuration > 0 && duration <= 0; }
    bool IsPermanent() const { return maxDuration <= 0; }
    bool IsPeriodic() const { return tickInterval > 0; }
};

} // namespace MMO
