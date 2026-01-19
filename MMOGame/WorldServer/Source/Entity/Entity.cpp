#include "Entity.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace MMO {

// ============================================================
// INVENTORY COMPONENT
// ============================================================

bool InventoryComponent::AddItem(const ItemInstance& item, uint8_t* outSlot) {
    int slot = FindFirstEmptySlot();
    if (slot < 0) return false;

    slots[slot].item = item;
    if (outSlot) *outSlot = static_cast<uint8_t>(slot);
    return true;
}

bool InventoryComponent::RemoveItem(uint8_t slot) {
    if (slot >= INVENTORY_SIZE) return false;
    slots[slot].Clear();
    return true;
}

bool InventoryComponent::SwapSlots(uint8_t slotA, uint8_t slotB) {
    if (slotA >= INVENTORY_SIZE || slotB >= INVENTORY_SIZE) return false;
    std::swap(slots[slotA], slots[slotB]);
    return true;
}

ItemInstance* InventoryComponent::GetItem(uint8_t slot) {
    if (slot >= INVENTORY_SIZE || slots[slot].IsEmpty()) return nullptr;
    return &slots[slot].item;
}

const ItemInstance* InventoryComponent::GetItem(uint8_t slot) const {
    if (slot >= INVENTORY_SIZE || slots[slot].IsEmpty()) return nullptr;
    return &slots[slot].item;
}

int InventoryComponent::FindFirstEmptySlot() const {
    for (uint8_t i = 0; i < INVENTORY_SIZE; i++) {
        if (slots[i].IsEmpty()) return i;
    }
    return -1;
}

int InventoryComponent::FindItemByTemplateId(ItemId templateId) const {
    for (uint8_t i = 0; i < INVENTORY_SIZE; i++) {
        if (!slots[i].IsEmpty() && slots[i].item.templateId == templateId) {
            return i;
        }
    }
    return -1;
}

bool InventoryComponent::IsFull() const {
    return FindFirstEmptySlot() < 0;
}

int InventoryComponent::GetItemCount() const {
    int count = 0;
    for (const auto& slot : slots) {
        if (!slot.IsEmpty()) count++;
    }
    return count;
}

// ============================================================
// EQUIPMENT COMPONENT
// ============================================================

bool EquipmentComponent::Equip(EquipmentSlot slot, const ItemInstance& item) {
    if (slot == EquipmentSlot::NONE || slot == EquipmentSlot::COUNT) return false;
    slots[static_cast<size_t>(slot)] = item;
    return true;
}

bool EquipmentComponent::Unequip(EquipmentSlot slot) {
    if (slot == EquipmentSlot::NONE || slot == EquipmentSlot::COUNT) return false;
    slots[static_cast<size_t>(slot)].Clear();
    return true;
}

ItemInstance* EquipmentComponent::GetEquipped(EquipmentSlot slot) {
    if (slot == EquipmentSlot::NONE || slot == EquipmentSlot::COUNT) return nullptr;
    auto& item = slots[static_cast<size_t>(slot)];
    return item.IsValid() ? &item : nullptr;
}

const ItemInstance* EquipmentComponent::GetEquipped(EquipmentSlot slot) const {
    if (slot == EquipmentSlot::NONE || slot == EquipmentSlot::COUNT) return nullptr;
    const auto& item = slots[static_cast<size_t>(slot)];
    return item.IsValid() ? &item : nullptr;
}

bool EquipmentComponent::IsSlotEmpty(EquipmentSlot slot) const {
    if (slot == EquipmentSlot::NONE || slot == EquipmentSlot::COUNT) return true;
    return !slots[static_cast<size_t>(slot)].IsValid();
}

bool EquipmentComponent::CanEquipInSlot(EquipmentSlot slot, const ItemTemplate* tmpl) const {
    if (!tmpl || !tmpl->IsEquippable()) return false;

    // Rings can go in either ring slot
    if (tmpl->equipSlot == EquipmentSlot::RING1) {
        return slot == EquipmentSlot::RING1 || slot == EquipmentSlot::RING2;
    }

    return tmpl->equipSlot == slot;
}

// ============================================================
// STATS COMPONENT
// ============================================================

void StatsComponent::ApplyMod(StatType stat, ModifierType type, float value) {
    modifiers[static_cast<size_t>(stat)][static_cast<size_t>(type)] += value;
}

void StatsComponent::RemoveMod(StatType stat, ModifierType type, float value) {
    modifiers[static_cast<size_t>(stat)][static_cast<size_t>(type)] -= value;
}

