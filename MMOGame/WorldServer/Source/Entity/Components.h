#pragma once

#include "../../../Shared/Source/Types/Types.h"
#include "../../../Shared/Source/Items/Items.h"
#include <unordered_map>
#include <array>
#include <cmath>

namespace MMO {

// ============================================================
// HEALTH COMPONENT
// ============================================================

struct HealthComponent {
    int32_t current = 100;
    int32_t max = 100;
    int32_t baseMax = 100;  // Base max before stat bonuses

    bool IsDead() const { return current <= 0; }
    float Percent() const { return max > 0 ? static_cast<float>(current) / max : 0.0f; }

    void TakeDamage(int32_t amount) {
        current -= amount;
        if (current < 0) current = 0;
    }

    void Heal(int32_t amount) {
        current += amount;
        if (current > max) current = max;
    }
};

// ============================================================
// MANA COMPONENT
// ============================================================

struct ManaComponent {
    int32_t current = 100;
    int32_t max = 100;
    int32_t baseMax = 100;  // Base max before stat bonuses

    bool HasMana(int32_t amount) const { return current >= amount; }
    float Percent() const { return max > 0 ? static_cast<float>(current) / max : 0.0f; }
    void UseMana(int32_t amount) { current -= amount; if (current < 0) current = 0; }
    void RestoreMana(int32_t amount) { current += amount; if (current > max) current = max; }
};

// ============================================================
// MOVEMENT COMPONENT
// ============================================================

struct MovementComponent {
    Vec2 position;
    Vec2 velocity;
    float rotation = 0.0f;
    float speed = 5.0f;
    MoveState moveState = MoveState::IDLE;
    // Note: Speed modifiers are now handled via AuraComponent
};

// ============================================================
// COMBAT COMPONENT
// ============================================================

struct CombatComponent {
    EntityId targetId = INVALID_ENTITY_ID;
    float attackRange = 2.0f;
    std::unordered_map<AbilityId, float> cooldowns;
    AbilityId currentCast = AbilityId::NONE;
    float castTimer = 0.0f;
    float castDuration = 0.0f;
    EntityId castTargetId = INVALID_ENTITY_ID;
    Vec2 castTargetPosition;

    // Combat state tracking for regeneration
    float lastCombatTime = 0.0f;        // World time of last combat action
    float lastManaUseTime = 0.0f;       // For "5-second rule" mana regen
    static constexpr float COMBAT_TIMEOUT = 5.0f;      // Seconds out of combat before regen
    static constexpr float MANA_REGEN_DELAY = 5.0f;    // "5-second rule" for mana

    bool IsCasting() const { return currentCast != AbilityId::NONE; }
    bool IsAbilityReady(AbilityId id) const {
        auto it = cooldowns.find(id);
        return it == cooldowns.end() || it->second <= 0.0f;
    }

    bool IsInCombat(float currentTime) const {
        return (currentTime - lastCombatTime) < COMBAT_TIMEOUT;
    }

    bool CanRegenMana(float currentTime) const {
        return (currentTime - lastManaUseTime) >= MANA_REGEN_DELAY;
    }

    void MarkCombat(float currentTime) {
        lastCombatTime = currentTime;
    }

    void MarkManaUse(float currentTime) {
        lastManaUseTime = currentTime;
    }
};

// ============================================================
// WALLET COMPONENT
// ============================================================

struct WalletComponent {
    uint32_t IsCopper = 0;

    void AddMoney(uint32_t copper) { IsCopper += copper; }
    bool RemoveMoney(uint32_t copper) {
        if (IsCopper < copper) return false;
        IsCopper -= copper;
        return true;
    }
};

// ============================================================
// EXPERIENCE COMPONENT
// ============================================================

struct ExperienceComponent {
    uint32_t current = 0;       // Current XP
    uint32_t total = 0;         // Lifetime total XP earned

    // WoW-like XP formula: increases per level
    static uint32_t GetXPForLevel(uint8_t level) {
        if (level < 2) return 400;
        // Formula: 400 + 100 * level^1.5 (simplified WoW-like curve)
        return 400 + static_cast<uint32_t>(100.0f * std::pow(level, 1.5f));
    }

