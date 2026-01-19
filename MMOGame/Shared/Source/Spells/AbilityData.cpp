#include "AbilityData.h"

namespace MMO {

AbilityData AbilityData::GetAbilityData(AbilityId id) {
    AbilityData data;
    data.id = id;

    switch (id) {
        // ============================================================
        // WARRIOR ABILITIES
        // ============================================================

        case AbilityId::WARRIOR_SLASH:
            data.name = "Slash";
            data.cooldown = 1.5f;
            data.castTime = 0.0f;
            data.range = 2.0f;
            data.manaCost = 0;
            data.effects = {
                SpellEffect::Damage(25, DamageType::PHYSICAL)
            };
            break;

        case AbilityId::WARRIOR_SHIELD_BASH:
            data.name = "Shield Bash";
            data.cooldown = 8.0f;
            data.castTime = 0.0f;
            data.range = 2.0f;
            data.manaCost = 0;
            data.effects = {
                SpellEffect::Damage(15, DamageType::PHYSICAL),
                SpellEffect::Slow(50, 2.0f)  // 50% slow for 2 seconds
            };
            break;

        case AbilityId::WARRIOR_CHARGE:
            data.name = "Charge";
            data.cooldown = 12.0f;
            data.castTime = 0.0f;
            data.range = 15.0f;
            data.manaCost = 0;
            data.effects = {
                SpellEffect::Damage(30, DamageType::PHYSICAL)
            };
            break;

        // ============================================================
        // WITCH ABILITIES
        // ============================================================

        case AbilityId::WITCH_FIREBALL:
            data.name = "Fireball";
            data.cooldown = 2.0f;
            data.castTime = 1.5f;
            data.range = 25.0f;
            data.manaCost = 20;
            data.effects = {
                SpellEffect::ProjectileDamage(50, DamageType::FIRE, 15.0f)
            };
            break;

        case AbilityId::WITCH_HEAL:
            data.name = "Heal";
            data.cooldown = 4.0f;
            data.castTime = 2.0f;
            data.range = 30.0f;
            data.manaCost = 30;
            data.effects = {
                SpellEffect::Heal(40)
            };
            break;

        case AbilityId::WITCH_FROST_BOLT:
            data.name = "Frost Bolt";
            data.cooldown = 3.0f;
            data.castTime = 1.0f;
            data.range = 25.0f;
            data.manaCost = 15;
            data.effects = {
                SpellEffect::ProjectileDamage(35, DamageType::FROST, 15.0f),
                SpellEffect::Slow(40, 3.0f)  // 40% slow for 3 seconds
            };
            break;

        // ============================================================
        // MOB ABILITIES
        // ============================================================

        case AbilityId::MOB_BASIC_ATTACK:
            data.name = "Attack";
            data.cooldown = 2.0f;
            data.castTime = 0.0f;
            data.range = 2.0f;
            data.manaCost = 0;
            data.effects = {
                SpellEffect::Damage(15, DamageType::PHYSICAL)
            };
            break;

        case AbilityId::WEREWOLF_CLAW:
            data.name = "Claw";
            data.cooldown = 2.5f;
            data.castTime = 0.0f;
            data.range = 2.5f;
            data.manaCost = 0;
            data.effects = {
                SpellEffect::Damage(20, DamageType::PHYSICAL)
            };
            break;

        case AbilityId::WEREWOLF_HOWL:
            data.name = "Howl";
            data.cooldown = 15.0f;
            data.castTime = 0.5f;
            data.range = 0.0f;
            data.manaCost = 0;
            // Howl could apply a buff - for now no effect
            data.effects = {};
            break;

        default:
            data.name = "Unknown";
            data.cooldown = 0.0f;
            data.castTime = 0.0f;
            data.range = 0.0f;
            data.manaCost = 0;
            data.effects = {};
            break;
    }

    return data;
}

} // namespace MMO
