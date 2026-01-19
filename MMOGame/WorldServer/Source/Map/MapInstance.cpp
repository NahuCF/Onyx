#include "MapInstance.h"
#include "MapManager.h"
#include "MapTemplates.h"
#include "../AI/ScriptedAI.h"
#include "../AI/ScriptRegistry.h"
#include "../AI/CreatureTemplates.h"
#include "../Scripts/ScriptLoader.h"
#include "Items/Items.h"
#include "../../../Shared/Source/Spells/SpellDefines.h"
#include <iostream>
#include <algorithm>

namespace MMO {

// ============================================================
// MAP INSTANCE
// ============================================================

MapInstance::MapInstance(uint32_t instanceId, const MapTemplate* tmpl)
    : m_InstanceId(instanceId)
    , m_Template(tmpl)
    , m_Grid(this) {
}

MapInstance::~MapInstance() = default;

EntityId MapInstance::GenerateEntityId() {
    // Use global counter from MapManager to prevent ID collisions across maps
    return MapManager::Instance().GenerateGlobalEntityId();
}

void MapInstance::SpawnInitialMobs() {
    // Register spawn points with the grid (AzerothCore-style lazy loading)
    // Mobs will be spawned when players enter the grid cells
    for (const auto& spawn : m_Template->mobSpawns) {
        m_Grid.RegisterSpawnPoint(spawn.id, spawn.position);
    }
    std::cout << "[Map:" << m_Template->name << "] Registered " << m_Template->mobSpawns.size()
              << " spawn points for grid-based loading" << std::endl;
}

Entity* MapInstance::CreatePlayer(CharacterId characterId, const std::string& name,
                                   CharacterClass charClass, uint32_t level, Vec2 position,
                                   int32_t currentHealth, int32_t currentMana) {
    EntityId id = GenerateEntityId();
    auto entity = std::make_unique<Entity>(id, EntityType::PLAYER, name);

    entity->SetClass(charClass);
    entity->SetLevel(level);

    // Add components based on class
    switch (charClass) {
        case CharacterClass::WARRIOR:
            entity->AddHealthComponent(150 + (level - 1) * 15);
            entity->AddManaComponent(50 + (level - 1) * 5);
            entity->AddMovementComponent(5.0f);
            entity->AddCombatComponent(2.0f);
            entity->AddAbility(AbilityId::WARRIOR_SLASH);
            entity->AddAbility(AbilityId::WARRIOR_SHIELD_BASH);
            entity->AddAbility(AbilityId::WARRIOR_CHARGE);
            break;

        case CharacterClass::WITCH:
            entity->AddHealthComponent(80 + (level - 1) * 8);
            entity->AddManaComponent(120 + (level - 1) * 12);
            entity->AddMovementComponent(5.0f);
            entity->AddCombatComponent(25.0f);
            entity->AddAbility(AbilityId::WITCH_FIREBALL);
            entity->AddAbility(AbilityId::WITCH_HEAL);
            entity->AddAbility(AbilityId::WITCH_FROST_BOLT);
            break;

        default:
            entity->AddHealthComponent(100);
            entity->AddManaComponent(100);
            entity->AddMovementComponent(5.0f);
            entity->AddCombatComponent(2.0f);
            break;
    }

    // Set position
    entity->GetMovement()->position = position;

    // Restore health/mana if provided (from database)
    auto health = entity->GetHealth();
    auto mana = entity->GetMana();
    if (health && currentHealth > 0) {
        health->current = std::min(currentHealth, health->max);
    }
    if (mana && currentMana > 0) {
        mana->current = std::min(currentMana, mana->max);
    }

    // Add wallet for currency (money will be loaded from DB by WorldServer)
    entity->AddWalletComponent(0);

    // Add experience component (XP will be loaded from DB by WorldServer)
    entity->AddExperienceComponent(0);

    // Add aura component for buff/debuff tracking
    entity->AddAuraComponent();

    Entity* ptr = entity.get();
    Vec2 pos = ptr->GetMovement()->position;
    m_Entities[id] = std::move(entity);

    // Add to grid (isPlayer = true)
    m_Grid.AddEntity(id, pos, true);

    std::cout << "[Map:" << m_Template->name << "] Player created: " << name << " (ID: " << id << ")" << std::endl;
    return ptr;
}

Entity* MapInstance::CreateMob(uint32_t creatureTemplateId, Vec2 position, uint32_t spawnPointId) {
    const CreatureTemplate* tmpl = CreatureTemplates::GetTemplate(creatureTemplateId);
    if (!tmpl) {
        std::cout << "[Map:" << m_Template->name << "] Unknown creature template: " << creatureTemplateId << std::endl;
        return nullptr;
    }

    EntityId id = GenerateEntityId();
    auto entity = std::make_unique<Entity>(id, EntityType::MOB, tmpl->name);

    entity->SetLevel(tmpl->level);
    entity->AddHealthComponent(tmpl->maxHealth);
    if (tmpl->maxMana > 0) {
        entity->AddManaComponent(tmpl->maxMana);
    }
    entity->AddMovementComponent(tmpl->speed);
    entity->AddCombatComponent(2.5f);
    entity->AddAggroComponent(tmpl->aggroRadius, tmpl->leashRadius);
    entity->AddAuraComponent();

    // Set position and home
    entity->GetMovement()->position = position;
    entity->GetAggro()->homePosition = position;

    // Add abilities from template
    for (const auto& rule : tmpl->abilities) {
        entity->AddAbility(rule.ability);
    }

    // Create AI using ScriptRegistry (creates custom script if defined, otherwise default)
    Entity* ptr = entity.get();
    m_MobAIs[id] = ScriptRegistry::Instance().CreateAI(ptr, tmpl);

    // Track spawn point for respawning
    if (spawnPointId > 0) {
        m_EntityToSpawnPoint[id] = spawnPointId;
    }

    m_Entities[id] = std::move(entity);

    // Add to grid (isPlayer = false)
    m_Grid.AddEntity(id, position, false);

    std::cout << "[Map:" << m_Template->name << "] Mob created: " << tmpl->name << " (ID: " << id << ")" << std::endl;
    return ptr;
}

Entity* MapInstance::SummonCreature(uint32_t creatureTemplateId, Vec2 position, EntityId summonerId) {
    Entity* creature = CreateMob(creatureTemplateId, position, 0);  // No spawn point
    if (creature) {
        creature->SetSummoner(summonerId);
        std::cout << "[Map:" << m_Template->name << "] Creature summoned by entity " << summonerId << std::endl;
    }
    return creature;
}

ScriptedAI* MapInstance::GetMobAI(EntityId entityId) {
    auto it = m_MobAIs.find(entityId);
    return it != m_MobAIs.end() ? it->second.get() : nullptr;
}

void MapInstance::RemoveEntity(EntityId id) {
    // Remove from grid (marks despawned flag for visibility system)
    m_Grid.RemoveEntity(id);

    m_Entities.erase(id);
    m_Players.erase(id);
    m_MobAIs.erase(id);
    m_EntityToSpawnPoint.erase(id);
}

Entity* MapInstance::GetEntity(EntityId id) {
    auto it = m_Entities.find(id);
    return it != m_Entities.end() ? it->second.get() : nullptr;
}

std::unique_ptr<Entity> MapInstance::ReleaseEntity(EntityId id) {
    auto it = m_Entities.find(id);
    if (it == m_Entities.end()) {
        std::cerr << "[Map:" << m_Template->name << "] ReleaseEntity failed: entity " << id
                  << " not found. Entities in map: " << m_Entities.size() << std::endl;
        for (const auto& [eid, ent] : m_Entities) {
            std::cerr << "  - Entity " << eid << ": " << ent->GetName() << std::endl;
        }
        return nullptr;
    }

    // Remove from grid
    m_Grid.RemoveEntity(id);

    // Extract the entity (transfers ownership)
    std::unique_ptr<Entity> entity = std::move(it->second);
    m_Entities.erase(it);

    // Clean up player registration if this is a player
    auto playerIt = m_Players.find(id);
    if (playerIt != m_Players.end()) {
        m_PeerToEntity.erase(playerIt->second.peerId);
        m_Players.erase(playerIt);
    }

    // Clean up associated data
    m_MobAIs.erase(id);
    m_EntityToSpawnPoint.erase(id);

    std::cout << "[Map:" << m_Template->name << "] Released entity: " << entity->GetName()
              << " (ID: " << id << ")" << std::endl;

    return entity;
}

Entity* MapInstance::AdoptEntity(std::unique_ptr<Entity> entity) {
    if (!entity) return nullptr;

    EntityId id = entity->GetId();
    Vec2 pos = entity->GetMovement() ? entity->GetMovement()->position : Vec2();
    bool isPlayer = entity->GetType() == EntityType::PLAYER;

    Entity* ptr = entity.get();
    m_Entities[id] = std::move(entity);

    // Add to grid
    m_Grid.AddEntity(id, pos, isPlayer);

    std::cout << "[Map:" << m_Template->name << "] Adopted entity: " << ptr->GetName()
              << " (ID: " << id << ")" << std::endl;

    return ptr;
}

void MapInstance::RegisterPlayer(EntityId entityId, uint32_t peerId,
                                  CharacterId characterId, AccountId accountId) {
    PlayerInfo info;
    info.peerId = peerId;
    info.characterId = characterId;
    info.accountId = accountId;
    info.lastInputSeq = 0;

    m_Players[entityId] = info;
    m_PeerToEntity[peerId] = entityId;
}

void MapInstance::UnregisterPlayer(EntityId entityId) {
    auto it = m_Players.find(entityId);
    if (it != m_Players.end()) {
        m_PeerToEntity.erase(it->second.peerId);
        m_Players.erase(it);
    }
    RemoveEntity(entityId);
}

EntityId MapInstance::GetEntityIdByPeerId(uint32_t peerId) const {
    auto it = m_PeerToEntity.find(peerId);
    return it != m_PeerToEntity.end() ? it->second : INVALID_ENTITY_ID;
}

const PlayerInfo* MapInstance::GetPlayerInfo(EntityId entityId) const {
    auto it = m_Players.find(entityId);
    return it != m_Players.end() ? &it->second : nullptr;
}

std::vector<Entity*> MapInstance::GetEntitiesInRadius(Vec2 center, float radius) {
    std::vector<Entity*> result;
    float radiusSq = radius * radius;

    for (auto& [id, entity] : m_Entities) {
        auto movement = entity->GetMovement();
        if (movement) {
            float distSq = Vec2::DistanceSquared(center, movement->position);
            if (distSq <= radiusSq) {
                result.push_back(entity.get());
            }
        }
    }
    return result;
}

std::vector<Entity*> MapInstance::GetPlayersInRadius(Vec2 center, float radius) {
    std::vector<Entity*> result;
    float radiusSq = radius * radius;

    for (auto& [entityId, playerInfo] : m_Players) {
        Entity* entity = GetEntity(entityId);
        if (entity) {
            auto movement = entity->GetMovement();
            if (movement) {
                float distSq = Vec2::DistanceSquared(center, movement->position);
                if (distSq <= radiusSq) {
                    result.push_back(entity);
                }
            }
        }
    }
    return result;
}

const Portal* MapInstance::CheckPortal(Vec2 position) {
    for (const auto& portal : m_Template->portals) {
        if (portal.Contains(position)) {
            return &portal;
        }
    }
    return nullptr;
}

void MapInstance::Update(float dt) {
    m_Time += dt;

    // Update grid activation (AzerothCore-style lazy loading)
    UpdateGridActivation(dt);

    // Store old positions for grid updates and dirty tracking
    std::unordered_map<EntityId, Vec2> oldPositions;
    for (auto& [id, entity] : m_Entities) {
        auto movement = entity->GetMovement();
        if (movement) {
            oldPositions[id] = movement->position;
        }
    }

    // Update all entities
    for (auto& [id, entity] : m_Entities) {
        entity->Update(dt);
    }

    // Update mob AI
    for (auto& [id, ai] : m_MobAIs) {
        Entity* mob = GetEntity(id);
        if (mob && mob->GetHealth() && !mob->GetHealth()->IsDead()) {
            UpdateMobAI(mob, dt);
        }
    }

    // Check for position changes and update grid
    for (auto& [id, entity] : m_Entities) {
        auto movement = entity->GetMovement();
        if (!movement) continue;

        auto it = oldPositions.find(id);
        if (it != oldPositions.end()) {
            Vec2 oldPos = it->second;
            Vec2 newPos = movement->position;

            // Check if position changed significantly (more than 0.01 units)
            float distSq = Vec2::DistanceSquared(oldPos, newPos);
            if (distSq > 0.0001f) {
                bool isPlayer = (entity->GetType() == EntityType::PLAYER);
                m_Grid.MoveEntity(id, oldPos, newPos, isPlayer);
            }
        }
    }

    // Update casts
    UpdateCasts(dt);

    // Update projectiles
    UpdateProjectiles(dt);

    // Update respawns
    UpdateRespawns(dt);

    // Update loot despawn timers
    UpdateLoot(dt);

    // Update health/mana regeneration
    UpdateRegeneration(dt);

    // Update auras (ticks, expiration)
    UpdateAuras(dt);
}

void MapInstance::UpdateMobAI(Entity* mob, float dt) {
    auto it = m_MobAIs.find(mob->GetId());
    if (it == m_MobAIs.end()) return;

    ScriptedAI* ai = it->second.get();
    auto aggro = mob->GetAggro();
    auto combat = mob->GetCombat();
    auto movement = mob->GetMovement();

    if (!aggro || !combat || !movement) return;

    // Get current target
    EntityId targetId = aggro->GetTopThreat();
    Entity* target = GetEntity(targetId);

    // Clean up stale threat entries (player left zone, etc.)
    if (targetId != INVALID_ENTITY_ID && !target) {
        aggro->RemoveThreat(targetId);
        targetId = aggro->GetTopThreat();
        target = GetEntity(targetId);
    }

    if (targetId == INVALID_ENTITY_ID) {
        // Look for players in aggro radius
        if (!aggro->isEvading) {
            auto nearbyPlayers = GetPlayersInRadius(movement->position, aggro->aggroRadius);
            for (Entity* player : nearbyPlayers) {
                if (player->GetHealth() && !player->GetHealth()->IsDead()) {
                    targetId = player->GetId();
                    target = player;
                    aggro->AddThreat(targetId, 1.0f);
                    ai->OnEnterCombat(target);
                    break;
                }
            }
        }
    }

    // Check leash distance
    if (target && !aggro->isEvading) {
        float distFromHome = Vec2::Distance(movement->position, aggro->homePosition);
        if (distFromHome > aggro->leashRadius) {
            // Evade - return home
            aggro->ClearThreat();
            aggro->isEvading = true;
            ai->OnEvade();
            combat->targetId = INVALID_ENTITY_ID;
            target = nullptr;
        }
    }

    // Handle evading
    if (aggro->isEvading) {
        float distToHome = Vec2::Distance(movement->position, aggro->homePosition);
        if (distToHome < 1.0f) {
            // Arrived home
            aggro->isEvading = false;
            movement->velocity = Vec2(0, 0);
            // Heal to full
            auto health = mob->GetHealth();
            if (health && health->current < health->max) {
                health->current = health->max;
                MarkHealthDirty(mob->GetId());
            }
        } else {
            // Move towards home
            Vec2 dir = (aggro->homePosition - movement->position).Normalized();
            movement->velocity = dir * movement->speed;
        }
        return;
    }

    // If we have a target, run AI
    if (target) {
        EntityId oldTargetId = combat->targetId;
        combat->targetId = targetId;

        // Mark target dirty if target changed
        if (oldTargetId != targetId) {
            MarkTargetDirty(mob->GetId());
        }

        // Chase if not in range
        float dist = Vec2::Distance(movement->position, target->GetMovement()->position);
        if (dist > combat->attackRange) {
            Vec2 dir = (target->GetMovement()->position - movement->position).Normalized();
            movement->velocity = dir * movement->speed;
        } else {
            movement->velocity = Vec2(0, 0);
        }

        // Run AI for abilities with callbacks
        CastAbilityFn castFn = [this](EntityId src, EntityId tgt, AbilityId ability) {
            ProcessAbility(src, tgt, ability);
        };
        SummonCreatureFn summonFn = [this](uint32_t creatureId, Vec2 pos, EntityId summoner) {
            return SummonCreature(creatureId, pos, summoner);
        };
        DespawnCreatureFn despawnFn = [this](EntityId entityId) {
            RemoveEntity(entityId);
        };

        ai->Update(dt, target, castFn, summonFn, despawnFn);
    } else {
        // No target - return home if not already there
        if (combat->targetId != INVALID_ENTITY_ID) {
            combat->targetId = INVALID_ENTITY_ID;
            MarkTargetDirty(mob->GetId());
        }

        float distToHome = Vec2::Distance(movement->position, aggro->homePosition);
        if (distToHome > 1.0f) {
            // Return to home position
            Vec2 dir = (aggro->homePosition - movement->position).Normalized();
            movement->velocity = dir * movement->speed;

            // Reset combat state
            if (ai->IsInCombat()) {
                ai->OnEvade();
            }
        } else {
            // At home, stop moving and heal to full
            movement->velocity = Vec2(0, 0);
            auto health = mob->GetHealth();
            if (health && health->current < health->max) {
                health->current = health->max;
                MarkHealthDirty(mob->GetId());
            }
        }
    }
}

void MapInstance::UpdateCasts(float dt) {
    for (auto& [id, entity] : m_Entities) {
        auto combat = entity->GetCombat();
        if (!combat || !combat->IsCasting()) continue;

        if (combat->castTimer >= combat->castDuration) {
            AbilityId abilityId = combat->currentCast;
            EntityId targetId = combat->castTargetId;
            Vec2 targetPos = combat->castTargetPosition;

            combat->currentCast = AbilityId::NONE;
            combat->castTimer = 0.0f;

            // Mark casting dirty when cast ends
            MarkCastingDirty(id);

            ExecuteAbility(id, targetId, abilityId, targetPos);

            GameEvent event;
            event.type = GameEventType::CAST_END;
            event.sourceId = id;
            event.targetId = targetId;
            event.abilityId = abilityId;
            event.value = 0;
            event.position = entity->GetMovement() ? entity->GetMovement()->position : Vec2();
            BroadcastEvent(event);
        }
    }
}

void MapInstance::UpdateProjectiles(float dt) {
    const float HIT_RADIUS = 0.5f;

    for (auto it = m_Projectiles.begin(); it != m_Projectiles.end();) {
        Projectile& proj = *it;

        Entity* target = GetEntity(proj.targetId);
        if (target && target->GetMovement()) {
            proj.targetPosition = target->GetMovement()->position;
        }

        Vec2 direction = proj.targetPosition - proj.position;
        float distance = direction.Length();

        if (distance < HIT_RADIUS) {
            Entity* source = GetEntity(proj.sourceId);

            if (proj.isHeal) {
                if (target) {
                    ApplyHeal(source, target, proj.damage, proj.abilityId);
                }
            } else {
                if (target) {
                    ApplyDamage(source, target, proj.damage, proj.abilityId, proj.damageType);

                    // Apply aura effect if this projectile carries one
                    if (proj.auraType != AuraType::NONE) {
                        SpellEffect auraEffect;
                        auraEffect.type = SpellEffectType::APPLY_AURA;
                        auraEffect.auraType = proj.auraType;
                        auraEffect.auraValue = proj.auraValue;
                        auraEffect.auraDuration = proj.auraDuration;
                        ApplyAura(source, target, auraEffect, proj.abilityId);
                    }
                }
            }

            GameEvent event;
            event.type = GameEventType::PROJECTILE_HIT;
            event.sourceId = proj.sourceId;
            event.targetId = proj.targetId;
            event.abilityId = proj.abilityId;
            event.value = proj.damage;
            event.position = proj.position;
            BroadcastEvent(event);

            it = m_Projectiles.erase(it);
        } else {
            proj.position += direction.Normalized() * proj.speed * dt;
            it++;
        }
    }
}

void MapInstance::UpdateRespawns(float dt) {
    // Don't remove dead mobs here - let UpdateLoot handle corpse removal
    // Only process pending respawns

    // Process pending respawns (grid-aware)
    for (auto it = m_PendingRespawns.begin(); it != m_PendingRespawns.end();) {
        if (m_Time >= it->respawnAt) {
            // Find spawn point
            for (const auto& spawn : m_Template->mobSpawns) {
                if (spawn.id == it->spawnPointId) {
                    // Check if the grid cell is active before spawning
                    CellCoord cellCoord = Grid::PositionToCell(spawn.position);
                    if (m_Grid.IsCellActive(cellCoord)) {
                        Entity* newMob = CreateMob(spawn.creatureTemplateId, spawn.position, spawn.id);
                        if (newMob) {
                            // Register with grid (visibility system handles network broadcast)
                            m_Grid.RegisterSpawnedMob(cellCoord, newMob->GetId());
                            std::cout << "[Respawn] " << newMob->GetName() << " spawned in active cell"
                                      << std::endl;
                        }
                    } else {
                        // Cell is inactive - mob will spawn when cell reactivates
                        std::cout << "[Respawn] Skipping spawn in inactive cell ("
                                  << cellCoord.x << ", " << cellCoord.y << ")" << std::endl;
                    }
                    break;
                }
            }
            it = m_PendingRespawns.erase(it);
        } else {
            it++;
        }
    }
}

void MapInstance::ProcessInput(EntityId playerId, const C_Input& input) {
    Entity* player = GetEntity(playerId);
    if (!player) return;

    auto movement = player->GetMovement();
    auto combat = player->GetCombat();
    auto health = player->GetHealth();
    if (!movement) return;

    if (health && health->IsDead()) {
        movement->velocity = Vec2(0, 0);
        return;
    }

    auto it = m_Players.find(playerId);
    if (it != m_Players.end()) {
        it->second.lastInputSeq = input.sequence;
    }

    if (combat && combat->IsCasting() && (input.moveX != 0 || input.moveY != 0)) {
        combat->currentCast = AbilityId::NONE;
        combat->castTimer = 0.0f;

        // Mark casting dirty when cancelled
        MarkCastingDirty(playerId);

        GameEvent event;
        event.type = GameEventType::CAST_CANCEL;
        event.sourceId = playerId;
        event.targetId = INVALID_ENTITY_ID;
        event.abilityId = AbilityId::NONE;
        event.value = 0;
        event.position = movement->position;
        BroadcastEvent(event);
    }

    Vec2 inputDir(static_cast<float>(input.moveX), static_cast<float>(input.moveY));
    if (inputDir.LengthSquared() > 0.1f) {
        // Apply speed modifier from auras
        float speedModifier = 1.0f;
        auto auras = player->GetAuras();
        if (auras) {
            speedModifier = auras->GetSpeedModifier();
        }
        movement->velocity = inputDir.Normalized() * movement->speed * speedModifier;
        movement->rotation = input.rotation;
    } else {
        movement->velocity = Vec2(0, 0);
    }
}

void MapInstance::ProcessAbility(EntityId sourceId, EntityId targetId, AbilityId abilityId) {
    Entity* source = GetEntity(sourceId);
    Entity* target = GetEntity(targetId);

    if (!source) return;

    auto sourceHealth = source->GetHealth();
    if (sourceHealth && sourceHealth->IsDead()) return;

    AbilityData ability = AbilityData::GetAbilityData(abilityId);
    auto combat = source->GetCombat();
    auto mana = source->GetMana();
    auto movement = source->GetMovement();

    if (!combat || !movement) return;

    if (combat->IsCasting()) return;
    if (!combat->IsAbilityReady(abilityId)) return;
    if (mana && !mana->HasMana(ability.manaCost)) return;

    if (target && target->GetMovement() && ability.range > 0) {
        float distance = Vec2::Distance(movement->position, target->GetMovement()->position);
        if (distance > ability.range) return;
    }

    if (ability.castTime > 0.0f) {
        combat->currentCast = abilityId;
        combat->castTimer = 0.0f;
        combat->castDuration = ability.castTime;
        combat->castTargetId = targetId;
        combat->castTargetPosition = target && target->GetMovement() ?
            target->GetMovement()->position : movement->position;

        movement->velocity = Vec2(0, 0);

        // Mark casting dirty
        MarkCastingDirty(sourceId);

        GameEvent event;
        event.type = GameEventType::CAST_START;
        event.sourceId = sourceId;
        event.targetId = targetId;
        event.abilityId = abilityId;
        event.value = static_cast<int32_t>(ability.castTime * 1000);
        event.position = movement->position;
        BroadcastEvent(event);
        return;
    }

    ExecuteAbility(sourceId, targetId, abilityId,
        target && target->GetMovement() ? target->GetMovement()->position : movement->position);
}

int32_t MapInstance::CalculateEffectDamage(Entity* source, const SpellEffect& effect) {
    auto stats = source->GetStats();

    // For mobs or entities without stats, use base effect value
    if (!stats) {
        return effect.value;
    }

    int32_t finalDamage = 0;

    if (effect.damageType == DamageType::PHYSICAL) {
        // Physical damage: weapon damage + ability base damage
        // Random roll between min and max weapon damage
        float weaponDmgMin = stats->GetMeleeDamageMin();
        float weaponDmgMax = stats->GetMeleeDamageMax();
        float weaponDamage = weaponDmgMin + (static_cast<float>(rand()) / RAND_MAX) * (weaponDmgMax - weaponDmgMin);

        // Final damage = weapon damage + effect base damage
        finalDamage = static_cast<int32_t>(weaponDamage) + effect.value;
    } else {
        // Magical damage: base + spell power contribution
        float spellPower = stats->GetSpellPower();
        finalDamage = effect.value + static_cast<int32_t>(spellPower * 0.5f);
    }

    // Minimum 1 damage
    return std::max(1, finalDamage);
}

void MapInstance::ExecuteAbility(EntityId sourceId, EntityId targetId, AbilityId abilityId, Vec2 targetPosition) {
    Entity* source = GetEntity(sourceId);
    Entity* target = GetEntity(targetId);

    if (!source) return;

    AbilityData ability = AbilityData::GetAbilityData(abilityId);
    auto combat = source->GetCombat();
    auto mana = source->GetMana();
    auto movement = source->GetMovement();

    if (!combat) return;

    if (mana && ability.manaCost > 0) {
        mana->UseMana(ability.manaCost);
        // Mark mana use time for 5-second rule (AzerothCore-style)
        combat->MarkManaUse(m_Time);
    }

    combat->cooldowns[abilityId] = ability.cooldown;

    // Process all spell effects
    for (const auto& effect : ability.effects) {
        ProcessSpellEffect(source, target, effect, abilityId);
    }

    // Broadcast ability effect event
    GameEvent event;
    event.type = GameEventType::ABILITY_EFFECT;
    event.sourceId = sourceId;
    event.targetId = targetId;
    event.abilityId = abilityId;
    event.value = ability.GetDirectDamage();  // For display purposes
    event.position = movement ? movement->position : Vec2();
    BroadcastEvent(event);
}

void MapInstance::ProcessSpellEffect(Entity* source, Entity* target, const SpellEffect& effect, AbilityId abilityId) {
    if (!source) return;

    switch (effect.type) {
        case SpellEffectType::DIRECT_DAMAGE: {
            if (!target) return;
            int32_t damage = CalculateEffectDamage(source, effect);
            ApplyDamage(source, target, damage, abilityId, effect.damageType);
            break;
        }

        case SpellEffectType::DIRECT_HEAL: {
            // Heal target, or self if no valid target
            Entity* healTarget = target;
            if (!healTarget || healTarget->GetType() == EntityType::MOB) {
                healTarget = source;
            }
            int32_t healAmount = effect.value;
            ApplyHeal(source, healTarget, healAmount, abilityId);
            break;
        }

        case SpellEffectType::SPAWN_PROJECTILE: {
            if (!target) return;
            SpawnProjectile(source, target, effect, abilityId);
            break;
        }

        case SpellEffectType::APPLY_AURA: {
            // Apply aura to target (or self if no target)
            Entity* auraTarget = target ? target : source;
            ApplyAura(source, auraTarget, effect, abilityId);
            break;
        }

        case SpellEffectType::REMOVE_AURA: {
            // Remove auras of specified type from target
            Entity* auraTarget = target ? target : source;
            auto auras = auraTarget->GetAuras();
            if (auras) {
                auras->RemoveAurasByType(effect.auraType);
            }
            break;
        }

        default:
            break;
    }
}

void MapInstance::SpawnProjectile(Entity* source, Entity* target, const SpellEffect& effect, AbilityId abilityId) {
    if (!source || !target) return;

    auto sourceMovement = source->GetMovement();
    auto targetMovement = target->GetMovement();
    if (!sourceMovement || !targetMovement) return;

    // Calculate damage for projectile
    int32_t damage = CalculateEffectDamage(source, effect);

    Projectile proj;
    proj.id = m_NextProjectileId++;
    proj.sourceId = source->GetId();
    proj.targetId = target->GetId();
    proj.abilityId = abilityId;
    proj.position = sourceMovement->position;
    proj.targetPosition = targetMovement->position;
    proj.speed = effect.projectileSpeed > 0 ? effect.projectileSpeed : 15.0f;
    proj.damage = damage;
    proj.damageType = effect.damageType;
    proj.isHeal = (effect.auraValue == 1);  // auraValue = 1 means healing projectile

    // Carry aura effect if specified
    proj.auraType = effect.auraType;
    proj.auraValue = effect.auraValue;
    proj.auraDuration = effect.auraDuration;

    m_Projectiles.push_back(proj);

    GameEvent event;
    event.type = GameEventType::PROJECTILE_SPAWN;
    event.sourceId = source->GetId();
    event.targetId = target->GetId();
    event.abilityId = abilityId;
    event.value = static_cast<int32_t>(proj.id);
    event.position = proj.position;
    BroadcastEvent(event);
}

void MapInstance::ProcessTargetSelection(EntityId playerId, EntityId targetId) {
    Entity* player = GetEntity(playerId);
    if (!player) return;

    auto combat = player->GetCombat();
    if (combat) {
        combat->targetId = targetId;
        MarkTargetDirty(playerId);
    }
}

void MapInstance::ApplyDamage(Entity* source, Entity* target, int32_t damage, AbilityId abilityId, DamageType damageType) {
    if (!target) return;

    auto health = target->GetHealth();
    auto aggro = target->GetAggro();
    auto auras = target->GetAuras();

    if (!health || health->IsDead()) return;

    if (aggro && aggro->isEvading) return;

    // Check for damage immunity
    if (auras) {
        if (auras->IsImmune() || auras->IsImmuneToSchool(damageType)) {
            std::cout << "[Combat] " << target->GetName() << " is immune to damage" << std::endl;
            return;
        }

        // Apply damage taken modifiers
        float damageMod = auras->GetDamageTakenModifier();
        damage = static_cast<int32_t>(damage * damageMod);
    }

    health->TakeDamage(damage);

    // Mark both entities as in combat (AzerothCore-style combat state tracking)
    if (source) {
        auto sourceCombat = source->GetCombat();
        if (sourceCombat) {
            sourceCombat->MarkCombat(m_Time);
        }
    }
    auto targetCombat = target->GetCombat();
    if (targetCombat) {
        targetCombat->MarkCombat(m_Time);
    }

    // Mark health as dirty for grid-based broadcasting
    MarkHealthDirty(target->GetId());

    if (aggro && source) {
        aggro->AddThreat(source->GetId(), static_cast<float>(damage));
    }

    GameEvent event;
    event.type = GameEventType::DAMAGE;
    event.sourceId = source ? source->GetId() : INVALID_ENTITY_ID;
    event.targetId = target->GetId();
    event.abilityId = abilityId;
    event.value = damage;
    event.position = target->GetMovement() ? target->GetMovement()->position : Vec2();
    BroadcastEvent(event);

    if (health->IsDead()) {
        GameEvent deathEvent;
        deathEvent.type = GameEventType::DEATH;
        deathEvent.sourceId = source ? source->GetId() : INVALID_ENTITY_ID;
        deathEvent.targetId = target->GetId();
        deathEvent.abilityId = AbilityId::NONE;
        deathEvent.value = 0;
        deathEvent.position = target->GetMovement() ? target->GetMovement()->position : Vec2();
        BroadcastEvent(deathEvent);

        if (target->GetMovement()) {
            target->GetMovement()->velocity = Vec2(0, 0);
            target->GetMovement()->moveState = MoveState::DEAD;
            MarkMoveStateDirty(target->GetId());
        }

        // Generate loot for mobs
        if (target->GetType() == EntityType::MOB && source) {
            GenerateLoot(target, source->GetId());

            // Award XP to killer (if it's a player)
            if (source->GetType() == EntityType::PLAYER && source->GetExperience()) {
                AwardXP(source, target);
            }
        }

        // Notify summoner that this creature died (AzerothCore-style callback)
        EntityId summonerId = target->GetSummoner();
        if (summonerId != 0) {
            ScriptedAI* summonerAI = GetMobAI(summonerId);
            if (summonerAI) {
                summonerAI->OnSummonDied(target);
            }
        }

        // Notify this entity's AI that it died
        ScriptedAI* targetAI = GetMobAI(target->GetId());
        if (targetAI) {
            targetAI->OnDeath(source);
        }
    }
}

void MapInstance::ApplyHeal(Entity* source, Entity* target, int32_t amount, AbilityId abilityId) {
    if (!target) return;

    auto health = target->GetHealth();
    if (!health || health->IsDead()) return;

    health->Heal(amount);

    // Healing puts the source in combat (if target was in combat)
    // This matches AzerothCore behavior - healing pulls you into combat
    auto targetCombat = target->GetCombat();
    if (source && targetCombat && targetCombat->IsInCombat(m_Time)) {
        auto sourceCombat = source->GetCombat();
        if (sourceCombat) {
            sourceCombat->MarkCombat(m_Time);
        }
    }

    // Mark health as dirty for grid-based broadcasting
    MarkHealthDirty(target->GetId());

    GameEvent event;
    event.type = GameEventType::HEAL;
    event.sourceId = source ? source->GetId() : INVALID_ENTITY_ID;
    event.targetId = target->GetId();
    event.abilityId = abilityId;
    event.value = amount;
    event.position = target->GetMovement() ? target->GetMovement()->position : Vec2();
    BroadcastEvent(event);
}

void MapInstance::ApplyAura(Entity* source, Entity* target, const SpellEffect& effect, AbilityId abilityId) {
    if (!target) return;

    auto auras = target->GetAuras();
    if (!auras) return;

    // Create the aura
    Aura aura;
    aura.sourceAbility = abilityId;
    aura.casterId = source ? source->GetId() : INVALID_ENTITY_ID;
    aura.type = effect.auraType;
    aura.value = effect.auraValue;
    aura.damageType = effect.damageType;
    aura.duration = effect.auraDuration;
    aura.maxDuration = effect.auraDuration;
    aura.tickInterval = effect.auraTickInterval;
    aura.tickTimer = 0.0f;

    // Add the aura
    uint32_t auraId = auras->AddAura(aura);
    aura.id = auraId;  // Update with assigned ID

    std::cout << "[Aura] Applied " << static_cast<int>(effect.auraType)
              << " to " << target->GetName() << " (ID: " << auraId
              << ", duration: " << effect.auraDuration << "s)" << std::endl;

    // Broadcast to nearby players
    BroadcastAuraUpdate(target->GetId(), aura, AuraUpdateType::ADD);
}

void MapInstance::BroadcastAuraUpdate(EntityId targetId, const Aura& aura, AuraUpdateType updateType) {
    // Get target position for distance-based sending
    Entity* target = GetEntity(targetId);
    Vec2 targetPos = {0, 0};
    if (target && target->GetMovement()) {
        targetPos = target->GetMovement()->position;
    }

    // Queue for WorldServer to send to nearby players
    PendingAuraUpdate update;
    update.targetId = targetId;
    update.targetPosition = targetPos;
    update.updateType = updateType;
    update.aura = aura;
    m_PendingAuraUpdates.push_back(update);
}

void MapInstance::SendAllAuras(EntityId targetId, uint32_t peerId) {
    Entity* entity = GetEntity(targetId);
    if (!entity) return;

    auto auras = entity->GetAuras();
    if (!auras) return;

    S_AuraUpdateAll packet;
    packet.targetId = targetId;

    for (const auto& aura : auras->GetAuras()) {
        AuraInfo info;
        info.auraId = aura.id;
        info.sourceAbility = aura.sourceAbility;
        info.auraType = static_cast<uint8_t>(aura.type);
        info.value = aura.value;
        info.duration = aura.duration;
        info.maxDuration = aura.maxDuration;
        info.casterId = aura.casterId;
        packet.auras.push_back(info);
    }

    // This would be sent via WorldServer - for now just log
    std::cout << "[Aura] Sending " << packet.auras.size() << " auras for entity "
              << targetId << " to peer " << peerId << std::endl;
}

void MapInstance::BroadcastEvent(const GameEvent& event) {
    m_PendingEvents.push_back(event);
}

std::vector<EntityState> MapInstance::GetWorldStateForPlayer(EntityId playerId) {
    std::vector<EntityState> states;

    Entity* player = GetEntity(playerId);
    if (!player || !player->GetMovement()) return states;

    Vec2 playerPos = player->GetMovement()->position;

    for (auto& [id, entity] : m_Entities) {
        auto movement = entity->GetMovement();
        if (!movement) continue;

        float distance = Vec2::Distance(playerPos, movement->position);
        if (distance > VIEW_DISTANCE && id != playerId) {
            continue;
        }

        EntityState state;
        state.id = id;
        state.type = entity->GetType();
        state.position = movement->position;
        state.rotation = movement->rotation;
        state.moveState = movement->moveState;

        auto health = entity->GetHealth();
        if (health) {
            state.health = health->current;
            state.maxHealth = health->max;
        }

        auto mana = entity->GetMana();
        if (mana) {
            state.mana = mana->current;
            state.maxMana = mana->max;
        }

        auto combat = entity->GetCombat();
        if (combat) {
            state.targetId = combat->targetId;
            state.isCasting = combat->IsCasting();
            if (state.isCasting) {
                state.castingAbilityId = combat->currentCast;
                state.castProgress = combat->castDuration > 0 ?
                    combat->castTimer / combat->castDuration : 0.0f;
            }
        }

        states.push_back(state);
    }

    return states;
}

// ============================================================
// LOOT SYSTEM
// ============================================================

void MapInstance::GenerateLoot(Entity* mob, EntityId killerEntityId) {
    if (!mob) return;

    // Find the creature template and spawn point for this mob
    auto it = m_EntityToSpawnPoint.find(mob->GetId());
    const CreatureTemplate* tmpl = nullptr;
    const MobSpawnPoint* spawnPoint = nullptr;

    // Try to find the template by checking the spawn point
    if (it != m_EntityToSpawnPoint.end()) {
        for (const auto& spawn : m_Template->mobSpawns) {
            if (spawn.id == it->second) {
                tmpl = CreatureTemplates::GetTemplate(spawn.creatureTemplateId);
                spawnPoint = &spawn;
                break;
            }
        }
    }

    // Generate loot
    LootData loot;
    loot.corpseId = mob->GetId();
    loot.killerEntityId = killerEntityId;
    loot.moneyLooted = false;

    // Get corpse decay time: spawn override > template override > rank default
    if (spawnPoint) {
        loot.despawnTimer = spawnPoint->GetCorpseDecayTime(tmpl);
    } else if (tmpl) {
        loot.despawnTimer = tmpl->GetCorpseDecayTime();
    } else {
        loot.despawnTimer = GetDefaultCorpseDecayTime(CreatureRank::NORMAL);
    }

    // Random money in range
    if (tmpl && tmpl->maxMoney > 0) {
        uint32_t range = tmpl->maxMoney - tmpl->minMoney;
        loot.money = tmpl->minMoney + (range > 0 ? (rand() % (range + 1)) : 0);
    } else {
        // Default money if no template found
        loot.money = 10 + (rand() % 20);
    }

    // Roll for item drops from loot table
    // Assign stable slot IDs as items are added (AzerothCore style)
    uint8_t nextSlotId = 0;
    if (tmpl && !tmpl->lootTable.empty()) {
        for (const auto& entry : tmpl->lootTable) {

            // Roll for drop chance (0-100)
            float roll = static_cast<float>(rand() % 10000) / 100.0f;
            if (roll < entry.dropChance) {
                LootItem item;
                item.slotId = nextSlotId++;  // Server-assigned stable slot ID
                item.templateId = entry.itemId;
                item.stackCount = entry.minCount;
                if (entry.maxCount > entry.minCount) {
                    item.stackCount += rand() % (entry.maxCount - entry.minCount + 1);
                }
                item.looted = false;
                loot.items.push_back(item);

                const ItemTemplate* itemTmpl = ItemTemplateManager::Instance().GetTemplate(entry.itemId);
                std::cout << "[Loot] Dropped item: " << (itemTmpl ? itemTmpl->name : "Unknown")
                          << " (ID: " << entry.itemId << ") slot=" << (int)item.slotId
                          << " x" << item.stackCount << std::endl;
            }
        }
    }

    m_Lootables[mob->GetId()] = loot;
    std::cout << "[Loot] Generated " << loot.money << " copper, " << loot.items.size()
              << " items from " << mob->GetName()
              << " (corpse decay: " << loot.despawnTimer << "s)" << std::endl;
}

void MapInstance::AwardXP(Entity* player, Entity* mob) {
    if (!player || !mob || !player->GetExperience()) return;

    // Find the creature template
    auto it = m_EntityToSpawnPoint.find(mob->GetId());
    const CreatureTemplate* tmpl = nullptr;

    if (it != m_EntityToSpawnPoint.end()) {
        for (const auto& spawn : m_Template->mobSpawns) {
            if (spawn.id == it->second) {
                tmpl = CreatureTemplates::GetTemplate(spawn.creatureTemplateId);
                break;
            }
        }
    }

    if (!tmpl) {
        std::cout << "[XP] Could not find creature template for mob" << std::endl;
        return;
    }

    // Calculate XP with level scaling (WoW-style)
    uint32_t baseXP = tmpl->baseXP;
    float levelMod = ExperienceComponent::GetLevelModifier(
        static_cast<int8_t>(player->GetLevel()),
        static_cast<int8_t>(tmpl->level)
    );

    uint32_t xpGained = static_cast<uint32_t>(baseXP * levelMod);

    // Grey mobs give no XP
    if (xpGained == 0) {
        std::cout << "[XP] " << player->GetName() << " killed grey mob " << mob->GetName()
                  << " (no XP)" << std::endl;
        return;
    }

    // Give XP to player
    uint32_t levelsGained = player->GiveXP(xpGained);

    std::cout << "[XP] " << player->GetName() << " gained " << xpGained << " XP from "
              << mob->GetName() << " (level " << (int)tmpl->level << ")"
              << " - now " << player->GetExperience()->current << "/"
              << player->GetXPForNextLevel() << std::endl;

    // Queue XP gain event for network broadcast
    GameEvent xpEvent;
    xpEvent.type = GameEventType::XP_GAIN;
    xpEvent.sourceId = player->GetId();
    xpEvent.targetId = mob->GetId();
    xpEvent.value = static_cast<int32_t>(xpGained);
    xpEvent.position = player->GetMovement() ? player->GetMovement()->position : Vec2();
    BroadcastEvent(xpEvent);

    // If player leveled up, broadcast level up event
    if (levelsGained > 0) {
        GameEvent levelEvent;
        levelEvent.type = GameEventType::LEVEL_UP;
        levelEvent.sourceId = player->GetId();
        levelEvent.targetId = INVALID_ENTITY_ID;
        levelEvent.value = static_cast<int32_t>(player->GetLevel());
        levelEvent.position = player->GetMovement() ? player->GetMovement()->position : Vec2();
        BroadcastEvent(levelEvent);
    }
}

LootData* MapInstance::GetLoot(EntityId corpseId) {
    auto it = m_Lootables.find(corpseId);
    if (it != m_Lootables.end()) {
        return &it->second;
    }
    return nullptr;
}

bool MapInstance::HasLoot(EntityId corpseId) const {
    auto it = m_Lootables.find(corpseId);
    return it != m_Lootables.end() && !it->second.IsEmpty();
}

bool MapInstance::TakeLootMoney(EntityId corpseId, EntityId playerId) {
    auto* loot = GetLoot(corpseId);
    if (!loot || loot->moneyLooted || loot->money == 0) {
        return false;
    }

    // Verify the player is the killer (has loot rights)
    if (loot->killerEntityId != playerId) {
        return false;
    }

    // Get the player entity
    Entity* player = GetEntity(playerId);
    if (!player || !player->GetWallet()) {
        return false;
    }

    // Give money to player
    player->GetWallet()->AddMoney(loot->money);
    std::cout << "[Loot] Player " << player->GetName() << " looted " << loot->money << " copper" << std::endl;

    loot->money = 0;
    loot->moneyLooted = true;

    return true;
}

bool MapInstance::TakeLootItem(EntityId corpseId, EntityId playerId, uint8_t lootSlot, ItemInstance& outItem, uint8_t* outInventorySlot) {
    auto* loot = GetLoot(corpseId);
    if (!loot) {
        return false;
    }

    // Verify the player has loot rights
    if (loot->killerEntityId != playerId) {
        return false;
    }

    // Direct lookup by server-assigned slot ID (AzerothCore style)
    // No conversion needed - client sends back the exact slot ID we gave it
    LootItem* lootItem = loot->GetItemBySlot(lootSlot);
    if (!lootItem) {
        std::cout << "[Loot] Invalid slot ID: " << (int)lootSlot << std::endl;
        return false;
    }

    // Get the player entity
    Entity* player = GetEntity(playerId);
    if (!player || !player->GetInventory()) {
        return false;
    }

    // Check if player has inventory space
    if (player->GetInventory()->IsFull()) {
        std::cout << "[Loot] Player " << player->GetName() << " inventory full, cannot loot item" << std::endl;
        return false;
    }

    // Create item instance
    static ItemInstanceId s_NextInstanceId = 100000;  // Start high to avoid conflicts with DB
    outItem.instanceId = s_NextInstanceId++;
    outItem.templateId = lootItem->templateId;
    outItem.stackCount = lootItem->stackCount;

    // Add to player inventory
    uint8_t inventorySlot;
    if (!player->GetInventory()->AddItem(outItem, &inventorySlot)) {
        return false;
    }

    // Return the inventory slot if requested
    if (outInventorySlot) {
        *outInventorySlot = inventorySlot;
    }

    // Mark as looted
    lootItem->looted = true;

    const ItemTemplate* tmpl = ItemTemplateManager::Instance().GetTemplate(lootItem->templateId);
    std::cout << "[Loot] Player " << player->GetName() << " looted "
              << (tmpl ? tmpl->name : "Unknown Item") << " (slot=" << (int)lootSlot
              << ") to inventory slot " << (int)inventorySlot << std::endl;

    return true;
}

void MapInstance::UpdateLoot(float dt) {
    // Update corpse decay timers
    // Corpses stay visible until their decay timer expires, regardless of loot state
    // This matches AzerothCore behavior: corpse decay is independent of looting
    for (auto it = m_Lootables.begin(); it != m_Lootables.end();) {
        it->second.despawnTimer -= dt;

        // Only remove when corpse decay timer expires (NOT when loot is empty)
        if (it->second.despawnTimer <= 0.0f) {
            // Corpse decay complete - schedule respawn
            EntityId corpseId = it->first;
            auto spawnIt = m_EntityToSpawnPoint.find(corpseId);
            if (spawnIt != m_EntityToSpawnPoint.end()) {
                // Find spawn point info and get resolved respawn time
                for (const auto& spawn : m_Template->mobSpawns) {
                    if (spawn.id == spawnIt->second) {
                        const CreatureTemplate* tmpl = CreatureTemplates::GetTemplate(spawn.creatureTemplateId);
                        float respawnTime = spawn.GetRespawnTime(tmpl);

                        PendingRespawn respawn;
                        respawn.spawnPointId = spawn.id;
                        respawn.respawnAt = m_Time + respawnTime;
                        m_PendingRespawns.push_back(respawn);

                        std::cout << "[Respawn] Corpse decayed, scheduling " << (tmpl ? tmpl->name : "mob")
                                  << " respawn in " << respawnTime << "s" << std::endl;
                        break;
                    }
                }
            }

            // Remove corpse entity and loot data
            RemoveEntity(corpseId);
            it = m_Lootables.erase(it);
        } else {
            ++it;
        }
    }
}

void MapInstance::UpdateRegeneration(float dt) {
    // AzerothCore-style tick-based regeneration (every 2 seconds)
    m_RegenTimer += dt;
    if (m_RegenTimer < REGEN_TICK_INTERVAL) {
        return;
    }
    m_RegenTimer -= REGEN_TICK_INTERVAL;

    for (auto& [id, entity] : m_Entities) {
        auto health = entity->GetHealth();
        auto mana = entity->GetMana();
        auto combat = entity->GetCombat();

        if (!health || health->IsDead()) continue;

        bool healthChanged = false;
        bool manaChanged = false;

        if (entity->GetType() == EntityType::PLAYER) {
            // Player health regen: only out of combat
            if (combat && !combat->IsInCombat(m_Time)) {
                if (health->current < health->max) {
                    int32_t regenAmount = static_cast<int32_t>(health->max * PLAYER_HEALTH_REGEN_PCT);
                    regenAmount = std::max(1, regenAmount);  // At least 1 HP
                    health->Heal(regenAmount);
                    healthChanged = true;
                }
            }

            // Player mana regen: only after 5-second rule (no mana used recently)
            if (mana && combat && combat->CanRegenMana(m_Time)) {
                if (mana->current < mana->max) {
                    int32_t regenAmount = static_cast<int32_t>(mana->max * PLAYER_MANA_REGEN_PCT);
                    regenAmount = std::max(1, regenAmount);  // At least 1 MP
                    mana->RestoreMana(regenAmount);
                    manaChanged = true;
                }
            }
        } else if (entity->GetType() == EntityType::MOB) {
            // Mob health regen: only while evading (returning home)
            auto aggro = entity->GetAggro();
            if (aggro && aggro->isEvading) {
                if (health->current < health->max) {
                    int32_t regenAmount = static_cast<int32_t>(health->max * MOB_EVADE_REGEN_PCT);
                    regenAmount = std::max(1, regenAmount);
                    health->Heal(regenAmount);
                    healthChanged = true;
                }
            }
        }

        // Mark dirty for network sync
        if (healthChanged) {
            MarkHealthDirty(id);
        }
        if (manaChanged) {
            MarkManaDirty(id);
        }
    }
}

// ============================================================
// AURA PROCESSING
// ============================================================

void MapInstance::UpdateAuras(float dt) {
    for (auto& [id, entity] : m_Entities) {
        auto auras = entity->GetAuras();
        auto health = entity->GetHealth();

        if (!auras) continue;

        std::vector<uint32_t> expiredAuras;

        for (auto& aura : auras->GetAuras()) {
            // Update duration
            aura.duration -= dt;
            if (aura.duration <= 0.0f) {
                expiredAuras.push_back(aura.id);
                continue;
            }

            // Process periodic effects
            if (aura.tickInterval > 0.0f) {
                aura.tickTimer += dt;
                while (aura.tickTimer >= aura.tickInterval) {
                    aura.tickTimer -= aura.tickInterval;

                    switch (aura.type) {
                        case AuraType::PERIODIC_DAMAGE: {
                            if (health && !health->IsDead()) {
                                Entity* caster = GetEntity(aura.casterId);
                                ApplyDamage(caster, entity.get(), aura.value, aura.sourceAbility, aura.damageType);
                            }
                            break;
                        }

                        case AuraType::PERIODIC_HEAL: {
                            if (health && !health->IsDead()) {
                                Entity* caster = GetEntity(aura.casterId);
                                ApplyHeal(caster, entity.get(), aura.value, aura.sourceAbility);
                            }
                            break;
                        }

                        case AuraType::PERIODIC_MANA: {
                            auto mana = entity->GetMana();
                            if (mana) {
                                mana->RestoreMana(aura.value);
                                MarkManaDirty(id);
                            }
                            break;
                        }

                        default:
                            break;
                    }
                }
            }
        }

        // Remove expired auras and broadcast
        for (uint32_t auraId : expiredAuras) {
            // Create a minimal aura for the remove broadcast
            Aura expiredAura;
            expiredAura.id = auraId;
            expiredAura.sourceAbility = AbilityId::NONE;
            expiredAura.casterId = INVALID_ENTITY_ID;

            auras->RemoveAura(auraId);
            std::cout << "[Aura] Expired aura " << auraId << " on " << entity->GetName() << std::endl;

            // Broadcast removal
            BroadcastAuraUpdate(id, expiredAura, AuraUpdateType::REMOVE);
        }
    }
}

// ============================================================
// GRID ACTIVATION SYSTEM (AzerothCore-style)
// ============================================================

void MapInstance::UpdateGridActivation(float dt) {
    std::vector<CellCoord> cellsToLoad;
    std::vector<CellCoord> cellsToUnload;

    m_Grid.UpdateGridActivation(dt, cellsToLoad, cellsToUnload);

    // Spawn mobs for newly activated cells
    for (const CellCoord& coord : cellsToLoad) {
        SpawnCellMobs(coord);
    }

    // Despawn mobs for deactivated cells
    for (const CellCoord& coord : cellsToUnload) {
        DespawnCellMobs(coord);
    }
}

void MapInstance::SpawnCellMobs(CellCoord coord) {
    const std::unordered_set<uint32_t>* spawnPoints = m_Grid.GetCellSpawnPoints(coord);
    if (!spawnPoints) return;

    std::cout << "[Grid] Loading cell (" << coord.x << ", " << coord.y
              << ") with " << spawnPoints->size() << " spawn points" << std::endl;

    for (uint32_t spawnPointId : *spawnPoints) {
        // Find spawn point in template
        for (const auto& spawn : m_Template->mobSpawns) {
            if (spawn.id == spawnPointId) {
                // Check if there's already a mob from this spawn point (could be dead corpse)
                bool alreadySpawned = false;
                for (const auto& [entityId, spawnId] : m_EntityToSpawnPoint) {
                    if (spawnId == spawnPointId) {
                        alreadySpawned = true;
                        break;
                    }
                }

                // Check if respawn is pending
                for (const auto& respawn : m_PendingRespawns) {
                    if (respawn.spawnPointId == spawnPointId) {
                        alreadySpawned = true;
                        break;
                    }
                }

                if (!alreadySpawned) {
                    Entity* mob = CreateMob(spawn.creatureTemplateId, spawn.position, spawn.id);
                    if (mob) {
                        // Register with grid (visibility system handles network broadcast)
                        m_Grid.RegisterSpawnedMob(coord, mob->GetId());
                        std::cout << "[Grid] Spawned " << mob->GetName() << " (spawn point "
                                  << spawnPointId << ")" << std::endl;
                    }
                }
                break;
            }
        }
    }

    // Mark cell as active
    m_Grid.ActivateCell(coord);
}

void MapInstance::DespawnCellMobs(CellCoord coord) {
    std::vector<EntityId> mobsToDespawn = m_Grid.GetCellSpawnedMobs(coord);

    std::cout << "[Grid] Unloading cell (" << coord.x << ", " << coord.y
              << ") with " << mobsToDespawn.size() << " mobs" << std::endl;

    for (EntityId mobId : mobsToDespawn) {
        Entity* mob = GetEntity(mobId);
        if (!mob) continue;

        // If mob is dead (corpse), let the loot system handle it
        if (mob->GetHealth() && mob->GetHealth()->IsDead()) {
            std::cout << "[Grid] Skipping dead mob " << mobId << " (corpse)" << std::endl;
            continue;
        }

        // Remove AI
        m_MobAIs.erase(mobId);

        // Track for respawn later if grid reactivates
        auto spawnIt = m_EntityToSpawnPoint.find(mobId);
        if (spawnIt != m_EntityToSpawnPoint.end()) {
            // Don't schedule respawn - will spawn fresh when grid reactivates
            m_EntityToSpawnPoint.erase(spawnIt);
        }

        // Remove entity (visibility system handles network broadcast)
        RemoveEntity(mobId);

        std::cout << "[Grid] Despawned mob " << mobId << std::endl;
    }

    // Deactivate cell
    m_Grid.DeactivateCell(coord);
}

// ============================================================
// DIRTY FLAG HELPERS
// ============================================================

void MapInstance::MarkPositionDirty(EntityId id) {
    DirtyFlags flags;
    flags.position = true;
    m_Grid.MarkDirty(id, flags);
}

void MapInstance::MarkHealthDirty(EntityId id) {
    DirtyFlags flags;
    flags.health = true;
    m_Grid.MarkDirty(id, flags);
}

void MapInstance::MarkManaDirty(EntityId id) {
    DirtyFlags flags;
    flags.mana = true;
    m_Grid.MarkDirty(id, flags);
}

void MapInstance::MarkTargetDirty(EntityId id) {
    DirtyFlags flags;
    flags.target = true;
    m_Grid.MarkDirty(id, flags);
}

void MapInstance::MarkCastingDirty(EntityId id) {
    DirtyFlags flags;
    flags.casting = true;
    m_Grid.MarkDirty(id, flags);
}

void MapInstance::MarkMoveStateDirty(EntityId id) {
    DirtyFlags flags;
    flags.moveState = true;
    m_Grid.MarkDirty(id, flags);
}

} // namespace MMO
