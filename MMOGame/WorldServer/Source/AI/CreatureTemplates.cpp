#include "CreatureTemplates.h"

namespace MMO {
namespace CreatureTemplates {

// ============================================================
// CREATURE TEMPLATES (Data) - Simple mobs
// ============================================================

// Werewolf
CreatureTemplate CreateWerewolf() {
    CreatureTemplate t;
    t.id = 1;
    t.name = "Werewolf";
    t.level = 2;
    t.maxHealth = 80;
    t.speed = 4.0f;
    t.aggroRadius = 8.0f;
    t.leashRadius = 20.0f;
    t.baseXP = 75;
    t.minMoney = 15;
    t.maxMoney = 45;

    // Abilities
    t.abilities.push_back(
        AbilityRule(AbilityId::WEREWOLF_CLAW, 2.5f)
            .WithCondition(ConditionType::IN_RANGE, 2.5f)
    );
    t.abilities.push_back(
        AbilityRule(AbilityId::WEREWOLF_HOWL, 15.0f)
            .WithCondition(ConditionType::HEALTH_BELOW, 50.0f)
    );

    // Loot
    t.lootTable = {
        { 1001, 30.0f, 1, 1 },  // Rusty Sword
        { 2001, 25.0f, 1, 1 },  // Cloth Hood
        { 2101, 20.0f, 1, 1 },  // Cloth Robe
        { 3001, 15.0f, 1, 1 },  // Copper Ring
        { 5001, 2.0f,  1, 1 },  // Shadowfang (rare)
    };

    return t;
}

// Forest Spider
CreatureTemplate CreateForestSpider() {
    CreatureTemplate t;
    t.id = 2;
    t.name = "Forest Spider";
    t.level = 1;
    t.maxHealth = 50;
    t.speed = 3.5f;
    t.aggroRadius = 6.0f;
    t.leashRadius = 15.0f;
    t.baseXP = 40;
    t.minMoney = 8;
    t.maxMoney = 25;

    // Abilities
    t.abilities.push_back(
        AbilityRule(AbilityId::MOB_BASIC_ATTACK, 2.0f)
            .WithCondition(ConditionType::IN_RANGE, 2.0f)
    );

    // Loot
    t.lootTable = {
        { 2301, 35.0f, 1, 1 },  // Cloth Shoes
        { 2401, 30.0f, 1, 1 },  // Cloth Gloves
        { 3101, 20.0f, 1, 1 },  // Silver Pendant
    };

    return t;
}

// Shadow Add (summoned by Shadow Lord)
CreatureTemplate CreateShadowAdd() {
    CreatureTemplate t;
    t.id = 100;
    t.name = "Shadow Servant";
    t.level = 3;
    t.maxHealth = 40;
    t.speed = 3.0f;
    t.aggroRadius = 15.0f;  // Large aggro - always joins boss fight
    t.leashRadius = 50.0f;
    t.baseXP = 20;
    t.rank = CreatureRank::NORMAL;

    // Simple attack
    t.abilities.push_back(
        AbilityRule(AbilityId::MOB_BASIC_ATTACK, 2.0f)
            .WithCondition(ConditionType::IN_RANGE, 2.0f)
    );

    return t;
}

// Shadow Lord Boss
CreatureTemplate CreateShadowLord() {
    CreatureTemplate t;
    t.id = 50;
    t.name = "Shadow Lord";
    t.level = 5;
    t.maxHealth = 500;
    t.maxMana = 200;
    t.speed = 3.0f;
    t.aggroRadius = 12.0f;
    t.leashRadius = 30.0f;
    t.baseXP = 500;
    t.minMoney = 100;
    t.maxMoney = 250;
    t.rank = CreatureRank::BOSS;
    t.initialPhase = 1;
    t.scriptName = "shadow_lord";  // Uses custom script

    // Phase 1 abilities (normal combat)
    t.abilities.push_back(
        AbilityRule(AbilityId::MOB_BASIC_ATTACK, 3.0f, PHASE(1))
            .WithCondition(ConditionType::IN_RANGE, 8.0f)
    );

    // Phase 2 abilities (invulnerable, weaker attacks)
    t.abilities.push_back(
        AbilityRule(AbilityId::MOB_BASIC_ATTACK, 5.0f, PHASE(2))
            .WithCondition(ConditionType::IN_RANGE, 8.0f)
    );

    // Phase 3 abilities (enraged, fast attacks)
    t.abilities.push_back(
        AbilityRule(AbilityId::MOB_BASIC_ATTACK, 1.5f, PHASE(3))
            .WithCondition(ConditionType::IN_RANGE, 8.0f)
    );

    // Phase triggers
    // Phase 1 -> 2: At 50% HP, summon adds and become invulnerable
    t.phaseTriggers.push_back(
        PhaseTrigger(1, 2, PhaseTriggerType::HEALTH_BELOW, 50.0f)
            .WithSummon(100, 3, 3.0f)  // Summon 3 Shadow Adds
    );

    // Phase 2 -> 3: When all adds dead, become vulnerable and enrage
    t.phaseTriggers.push_back(
        PhaseTrigger(2, 3, PhaseTriggerType::SUMMONS_DEAD, 0.0f)
    );

    // Loot
    t.lootTable = {
        { 5001, 50.0f, 1, 1 },  // Shadowfang
        { 3001, 75.0f, 1, 1 },  // Copper Ring
    };

    return t;
}

// Template registry
std::unordered_map<uint32_t, CreatureTemplate>& GetTemplateRegistry() {
    static std::unordered_map<uint32_t, CreatureTemplate> registry;
    static bool initialized = false;

    if (!initialized) {
        auto werewolf = CreateWerewolf();
        auto spider = CreateForestSpider();
        auto shadowAdd = CreateShadowAdd();
        auto shadowLord = CreateShadowLord();

        registry[werewolf.id] = std::move(werewolf);
        registry[spider.id] = std::move(spider);
        registry[shadowAdd.id] = std::move(shadowAdd);
        registry[shadowLord.id] = std::move(shadowLord);

        initialized = true;
    }

    return registry;
}

const CreatureTemplate* GetTemplate(uint32_t id) {
    auto& registry = GetTemplateRegistry();
    auto it = registry.find(id);
    return it != registry.end() ? &it->second : nullptr;
}

} // namespace CreatureTemplates
} // namespace MMO