void StatsComponent::RecalculateStats() {
    for (size_t i = 0; i < static_cast<size_t>(StatType::COUNT); i++) {
        float base = baseStats[i];
        float basePct = 1.0f + modifiers[i][static_cast<size_t>(ModifierType::BASE_PCT)];
        float baseValue = modifiers[i][static_cast<size_t>(ModifierType::BASE_VALUE)];
        float totalValue = modifiers[i][static_cast<size_t>(ModifierType::TOTAL_VALUE)];
        float totalPct = 1.0f + modifiers[i][static_cast<size_t>(ModifierType::TOTAL_PCT)];

        finalStats[i] = ((base * basePct) + baseValue + totalValue) * totalPct;
        if (finalStats[i] < 0) finalStats[i] = 0;
    }
}

float StatsComponent::GetMeleeDamageMin() const {
    // Base weapon damage + attack power contribution
    // Formula: weaponDamage + (AP / 14) * weaponSpeed
    float apBonus = (GetAttackPower() / 14.0f) * weaponSpeed;
    return weaponDamageMin + apBonus;
}

float StatsComponent::GetMeleeDamageMax() const {
    float apBonus = (GetAttackPower() / 14.0f) * weaponSpeed;
    return weaponDamageMax + apBonus;
}

float StatsComponent::GetArmorReduction(int32_t attackerLevel) const {
    // WoW-like armor formula
    // Reduction = Armor / (Armor + 400 + 85 * AttackerLevel)
    if (totalArmor <= 0) return 0.0f;
    float reduction = static_cast<float>(totalArmor) /
                      (static_cast<float>(totalArmor) + 400.0f + 85.0f * attackerLevel);
    return std::min(reduction, 0.75f);  // Cap at 75% reduction
}

Entity::Entity(EntityId id, EntityType type, const std::string& name)
    : m_Id(id), m_Type(type), m_Name(name) {
}

void Entity::AddHealthComponent(int32_t max) {
    m_Health = std::make_unique<HealthComponent>();
    m_Health->baseMax = max;
    m_Health->max = max;
    m_Health->current = max;
}

void Entity::AddManaComponent(int32_t max) {
    m_Mana = std::make_unique<ManaComponent>();
    m_Mana->baseMax = max;
    m_Mana->max = max;
    m_Mana->current = max;
}

void Entity::AddMovementComponent(float speed) {
    m_Movement = std::make_unique<MovementComponent>();
    m_Movement->speed = speed;
}

void Entity::AddCombatComponent(float attackRange) {
    m_Combat = std::make_unique<CombatComponent>();
    m_Combat->attackRange = attackRange;
}

void Entity::AddAggroComponent(float aggroRadius, float leashRadius) {
    m_Aggro = std::make_unique<AggroComponent>();
    m_Aggro->aggroRadius = aggroRadius;
    m_Aggro->leashRadius = leashRadius;
}

void Entity::AddWalletComponent(uint32_t initialMoney) {
    m_Wallet = std::make_unique<WalletComponent>();
    m_Wallet->IsCopper = initialMoney;
}

void Entity::AddInventoryComponent() {
    m_Inventory = std::make_unique<InventoryComponent>();
}

void Entity::AddEquipmentComponent() {
    m_Equipment = std::make_unique<EquipmentComponent>();
}

void Entity::AddStatsComponent() {
    m_Stats = std::make_unique<StatsComponent>();

    // Set base stats based on class and level
    float levelMult = static_cast<float>(m_Level);

    switch (m_Class) {
        case CharacterClass::WARRIOR:
            m_Stats->baseStats[static_cast<size_t>(StatType::STRENGTH)] = 10.0f + levelMult * 2.0f;
            m_Stats->baseStats[static_cast<size_t>(StatType::AGILITY)] = 5.0f + levelMult * 1.0f;
            m_Stats->baseStats[static_cast<size_t>(StatType::STAMINA)] = 10.0f + levelMult * 2.0f;
            m_Stats->baseStats[static_cast<size_t>(StatType::INTELLECT)] = 3.0f + levelMult * 0.5f;
            m_Stats->baseStats[static_cast<size_t>(StatType::SPIRIT)] = 5.0f + levelMult * 0.5f;
            break;

        case CharacterClass::WITCH:
            m_Stats->baseStats[static_cast<size_t>(StatType::STRENGTH)] = 3.0f + levelMult * 0.5f;
            m_Stats->baseStats[static_cast<size_t>(StatType::AGILITY)] = 5.0f + levelMult * 1.0f;
            m_Stats->baseStats[static_cast<size_t>(StatType::STAMINA)] = 5.0f + levelMult * 1.0f;
            m_Stats->baseStats[static_cast<size_t>(StatType::INTELLECT)] = 12.0f + levelMult * 2.5f;
            m_Stats->baseStats[static_cast<size_t>(StatType::SPIRIT)] = 10.0f + levelMult * 1.5f;
            break;

        default:
            for (size_t i = 0; i < static_cast<size_t>(StatType::COUNT); i++) {
                m_Stats->baseStats[i] = 5.0f + levelMult;
            }
            break;
    }

    m_Stats->RecalculateStats();
}

