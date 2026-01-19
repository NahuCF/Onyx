#include "Items.h"
#include <iostream>

namespace MMO {

void ItemTemplateManager::Initialize() {
    std::cout << "[ItemTemplateManager] Initializing item templates..." << std::endl;

    // ============================================================
    // WEAPONS
    // ============================================================

    // Starter Sword (ID: 1001)
    {
        ItemTemplate item;
        item.id = 1001;
        item.name = "Rusty Sword";
        item.description = "A worn blade, but still sharp enough.";
        item.type = ItemType::WEAPON;
        item.quality = ItemQuality::POOR;
        item.equipSlot = EquipmentSlot::WEAPON;
        item.weaponType = WeaponType::SWORD;
        item.weaponDamageMin = 5.0f;
        item.weaponDamageMax = 10.0f;
        item.weaponSpeed = 2.0f;
        item.sellPrice = 25;
        item.iconName = "sword_rusty";
        AddTemplate(item);
    }

    // Iron Sword (ID: 1002)
    {
        ItemTemplate item;
        item.id = 1002;
        item.name = "Iron Sword";
        item.description = "A reliable iron blade.";
        item.type = ItemType::WEAPON;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::WEAPON;
        item.weaponType = WeaponType::SWORD;
        item.weaponDamageMin = 12.0f;
        item.weaponDamageMax = 20.0f;
        item.weaponSpeed = 2.2f;
        item.stats[0] = {StatType::STRENGTH, 5};
        item.statCount = 1;
        item.sellPrice = 150;
        item.requiredLevel = 3;
        item.iconName = "sword_iron";
        AddTemplate(item);
    }

    // Blade of the Wolf (ID: 1003)
    {
        ItemTemplate item;
        item.id = 1003;
        item.name = "Blade of the Wolf";
        item.description = "Crafted from the fangs of dire wolves.";
        item.type = ItemType::WEAPON;
        item.quality = ItemQuality::UNCOMMON;
        item.equipSlot = EquipmentSlot::WEAPON;
        item.weaponType = WeaponType::SWORD;
        item.weaponDamageMin = 18.0f;
        item.weaponDamageMax = 30.0f;
        item.weaponSpeed = 2.4f;
        item.stats[0] = {StatType::STRENGTH, 8};
        item.stats[1] = {StatType::AGILITY, 4};
        item.statCount = 2;
        item.sellPrice = 500;
        item.requiredLevel = 5;
        item.iconName = "sword_wolf";
        AddTemplate(item);
    }

    // Staff of Flames (ID: 1010)
    {
        ItemTemplate item;
        item.id = 1010;
        item.name = "Staff of Flames";
        item.description = "A staff imbued with fire magic.";
        item.type = ItemType::WEAPON;
        item.quality = ItemQuality::UNCOMMON;
        item.equipSlot = EquipmentSlot::WEAPON;
        item.weaponType = WeaponType::STAFF;
        item.weaponDamageMin = 10.0f;
        item.weaponDamageMax = 18.0f;
        item.weaponSpeed = 2.8f;
        item.stats[0] = {StatType::INTELLECT, 10};
        item.stats[1] = {StatType::SPIRIT, 5};
        item.statCount = 2;
        item.sellPrice = 450;
        item.requiredLevel = 5;
        item.iconName = "staff_fire";
        AddTemplate(item);
    }

    // Wooden Shield (ID: 1020)
    {
        ItemTemplate item;
        item.id = 1020;
        item.name = "Wooden Shield";
        item.description = "A simple wooden buckler.";
        item.type = ItemType::WEAPON;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::OFFHAND;
        item.weaponType = WeaponType::SHIELD;
        item.armor = 20;
        item.stats[0] = {StatType::STAMINA, 3};
        item.statCount = 1;
        item.sellPrice = 75;
        item.iconName = "shield_wood";
        AddTemplate(item);
    }

    // ============================================================
    // ARMOR - HEAD
    // ============================================================

    // Cloth Hood (ID: 2001)
    {
        ItemTemplate item;
        item.id = 2001;
        item.name = "Cloth Hood";
        item.description = "A simple cloth hood.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::HEAD;
        item.armorType = ArmorType::CLOTH;
        item.armor = 8;
        item.stats[0] = {StatType::INTELLECT, 3};
        item.statCount = 1;
        item.sellPrice = 50;
        item.iconName = "helm_cloth";
        AddTemplate(item);
    }

    // Iron Helm (ID: 2002)
    {
        ItemTemplate item;
        item.id = 2002;
        item.name = "Iron Helm";
        item.description = "A sturdy iron helmet.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::HEAD;
        item.armorType = ArmorType::PLATE;
        item.armor = 25;
        item.stats[0] = {StatType::STAMINA, 5};
        item.statCount = 1;
        item.sellPrice = 100;
        item.requiredLevel = 3;
        item.iconName = "helm_iron";
        AddTemplate(item);
    }

    // Wolf Skull Helm (ID: 2003)
    {
        ItemTemplate item;
        item.id = 2003;
        item.name = "Wolf Skull Helm";
        item.description = "A fearsome helm made from a wolf skull.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::UNCOMMON;
        item.equipSlot = EquipmentSlot::HEAD;
        item.armorType = ArmorType::LEATHER;
        item.armor = 18;
        item.stats[0] = {StatType::STRENGTH, 4};
        item.stats[1] = {StatType::AGILITY, 4};
        item.statCount = 2;
        item.sellPrice = 200;
        item.requiredLevel = 5;
        item.iconName = "helm_wolf";
        AddTemplate(item);
    }

    // ============================================================
    // ARMOR - CHEST
    // ============================================================

    // Cloth Robe (ID: 2101)
    {
        ItemTemplate item;
        item.id = 2101;
        item.name = "Cloth Robe";
        item.description = "A simple robe for spellcasters.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::CHEST;
        item.armorType = ArmorType::CLOTH;
        item.armor = 15;
        item.stats[0] = {StatType::INTELLECT, 5};
        item.stats[1] = {StatType::SPIRIT, 3};
        item.statCount = 2;
        item.sellPrice = 75;
        item.iconName = "chest_cloth";
        AddTemplate(item);
    }

    // Iron Breastplate (ID: 2102)
    {
        ItemTemplate item;
        item.id = 2102;
        item.name = "Iron Breastplate";
        item.description = "Heavy iron armor for the front lines.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::CHEST;
        item.armorType = ArmorType::PLATE;
        item.armor = 45;
        item.stats[0] = {StatType::STAMINA, 7};
        item.stats[1] = {StatType::STRENGTH, 3};
        item.statCount = 2;
        item.sellPrice = 200;
        item.requiredLevel = 3;
        item.iconName = "chest_plate";
        AddTemplate(item);
    }

    // ============================================================
    // ARMOR - LEGS
    // ============================================================

    // Cloth Pants (ID: 2201)
    {
        ItemTemplate item;
        item.id = 2201;
        item.name = "Cloth Pants";
        item.description = "Simple cloth leggings.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::LEGS;
        item.armorType = ArmorType::CLOTH;
        item.armor = 12;
        item.stats[0] = {StatType::INTELLECT, 4};
        item.statCount = 1;
        item.sellPrice = 60;
        item.iconName = "legs_cloth";
        AddTemplate(item);
    }

    // Iron Legguards (ID: 2202)
    {
        ItemTemplate item;
        item.id = 2202;
        item.name = "Iron Legguards";
        item.description = "Sturdy iron leg protection.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::LEGS;
        item.armorType = ArmorType::PLATE;
        item.armor = 35;
        item.stats[0] = {StatType::STAMINA, 6};
        item.statCount = 1;
        item.sellPrice = 150;
        item.requiredLevel = 3;
        item.iconName = "legs_plate";
        AddTemplate(item);
    }

    // ============================================================
    // ARMOR - FEET
    // ============================================================

    // Cloth Shoes (ID: 2301)
    {
        ItemTemplate item;
        item.id = 2301;
        item.name = "Cloth Shoes";
        item.description = "Light cloth footwear.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::FEET;
        item.armorType = ArmorType::CLOTH;
        item.armor = 6;
        item.stats[0] = {StatType::SPIRIT, 3};
        item.statCount = 1;
        item.sellPrice = 40;
        item.iconName = "feet_cloth";
        AddTemplate(item);
    }

    // Iron Boots (ID: 2302)
    {
        ItemTemplate item;
        item.id = 2302;
        item.name = "Iron Boots";
        item.description = "Heavy iron boots.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::FEET;
        item.armorType = ArmorType::PLATE;
        item.armor = 20;
        item.stats[0] = {StatType::STAMINA, 4};
        item.statCount = 1;
        item.sellPrice = 90;
        item.requiredLevel = 3;
        item.iconName = "feet_plate";
        AddTemplate(item);
    }

    // ============================================================
    // ARMOR - HANDS
    // ============================================================

    // Cloth Gloves (ID: 2401)
    {
        ItemTemplate item;
        item.id = 2401;
        item.name = "Cloth Gloves";
        item.description = "Simple cloth gloves.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::HANDS;
        item.armorType = ArmorType::CLOTH;
        item.armor = 5;
        item.stats[0] = {StatType::INTELLECT, 2};
        item.statCount = 1;
        item.sellPrice = 30;
        item.iconName = "hands_cloth";
        AddTemplate(item);
    }

    // Iron Gauntlets (ID: 2402)
    {
        ItemTemplate item;
        item.id = 2402;
        item.name = "Iron Gauntlets";
        item.description = "Heavy iron gauntlets.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::HANDS;
        item.armorType = ArmorType::PLATE;
        item.armor = 18;
        item.stats[0] = {StatType::STRENGTH, 3};
        item.statCount = 1;
        item.sellPrice = 80;
        item.requiredLevel = 3;
        item.iconName = "hands_plate";
        AddTemplate(item);
    }

    // ============================================================
    // ACCESSORIES - RINGS
    // ============================================================

    // Copper Ring (ID: 3001)
    {
        ItemTemplate item;
        item.id = 3001;
        item.name = "Copper Ring";
        item.description = "A simple copper ring.";
        item.type = ItemType::ACCESSORY;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::RING1;  // Can also go in RING2
        item.stats[0] = {StatType::STAMINA, 2};
        item.statCount = 1;
        item.sellPrice = 25;
        item.iconName = "ring_copper";
        AddTemplate(item);
    }

    // Silver Ring of Power (ID: 3002)
    {
        ItemTemplate item;
        item.id = 3002;
        item.name = "Silver Ring of Power";
        item.description = "A silver ring pulsing with energy.";
        item.type = ItemType::ACCESSORY;
        item.quality = ItemQuality::UNCOMMON;
        item.equipSlot = EquipmentSlot::RING1;
        item.stats[0] = {StatType::STRENGTH, 4};
        item.stats[1] = {StatType::STAMINA, 2};
        item.statCount = 2;
        item.sellPrice = 150;
        item.requiredLevel = 5;
        item.iconName = "ring_silver";
        AddTemplate(item);
    }

    // ============================================================
    // ACCESSORIES - NECK
    // ============================================================

    // Leather Necklace (ID: 3101)
    {
        ItemTemplate item;
        item.id = 3101;
        item.name = "Leather Necklace";
        item.description = "A simple leather cord with a pendant.";
        item.type = ItemType::ACCESSORY;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::NECK;
        item.stats[0] = {StatType::SPIRIT, 3};
        item.statCount = 1;
        item.sellPrice = 35;
        item.iconName = "neck_leather";
        AddTemplate(item);
    }

    // ============================================================
    // ACCESSORIES - BACK (CLOAK)
    // ============================================================

    // Traveler's Cloak (ID: 3201)
    {
        ItemTemplate item;
        item.id = 3201;
        item.name = "Traveler's Cloak";
        item.description = "A worn but warm cloak.";
        item.type = ItemType::ACCESSORY;
        item.quality = ItemQuality::COMMON;
        item.equipSlot = EquipmentSlot::BACK;
        item.armor = 5;
        item.stats[0] = {StatType::AGILITY, 2};
        item.statCount = 1;
        item.sellPrice = 45;
        item.iconName = "back_cloak";
        AddTemplate(item);
    }

    // ============================================================
    // RARE/EPIC ITEMS
    // ============================================================

    // Shadowfang (ID: 5001) - Rare Sword
    {
        ItemTemplate item;
        item.id = 5001;
        item.name = "Shadowfang";
        item.description = "A blade forged in darkness.";
        item.type = ItemType::WEAPON;
        item.quality = ItemQuality::RARE;
        item.equipSlot = EquipmentSlot::WEAPON;
        item.weaponType = WeaponType::SWORD;
        item.weaponDamageMin = 28.0f;
        item.weaponDamageMax = 45.0f;
        item.weaponSpeed = 2.6f;
        item.stats[0] = {StatType::STRENGTH, 12};
        item.stats[1] = {StatType::AGILITY, 8};
        item.stats[2] = {StatType::STAMINA, 5};
        item.statCount = 3;
        item.sellPrice = 2500;
        item.requiredLevel = 10;
        item.iconName = "sword_shadow";
        AddTemplate(item);
    }

    // Crown of the Fallen (ID: 5002) - Epic Helm
    {
        ItemTemplate item;
        item.id = 5002;
        item.name = "Crown of the Fallen";
        item.description = "A crown worn by a forgotten king.";
        item.type = ItemType::ARMOR;
        item.quality = ItemQuality::EPIC;
        item.equipSlot = EquipmentSlot::HEAD;
        item.armorType = ArmorType::PLATE;
        item.armor = 55;
        item.stats[0] = {StatType::STRENGTH, 10};
        item.stats[1] = {StatType::STAMINA, 15};
        item.stats[2] = {StatType::AGILITY, 5};
        item.statCount = 3;
        item.sellPrice = 5000;
        item.requiredLevel = 15;
        item.iconName = "helm_crown";
        AddTemplate(item);
    }

    std::cout << "[ItemTemplateManager] Loaded " << m_Templates.size() << " item templates" << std::endl;
}

} // namespace MMO