    // Calculate XP modifier based on level difference (WoW-style)
    // Grey = 0%, Green = 80%, Yellow = 100%, Orange = 115%, Red = 130%
    static float GetLevelModifier(int8_t playerLevel, int8_t mobLevel) {
        int diff = mobLevel - playerLevel;

        // Grey (too low level)
        if (diff <= -6) return 0.0f;

        // Green (easy)
        if (diff <= -3) return 0.8f;

        // Yellow (even)
        if (diff <= 2) return 1.0f;

        // Orange (challenging)
        if (diff <= 4) return 1.15f;

        // Red (very hard)
        return 1.3f;
    }
};

// ============================================================
// AGGRO COMPONENT
// ============================================================

struct AggroComponent {
    std::unordered_map<EntityId, float> threatTable;
    float aggroRadius = 10.0f;
    float leashRadius = 20.0f;
    Vec2 homePosition;
    bool isEvading = false;  // When true, mob is returning home and won't acquire new targets

    EntityId GetTopThreat() const {
        EntityId top = INVALID_ENTITY_ID;
        float maxThreat = 0.0f;
        for (const auto& [id, threat] : threatTable) {
            if (threat > maxThreat) {
                maxThreat = threat;
                top = id;
            }
        }
        return top;
    }

    void AddThreat(EntityId target, float amount) {
        threatTable[target] += amount;
    }

    void RemoveThreat(EntityId target) {
        threatTable.erase(target);
    }

    void ClearThreat() {
        threatTable.clear();
    }
};

// ============================================================
// INVENTORY COMPONENT
// ============================================================

struct InventoryComponent {
    std::array<InventorySlot, INVENTORY_SIZE> slots;

    bool AddItem(const ItemInstance& item, uint8_t* outSlot = nullptr);
    bool RemoveItem(uint8_t slot);
    bool SwapSlots(uint8_t slotA, uint8_t slotB);
    ItemInstance* GetItem(uint8_t slot);
    const ItemInstance* GetItem(uint8_t slot) const;
    int FindFirstEmptySlot() const;
    int FindItemByTemplateId(ItemId templateId) const;
    bool IsFull() const;
    int GetItemCount() const;
};

// ============================================================
// EQUIPMENT COMPONENT
// ============================================================

struct EquipmentComponent {
    std::array<ItemInstance, EQUIPMENT_SLOT_COUNT> slots;

    bool Equip(EquipmentSlot slot, const ItemInstance& item);
    bool Unequip(EquipmentSlot slot);
    ItemInstance* GetEquipped(EquipmentSlot slot);
    const ItemInstance* GetEquipped(EquipmentSlot slot) const;
    bool IsSlotEmpty(EquipmentSlot slot) const;

    // Check if an item can go in a slot (ring can go in either ring slot)
    bool CanEquipInSlot(EquipmentSlot slot, const ItemTemplate* tmpl) const;
};

// ============================================================
// STATS COMPONENT
// ============================================================

struct StatsComponent {
    // Base stats (from level/class)
    std::array<float, static_cast<size_t>(StatType::COUNT)> baseStats = {0};

    // Stat modifiers [stat][modifier_type]
    std::array<std::array<float, static_cast<size_t>(ModifierType::COUNT)>,
               static_cast<size_t>(StatType::COUNT)> modifiers = {{{0}}};

    // Cached final stats (recalculated when gear changes)
    std::array<float, static_cast<size_t>(StatType::COUNT)> finalStats = {0};

    // Derived combat stats
    float weaponDamageMin = 1.0f;
    float weaponDamageMax = 2.0f;
    float weaponSpeed = 2.0f;
    int32_t totalArmor = 0;

    // Apply/remove stat modifier
    void ApplyMod(StatType stat, ModifierType type, float value);
    void RemoveMod(StatType stat, ModifierType type, float value);

    // Recalculate final stats
    void RecalculateStats();

    // Get final stat value
    float GetStat(StatType stat) const { return finalStats[static_cast<size_t>(stat)]; }

    // Calculate bonus health from stamina
    int32_t GetBonusHealth() const { return static_cast<int32_t>(GetStat(StatType::STAMINA) * 10.0f); }

    // Calculate bonus mana from intellect
    int32_t GetBonusMana() const { return static_cast<int32_t>(GetStat(StatType::INTELLECT) * 15.0f); }

    // Calculate attack power from strength
    float GetAttackPower() const { return GetStat(StatType::STRENGTH) * 2.0f; }

    // Calculate spell power from intellect
    float GetSpellPower() const { return GetStat(StatType::INTELLECT) * 1.5f; }

    // Calculate weapon damage with attack power
    float GetMeleeDamageMin() const;
    float GetMeleeDamageMax() const;

    // Calculate damage reduction from armor
    float GetArmorReduction(int32_t attackerLevel) const;
};

} // namespace MMO