void Entity::AddExperienceComponent(uint32_t currentXP) {
    m_Experience = std::make_unique<ExperienceComponent>();
    m_Experience->current = currentXP;
}

void Entity::AddAuraComponent() {
    m_Auras = std::make_unique<AuraComponent>();
}

uint32_t Entity::GetXPForNextLevel() const {
    return ExperienceComponent::GetXPForLevel(m_Level);
}

uint32_t Entity::GiveXP(uint32_t amount) {
    if (!m_Experience) return 0;

    m_Experience->current += amount;
    m_Experience->total += amount;

    uint32_t levelsGained = 0;
    uint32_t xpNeeded = GetXPForNextLevel();

    // Check for level up (can level up multiple times)
    while (m_Experience->current >= xpNeeded) {
        m_Experience->current -= xpNeeded;
        m_Level++;
        levelsGained++;

        // Update base health/mana for new level
        if (m_Health) {
            int32_t healthPerLevel = (m_Class == CharacterClass::WARRIOR) ? 15 : 8;
            m_Health->baseMax += healthPerLevel;
            m_Health->max += healthPerLevel;
            m_Health->current = m_Health->max;  // Full heal on level up
        }
        if (m_Mana) {
            int32_t manaPerLevel = (m_Class == CharacterClass::WARRIOR) ? 5 : 12;
            m_Mana->baseMax += manaPerLevel;
            m_Mana->max += manaPerLevel;
            m_Mana->current = m_Mana->max;  // Full mana on level up
        }

        // Recalculate stats with new level
        if (m_Stats) {
            float levelMult = static_cast<float>(m_Level);
            switch (m_Class) {
                case CharacterClass::WARRIOR:
                    m_Stats->baseStats[static_cast<size_t>(StatType::STRENGTH)] = 10.0f + levelMult * 2.0f;
                    m_Stats->baseStats[static_cast<size_t>(StatType::AGILITY)] = 5.0f + levelMult * 1.0f;
                    m_Stats->baseStats[static_cast<size_t>(StatType::STAMINA)] = 10.0f + levelMult * 2.0f;
                    m_Stats->baseStats[static_cast<size_t>(StatType::INTELLECT)] = 3.0f + levelMult * 0.5f;
                    m_Stats->baseStats[static_cast<size_t>(StatType::SPIRIT)] = 5.0f + levelMult * 0.5f;
                    break;
                case CharacterClass::WITCH:
                    m_Stats->baseStats[static_cast<size_t>(StatType::STRENGTH)] = 3.0f + levelMult * 0.5f;
                    m_Stats->baseStats[static_cast<size_t>(StatType::AGILITY)] = 5.0f + levelMult * 1.0f;
                    m_Stats->baseStats[static_cast<size_t>(StatType::STAMINA)] = 5.0f + levelMult * 1.0f;
                    m_Stats->baseStats[static_cast<size_t>(StatType::INTELLECT)] = 12.0f + levelMult * 2.5f;
                    m_Stats->baseStats[static_cast<size_t>(StatType::SPIRIT)] = 10.0f + levelMult * 1.5f;
                    break;
                default:
                    break;
            }
            RecalculateStatsFromGear();
        }

        xpNeeded = GetXPForNextLevel();
        std::cout << "[XP] " << m_Name << " leveled up to " << m_Level << "!" << std::endl;
    }

    return levelsGained;
}

bool Entity::EquipItem(uint8_t inventorySlot, EquipmentSlot equipSlot) {
    if (!m_Inventory || !m_Equipment || !m_Stats) return false;

    ItemInstance* item = m_Inventory->GetItem(inventorySlot);
    if (!item) return false;

    const ItemTemplate* tmpl = ItemTemplateManager::Instance().GetTemplate(item->templateId);
    if (!tmpl) return false;

    // Check if item can be equipped in this slot
    if (!m_Equipment->CanEquipInSlot(equipSlot, tmpl)) return false;

    // If slot is occupied, swap items
    ItemInstance* equipped = m_Equipment->GetEquipped(equipSlot);
    if (equipped) {
        // Swap: put equipped item in inventory slot
        ItemInstance temp = *equipped;
        m_Equipment->Equip(equipSlot, *item);
        m_Inventory->slots[inventorySlot].item = temp;
    } else {
        // Just equip, remove from inventory
        m_Equipment->Equip(equipSlot, *item);
        m_Inventory->RemoveItem(inventorySlot);
    }

    RecalculateStatsFromGear();
    return true;
}

