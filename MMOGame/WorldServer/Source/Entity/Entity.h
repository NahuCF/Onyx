#pragma once

#include "Components.h"
#include "AuraComponent.h"
#include "../../../Shared/Source/Types/Types.h"
#include "../../../Shared/Source/Items/Items.h"
#include <memory>
#include <vector>
#include <string>

namespace MMO {

// ============================================================
// ENTITY
// ============================================================

class Entity {
public:
    Entity(EntityId id, EntityType type, const std::string& name);
    virtual ~Entity() = default;

    EntityId GetId() const { return m_Id; }
    EntityType GetType() const { return m_Type; }
    const std::string& GetName() const { return m_Name; }
    CharacterClass GetClass() const { return m_Class; }
    uint32_t GetLevel() const { return m_Level; }

    void SetClass(CharacterClass c) { m_Class = c; }
    void SetLevel(uint32_t level) { m_Level = level; }

    // Summoner tracking (for summoned creatures)
    void SetSummoner(EntityId id) { m_SummonerId = id; }
    EntityId GetSummoner() const { return m_SummonerId; }

    // Components - using optional pointers (could use ECS but keeping simple)
    HealthComponent* GetHealth() { return m_Health.get(); }
    ManaComponent* GetMana() { return m_Mana.get(); }
    MovementComponent* GetMovement() { return m_Movement.get(); }
    CombatComponent* GetCombat() { return m_Combat.get(); }
    AggroComponent* GetAggro() { return m_Aggro.get(); }
    WalletComponent* GetWallet() { return m_Wallet.get(); }
    InventoryComponent* GetInventory() { return m_Inventory.get(); }
    EquipmentComponent* GetEquipment() { return m_Equipment.get(); }
    StatsComponent* GetStats() { return m_Stats.get(); }
    ExperienceComponent* GetExperience() { return m_Experience.get(); }
    AuraComponent* GetAuras() { return m_Auras.get(); }

    const HealthComponent* GetHealth() const { return m_Health.get(); }
    const ManaComponent* GetMana() const { return m_Mana.get(); }
    const MovementComponent* GetMovement() const { return m_Movement.get(); }
    const CombatComponent* GetCombat() const { return m_Combat.get(); }
    const AggroComponent* GetAggro() const { return m_Aggro.get(); }
    const WalletComponent* GetWallet() const { return m_Wallet.get(); }
    const InventoryComponent* GetInventory() const { return m_Inventory.get(); }
    const EquipmentComponent* GetEquipment() const { return m_Equipment.get(); }
    const StatsComponent* GetStats() const { return m_Stats.get(); }
    const ExperienceComponent* GetExperience() const { return m_Experience.get(); }
    const AuraComponent* GetAuras() const { return m_Auras.get(); }

    void AddHealthComponent(int32_t max);
    void AddManaComponent(int32_t max);
    void AddMovementComponent(float speed);
    void AddCombatComponent(float attackRange);
    void AddAggroComponent(float aggroRadius, float leashRadius);
    void AddWalletComponent(uint32_t initialMoney = 0);
    void AddInventoryComponent();
    void AddEquipmentComponent();
    void AddStatsComponent();
    void AddExperienceComponent(uint32_t currentXP = 0);
    void AddAuraComponent();

    // XP and Leveling
    // Returns number of levels gained (0 if no level up)
    uint32_t GiveXP(uint32_t amount);
    uint32_t GetXPForNextLevel() const;

    // Equipment management
    bool EquipItem(uint8_t inventorySlot, EquipmentSlot equipSlot);
    bool UnequipItem(EquipmentSlot equipSlot, uint8_t* outInventorySlot = nullptr);
    void RecalculateStatsFromGear();

    // Abilities
    void AddAbility(AbilityId id);
    const std::vector<AbilityId>& GetAbilities() const { return m_Abilities; }
    bool HasAbility(AbilityId id) const;

    // Update
    virtual void Update(float dt);

private:
    EntityId m_Id;
    EntityType m_Type;
    std::string m_Name;
    CharacterClass m_Class = CharacterClass::NONE;
    uint32_t m_Level = 1;
    EntityId m_SummonerId = 0;  // Who summoned this entity (0 = not summoned)

    std::unique_ptr<HealthComponent> m_Health;
    std::unique_ptr<ManaComponent> m_Mana;
    std::unique_ptr<MovementComponent> m_Movement;
    std::unique_ptr<CombatComponent> m_Combat;
    std::unique_ptr<AggroComponent> m_Aggro;
    std::unique_ptr<WalletComponent> m_Wallet;
    std::unique_ptr<InventoryComponent> m_Inventory;
    std::unique_ptr<EquipmentComponent> m_Equipment;
    std::unique_ptr<StatsComponent> m_Stats;
    std::unique_ptr<ExperienceComponent> m_Experience;
    std::unique_ptr<AuraComponent> m_Auras;

    std::vector<AbilityId> m_Abilities;
};

} // namespace MMO
