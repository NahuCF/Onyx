#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cmath>

namespace MMO {

// ============================================================
// ENTITY AND NETWORK IDS
// ============================================================

using EntityId = uint32_t;
using AccountId = uint64_t;
using CharacterId = uint64_t;
using ItemId = uint32_t;
using ItemInstanceId = uint64_t;

constexpr EntityId INVALID_ENTITY_ID = 0;
constexpr ItemId INVALID_ITEM_ID = 0;
constexpr ItemInstanceId INVALID_ITEM_INSTANCE_ID = 0;

// ============================================================
// MATH TYPES
// ============================================================

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }

    float Length() const { return std::sqrt(x * x + y * y); }
    float LengthSquared() const { return x * x + y * y; }

    Vec2 Normalized() const {
        float len = Length();
        if (len > 0.0001f) {
            return Vec2(x / len, y / len);
        }
        return Vec2(0, 0);
    }

    static float Distance(const Vec2& a, const Vec2& b) {
        return (a - b).Length();
    }

    static float DistanceSquared(const Vec2& a, const Vec2& b) {
        return (a - b).LengthSquared();
    }
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }

    Vec3 Normalized() const {
        float len = Length();
        if (len > 0.0001f) {
            return Vec3(x / len, y / len, z / len);
        }
        return Vec3(0, 0, 0);
    }

    static float Distance(const Vec3& a, const Vec3& b) {
        return (a - b).Length();
    }
};

// ============================================================
// CURRENCY (stored as copper: 1g = 100s, 1s = 100c)
// ============================================================

struct Currency {
    uint32_t IsCopper = 0;

    Currency() = default;
    Currency(uint32_t copper) : IsCopper(copper) {}

    static Currency FromCopper(uint32_t copper) { return Currency(copper); }
    static Currency FromGold(uint32_t g, uint32_t s = 0, uint32_t c = 0) {
        return Currency(g * 10000 + s * 100 + c);
    }

    uint32_t GetGold() const { return IsCopper / 10000; }
    uint32_t GetSilver() const { return (IsCopper % 10000) / 100; }
    uint32_t GetCopper() const { return IsCopper % 100; }

    void Add(uint32_t copper) { IsCopper += copper; }
    bool Remove(uint32_t copper) {
        if (IsCopper < copper) return false;
        IsCopper -= copper;
        return true;
    }

    std::string ToString() const {
        std::string result;
        uint32_t g = GetGold(), s = GetSilver(), c = GetCopper();
        if (g > 0) result += std::to_string(g) + "g ";
        if (s > 0) result += std::to_string(s) + "s ";
        if (c > 0 || result.empty()) result += std::to_string(c) + "c";
        return result;
    }
};

// ============================================================
// ENUMS
// ============================================================

enum class CharacterClass : uint8_t {
    NONE = 0,
    WARRIOR = 1,
    WITCH = 2
};

enum class MoveState : uint8_t {
    IDLE = 0,
    WALKING = 1,
    RUNNING = 2,
    DEAD = 3
};

enum class EntityType : uint8_t {
    PLAYER = 0,
    MOB = 1,
    NPC = 2,
    BOSS = 3
};

enum class AbilityId : uint16_t {
    NONE = 0,

    // Warrior abilities
    WARRIOR_SLASH = 100,
    WARRIOR_SHIELD_BASH = 101,
    WARRIOR_CHARGE = 102,

    // Witch abilities
    WITCH_FIREBALL = 200,
    WITCH_HEAL = 201,
    WITCH_FROST_BOLT = 202,

    // Mob abilities
    MOB_BASIC_ATTACK = 1000,
    WEREWOLF_CLAW = 1001,
    WEREWOLF_HOWL = 1002
};

enum class DamageType : uint8_t {
    PHYSICAL = 0,
    FIRE = 1,
    FROST = 2,
    HOLY = 3
};

// ============================================================
// EQUIPMENT SLOTS (14 slots)
// ============================================================