bool Entity::UnequipItem(EquipmentSlot equipSlot, uint8_t* outInventorySlot) {
    if (!m_Inventory || !m_Equipment || !m_Stats) return false;

    ItemInstance* equipped = m_Equipment->GetEquipped(equipSlot);
    if (!equipped) return false;

    // Find empty inventory slot
    int emptySlot = m_Inventory->FindFirstEmptySlot();
    if (emptySlot < 0) return false;  // Inventory full

    // Move to inventory
    m_Inventory->slots[emptySlot].item = *equipped;
    m_Equipment->Unequip(equipSlot);

    if (outInventorySlot) *outInventorySlot = static_cast<uint8_t>(emptySlot);

    RecalculateStatsFromGear();
    return true;
}

void Entity::RecalculateStatsFromGear() {
    if (!m_Stats || !m_Equipment) return;

    // Clear all gear modifiers
    for (size_t i = 0; i < static_cast<size_t>(StatType::COUNT); i++) {
        m_Stats->modifiers[i][static_cast<size_t>(ModifierType::BASE_VALUE)] = 0;
    }
    m_Stats->totalArmor = 0;
    m_Stats->weaponDamageMin = 1.0f;
    m_Stats->weaponDamageMax = 2.0f;
    m_Stats->weaponSpeed = 2.0f;

    // Apply stats from all equipped items
    for (uint8_t i = 0; i < EQUIPMENT_SLOT_COUNT; i++) {
        const ItemInstance* item = m_Equipment->GetEquipped(static_cast<EquipmentSlot>(i));
        if (!item) continue;

        const ItemTemplate* tmpl = ItemTemplateManager::Instance().GetTemplate(item->templateId);
        if (!tmpl) continue;

        // Apply stat bonuses
        for (uint8_t j = 0; j < tmpl->statCount; j++) {
            m_Stats->ApplyMod(tmpl->stats[j].stat, ModifierType::BASE_VALUE,
                              static_cast<float>(tmpl->stats[j].value));
        }

        // Apply armor
        m_Stats->totalArmor += tmpl->armor;

        // Apply weapon stats (if weapon slot)
        if (static_cast<EquipmentSlot>(i) == EquipmentSlot::WEAPON && tmpl->IsWeapon()) {
            m_Stats->weaponDamageMin = tmpl->weaponDamageMin;
            m_Stats->weaponDamageMax = tmpl->weaponDamageMax;
            m_Stats->weaponSpeed = tmpl->weaponSpeed;
        }
    }

    m_Stats->RecalculateStats();

    // Update health/mana max based on stamina/intellect
    if (m_Health) {
        int32_t bonusHealth = m_Stats->GetBonusHealth();
        // Keep current health percentage
        float healthPct = static_cast<float>(m_Health->current) / static_cast<float>(m_Health->max);
        m_Health->max = m_Health->baseMax + bonusHealth;  // Class base + stamina bonus
        m_Health->current = static_cast<int32_t>(m_Health->max * healthPct);
    }

    if (m_Mana) {
        int32_t bonusMana = m_Stats->GetBonusMana();
        float manaPct = static_cast<float>(m_Mana->current) / static_cast<float>(m_Mana->max);
        m_Mana->max = m_Mana->baseMax + bonusMana;  // Class base + intellect bonus
        m_Mana->current = static_cast<int32_t>(m_Mana->max * manaPct);
    }
}

void Entity::AddAbility(AbilityId id) {
    if (!HasAbility(id)) {
        m_Abilities.push_back(id);
    }
}

bool Entity::HasAbility(AbilityId id) const {
    return std::find(m_Abilities.begin(), m_Abilities.end(), id) != m_Abilities.end();
}

void Entity::Update(float dt) {
    // Update cooldowns
    if (m_Combat) {
        for (auto& [id, cooldown] : m_Combat->cooldowns) {
            if (cooldown > 0.0f) {
                cooldown -= dt;
            }
        }

        // Update cast timer
        if (m_Combat->IsCasting()) {
            m_Combat->castTimer += dt;
        }
    }

    // Calculate speed modifier from auras
    float speedModifier = 1.0f;
    if (m_Auras) {
        speedModifier = m_Auras->GetSpeedModifier();
    }

    // Update position based on velocity
    if (m_Movement && m_Movement->velocity.LengthSquared() > 0.001f) {
        // Check if rooted (can't move)
        bool isRooted = m_Auras && m_Auras->IsRooted();
        if (!isRooted) {
            float effectiveSpeed = m_Movement->speed * speedModifier;
            m_Movement->position += m_Movement->velocity.Normalized() * effectiveSpeed * dt;
        }

        // Update move state
        if (m_Movement->velocity.Length() > 0.1f) {
            m_Movement->moveState = MoveState::RUNNING;
        }
    } else if (m_Movement) {
        m_Movement->moveState = m_Health && m_Health->IsDead() ?
                                MoveState::DEAD : MoveState::IDLE;
    }
}

} // namespace MMO
