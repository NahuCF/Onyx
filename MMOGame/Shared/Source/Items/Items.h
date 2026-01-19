#pragma once

#include "../Types/Types.h"
#include <string>
#include <array>
#include <unordered_map>
#include <memory>

namespace MMO {

// ============================================================
// ITEM STAT BONUS
// ============================================================

struct ItemStatBonus {
    StatType stat = StatType::STRENGTH;
    int32_t value = 0;
};

// ============================================================
// ITEM TEMPLATE (Static definition - loaded once)
// ============================================================

struct ItemTemplate {
    ItemId id = 0;
    std::string name;
    std::string description;

    ItemType type = ItemType::NONE;
    ItemQuality quality = ItemQuality::COMMON;
    EquipmentSlot equipSlot = EquipmentSlot::NONE;  // Where it can be equipped

    // Subtype info
    ArmorType armorType = ArmorType::NONE;
    WeaponType weaponType = WeaponType::NONE;

    // Stats (up to 5 stat bonuses per item)
    static constexpr int MAX_STATS = 5;
    ItemStatBonus stats[MAX_STATS];
    uint8_t statCount = 0;

    // Weapon-specific
    float weaponDamageMin = 0.0f;
    float weaponDamageMax = 0.0f;
    float weaponSpeed = 2.0f;  // Attacks per second

    // Armor-specific
    int32_t armor = 0;

    // General
    uint32_t maxStack = 1;
    uint32_t sellPrice = 0;   // In copper
    uint32_t buyPrice = 0;    // In copper
    uint8_t requiredLevel = 1;

    // Icon/visual (client-side)
    std::string iconName;

    // Helper methods
    bool IsEquippable() const { return equipSlot != EquipmentSlot::NONE; }
    bool IsWeapon() const { return type == ItemType::WEAPON; }
    bool IsArmor() const { return type == ItemType::ARMOR; }
    bool IsStackable() const { return maxStack > 1; }

    // Get total stat value for a specific stat
    int32_t GetStatValue(StatType stat) const {
        int32_t total = 0;
        for (uint8_t i = 0; i < statCount; i++) {
            if (stats[i].stat == stat) {
                total += stats[i].value;
            }
        }
        return total;
    }
};

// ============================================================
// ITEM INSTANCE (Per-player item with unique state)
// ============================================================

struct ItemInstance {
    ItemInstanceId instanceId = INVALID_ITEM_INSTANCE_ID;
    ItemId templateId = INVALID_ITEM_ID;
    uint32_t stackCount = 1;

    // Future: durability, enchants, etc.
    // int32_t durability = 0;
    // int32_t maxDurability = 0;

    bool IsValid() const { return templateId != INVALID_ITEM_ID; }
    void Clear() {
        instanceId = INVALID_ITEM_INSTANCE_ID;
        templateId = INVALID_ITEM_ID;
        stackCount = 1;
    }
};

// ============================================================
// INVENTORY SLOT
// ============================================================

struct InventorySlot {
    ItemInstance item;

    bool IsEmpty() const { return !item.IsValid(); }
    void Clear() { item.Clear(); }
};

// ============================================================
// ITEM TEMPLATE MANAGER (Singleton)
// ============================================================

class ItemTemplateManager {
public:
    static ItemTemplateManager& Instance() {
        static ItemTemplateManager instance;
        return instance;
    }

    void Initialize();

    const ItemTemplate* GetTemplate(ItemId id) const {
        auto it = m_Templates.find(id);
        return it != m_Templates.end() ? &it->second : nullptr;
    }

    const std::unordered_map<ItemId, ItemTemplate>& GetAllTemplates() const {
        return m_Templates;
    }

private:
    ItemTemplateManager() = default;
    void AddTemplate(const ItemTemplate& tmpl) { m_Templates[tmpl.id] = tmpl; }

    std::unordered_map<ItemId, ItemTemplate> m_Templates;
};

// ============================================================
// ITEM QUALITY COLORS
// ============================================================

inline uint32_t GetQualityColor(ItemQuality quality) {
    switch (quality) {
        case ItemQuality::POOR:      return 0xFF9D9D9D;  // Gray
        case ItemQuality::COMMON:    return 0xFFFFFFFF;  // White
        case ItemQuality::UNCOMMON:  return 0xFF1EFF00;  // Green
        case ItemQuality::RARE:      return 0xFF0070DD;  // Blue
        case ItemQuality::EPIC:      return 0xFFA335EE;  // Purple
        case ItemQuality::LEGENDARY: return 0xFFFF8000;  // Orange
        default:                     return 0xFFFFFFFF;
    }
}

inline const char* GetQualityName(ItemQuality quality) {
    switch (quality) {
        case ItemQuality::POOR:      return "Poor";
        case ItemQuality::COMMON:    return "Common";
        case ItemQuality::UNCOMMON:  return "Uncommon";
        case ItemQuality::RARE:      return "Rare";
        case ItemQuality::EPIC:      return "Epic";
        case ItemQuality::LEGENDARY: return "Legendary";
        default:                     return "Unknown";
    }
}

} // namespace MMO