enum class EquipmentSlot : uint8_t {
    HEAD = 0,
    NECK = 1,
    SHOULDERS = 2,
    BACK = 3,
    CHEST = 4,
    WRISTS = 5,
    HANDS = 6,
    WAIST = 7,
    LEGS = 8,
    FEET = 9,
    RING1 = 10,
    RING2 = 11,
    WEAPON = 12,
    OFFHAND = 13,

    COUNT = 14,
    NONE = 255
};

inline const char* GetEquipmentSlotName(EquipmentSlot slot) {
    switch (slot) {
        case EquipmentSlot::HEAD:      return "Head";
        case EquipmentSlot::NECK:      return "Neck";
        case EquipmentSlot::SHOULDERS: return "Shoulders";
        case EquipmentSlot::BACK:      return "Back";
        case EquipmentSlot::CHEST:     return "Chest";
        case EquipmentSlot::WRISTS:    return "Wrists";
        case EquipmentSlot::HANDS:     return "Hands";
        case EquipmentSlot::WAIST:     return "Waist";
        case EquipmentSlot::LEGS:      return "Legs";
        case EquipmentSlot::FEET:      return "Feet";
        case EquipmentSlot::RING1:     return "Ring 1";
        case EquipmentSlot::RING2:     return "Ring 2";
        case EquipmentSlot::WEAPON:    return "Weapon";
        case EquipmentSlot::OFFHAND:   return "Offhand";
        default:                       return "Unknown";
    }
}

// ============================================================
// STAT TYPES (5 primary stats)
// ============================================================

enum class StatType : uint8_t {
    STRENGTH = 0,   // Increases physical damage
    AGILITY = 1,    // Increases crit chance, dodge
    STAMINA = 2,    // Increases health
    INTELLECT = 3,  // Increases mana, spell power
    SPIRIT = 4,     // Increases mana/health regen

    COUNT = 5
};

inline const char* GetStatName(StatType stat) {
    switch (stat) {
        case StatType::STRENGTH:  return "Strength";
        case StatType::AGILITY:   return "Agility";
        case StatType::STAMINA:   return "Stamina";
        case StatType::INTELLECT: return "Intellect";
        case StatType::SPIRIT:    return "Spirit";
        default:                  return "Unknown";
    }
}

// ============================================================
// MODIFIER TYPES (for stat calculation order)
// ============================================================

enum class ModifierType : uint8_t {
    BASE_VALUE = 0,   // Flat bonus added to base (gear)
    BASE_PCT = 1,     // Percentage of base (talents)
    TOTAL_VALUE = 2,  // Flat bonus at end (buffs)
    TOTAL_PCT = 3,    // Percentage of total (raid buffs)

    COUNT = 4
};

// ============================================================
// ITEM TYPES
// ============================================================

enum class ItemType : uint8_t {
    NONE = 0,
    ARMOR = 1,
    WEAPON = 2,
    ACCESSORY = 3,  // Rings, Necks
    CONSUMABLE = 4,
    MATERIAL = 5,
    QUEST = 6
};

enum class ArmorType : uint8_t {
    NONE = 0,
    CLOTH = 1,
    LEATHER = 2,
    MAIL = 3,
    PLATE = 4
};

enum class WeaponType : uint8_t {
    NONE = 0,
    SWORD = 1,
    AXE = 2,
    MACE = 3,
    DAGGER = 4,
    STAFF = 5,
    WAND = 6,
    SHIELD = 7
};

enum class ItemQuality : uint8_t {
    POOR = 0,       // Gray
    COMMON = 1,     // White
    UNCOMMON = 2,   // Green
    RARE = 3,       // Blue
    EPIC = 4,       // Purple
    LEGENDARY = 5   // Orange
};

// Inventory constants
constexpr uint8_t INVENTORY_SIZE = 20;
constexpr uint8_t EQUIPMENT_SLOT_COUNT = static_cast<uint8_t>(EquipmentSlot::COUNT);

// ============================================================
// CHARACTER INFO (for character selection)
// ============================================================

struct CharacterInfo {
    CharacterId id;
    std::string name;
    CharacterClass characterClass;
    uint32_t level;
    std::string zoneName;
    uint64_t lastPlayed; // Unix timestamp
};

} // namespace MMO
