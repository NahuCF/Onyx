# MMOGame — Servers

Two binaries:

- **MMOLoginServer** (port 7000) — authentication, character CRUD, session issuance.
- **MMOWorldServer** (port 7001) — game simulation, entity state, AI, combat, abilities.

Patterns are AzerothCore/TrinityCore-inspired (grid visibility, visitor pattern, spell effects, aura ticks, dirty flags). Reference patterns described in `~/.claude/projects/-home-god-git-Onyx/memory/azerothcore-reference.md`.

## LoginServer

`MMOGame/LoginServer/Source/`:
- `Main.cpp` — entry point (`Onyx::CreateApplication` returns `LoginApp`).
- `LoginServer.h/.cpp` — main service.
- `Database.h/.cpp` — pqxx wrapper for accounts, characters, sessions.

### Flow

`LoginServer::Initialize(dbConnString, port = 7000)` connects the DB, populates `GameDataStore` (races/classes/create-info), starts `NetworkServer`. `Run()` loops: poll events (`CONNECTED` / `DISCONNECTED` / `DATA_RECEIVED`), process packets, run periodic `CleanupExpiredSessions` (every 5 minutes).

### Packet handlers

| Packet | Handler |
|---|---|
| `C_REGISTER_REQUEST` | Validate, salt + hash password, `CreateAccount`. |
| `C_LOGIN_REQUEST` | `GetAccountByUsername`, hash compare, `CreateSession`. |
| `C_CREATE_CHARACTER` | Validate via `GameDataStore` (race/class), `IsNameTaken`, `CreateCharacter`, `SaveCharacter`. |
| `C_DELETE_CHARACTER` | `DeleteCharacter`. |
| `C_SELECT_CHARACTER` | Issue auth token for the WorldServer handshake. |

### Database API (`Database.h`)

- Accounts: `GetAccountByUsername`, `CreateAccount`, `UpdateLastLogin`.
- Characters: `GetCharactersByAccountId`, `GetCharacterById`, `CreateCharacter`, `DeleteCharacter`, `SaveCharacter`, `IsNameTaken`.
- Sessions: `CreateSession`, `ValidateSession`, `DeleteSession`, `CleanupExpiredSessions`.

## WorldServer

`MMOGame/WorldServer/Source/`:

| Folder | Purpose |
|---|---|
| `Entity/` | `Entity.h/.cpp`, `Components.h`, `AuraComponent.h` |
| `Map/` | `Map.h`, `MapDefines.h/.cpp`, `MapInstance.h/.cpp`, `MapManager.h/.cpp`, `MapTemplates.h/.cpp` |
| `Grid/` | `Grid.h/.cpp`, `GridCell.h`, `GridDefines.h` |
| `AI/` | `ScriptedAI.h/.cpp`, `EventMap.h`, `AIDefines.h`, `ConditionEvaluator.h`, `CreatureTemplate.h/.cpp`, `CreatureTemplates.cpp`, `DataDrivenAI.h`, `AICallbacks.h`, `SummonList.h`, `ScriptRegistry.h/.cpp` |
| `Scripts/` | `ScriptLoader.h`, hand-written boss scripts (e.g., `ShadowLordAI.h`) |
| top-level | `WorldServer.h/.cpp`, `Main.cpp` |

## Entity components (`Entity/Components.h`)

All POD-style structs. `Entity` (`Entity.h`) owns optional component pointers via factory methods (`AddHealthComponent`, etc.).

| Component | Key state | Primary methods |
|---|---|---|
| `HealthComponent` | `current`, `max`, `baseMax` | `TakeDamage`, `Heal`, `IsDead`, `Percent` |
| `ManaComponent` | `current`, `max`, `baseMax` | `HasMana`, `UseMana`, `RestoreMana`, `Percent` |
| `MovementComponent` | `position`, `velocity`, `rotation`, `speed`, `moveState` | (data-only) |
| `CombatComponent` | `targetId`, `cooldowns`, `currentCast`, `castTimer`, `lastCombatTime` | `IsCasting`, `IsAbilityReady`, `IsInCombat`, `MarkCombat`, `MarkManaUse` |
| `AggroComponent` | `threatTable`, `aggroRadius`, `leashRadius`, `homePosition`, `isEvading` | `GetTopThreat`, `AddThreat`, `RemoveThreat`, `ClearThreat` |
| `AuraComponent` | `m_Auras`, `m_NextAuraId`, `m_Dirty` | see "Aura system" below |
| `InventoryComponent` | `slots: array<InventorySlot, 20>` | `AddItem`, `RemoveItem`, `SwapSlots`, `GetItem`, `FindFirstEmptySlot`, `IsFull`, `GetItemCount` |
| `EquipmentComponent` | `slots: array<ItemInstance, 14>` | `Equip`, `Unequip`, `GetEquipped`, `CanEquipInSlot` |
| `WalletComponent` | `money` | (currency) |
| `ExperienceComponent` | `level`, `xp`, `xpForNextLevel` | WoW-like XP curve |
| `StatsComponent` | base + modifier tiers (str/agi/sta/int/spi/armor) | applies to combat math |

## Map system (`Map/`)

`MapTemplate` (static map definition):
```cpp
struct MapTemplate {
    uint32_t id;
    std::string name;
    int32_t width, height;
    Vec2 spawnPoint;
    std::vector<Portal> portals;
    std::vector<MobSpawnPoint> mobSpawns;
};
```

`MapInstance` (runtime instance):
- Constructor: `MapInstance(uint32_t instanceId, const MapTemplate* tmpl)`.
- Lifecycle: `CreatePlayer`, `CreateMob`, `RemoveEntity`, `Update(dt)`.
- Queries: `GetEntity`, `GetEntitiesInRadius`, `GetPlayersInRadius`.
- Loot: `GenerateLoot(mob, killerEntityId)`, `TakeLootMoney`, `TakeLootItem`.
- Projectiles: `SpawnProjectile`, `UpdateProjectiles`, `GetProjectiles`.
- Grid hooks: `GetGrid`, `MarkPositionDirty`, dirty-flag set helpers.

`MapManager` (singleton):
- `Initialize()`, `GetMapInstance(templateId)`, `GetInstanceById(instanceId)`, `GetTemplate(templateId)`.
- `TransferPlayer(playerId, destMapId, destPosition)` — inter-map transfer.
- `GenerateGlobalEntityId()` — globally unique IDs across maps.

`MapTemplates.cpp` defines:
- **Starting Zone** (ID 1) — 100×100, spawn (10,10), portal to Dark Forest at (45,10), 2 Werewolf spawns.
- **Dark Forest** (ID 2) — 100×100, spawn (20,10), portal back at (10,10), 2 spiders + 1 werewolf + Shadow Lord boss (ID 50, 300s respawn).

## Grid system (`Grid/`)

AzerothCore-style spatial partitioning.

```cpp
constexpr float GRID_CELL_SIZE = 32.0f;
constexpr float VIEW_DISTANCE = 50.0f;
constexpr int32_t GRID_SEARCH_RADIUS = 2;   // cells
```

API (`Grid.h`):
- `void ForEachPlayerNear(Vec2 position, std::function<void(EntityId)> callback)` — visitor over nearby players.
- `void UpdateGridActivation(dt, outCellsToLoad, outCellsToUnload)` — returns cell deltas based on player proximity.
- `IsCellActive`, `ActivateCell`, `DeactivateCell`.

`GridCellState`: `UNLOADED`, `LOADING`, `ACTIVE`, `UNLOADING`. Cells activate within `VIEW_DISTANCE + search radius` of any player; mobs spawn/despawn with cell activation.

### Dirty flags (`GridDefines.h`)

```cpp
struct DirtyFlags {
    bool position, health, mana, target, casting, moveState;
    bool spawned, despawned;
};
```

`Grid::MarkDirty(...)` / `GetDirtyEntities()` drive delta updates to clients.

## AI system (`AI/`)

Data-driven mob AI with a hand-written script layer for bosses.

### `ScriptedAI` (base class)

Hooks: `OnEnterCombat`, `OnEvade`, `OnDeath`, `OnDamageTaken`, `OnSummonDied`, `OnPhaseTransition`.

Main update:
```cpp
void Update(float dt, Entity* target,
            CastAbilityFn cast, SummonCreatureFn summon, DespawnCreatureFn despawn);
```

State accessors: `IsInCombat`, `GetCurrentPhase`, `GetCombatTime`, `GetEvents`, `GetSummons`, `SetPhase`, `CheckPhaseTriggers`, `TransitionToPhase`, `TriggerPhase`.

### `EventMap`

Timer-based scheduler with phase masks.
- `ScheduleEvent(eventId, delay, phaseMask)`, `CancelEvent`, `ExecuteEvent` (returns `id+1` or `0`).
- Phase ops: `SetPhase`, `AddPhase`, `RemovePhase`, `IsInPhase`.

### Conditions (`AIDefines.h`)

`ConditionType`: `HEALTH_BELOW`, `HEALTH_ABOVE`, `MANA_BELOW`, `MANA_ABOVE`, `TARGET_HEALTH_BELOW`, `TARGET_HEALTH_ABOVE`, `IN_RANGE`, `OUT_OF_RANGE`, `COMBAT_TIME_ABOVE`, `COMBAT_TIME_BELOW`, `SUMMONS_ALIVE`, `NO_SUMMONS_ALIVE`, `HAS_BUFF`, `NOT_HAS_BUFF`, `RANDOM_CHANCE`.

### `AbilityRule`

```cpp
struct AbilityRule {
    AbilityId ability;
    float cooldown;
    float initialDelay;
    uint32_t phaseMask;
    std::vector<Condition> conditions;
    AbilityRule& WithCondition(...);
    AbilityRule& WithInitialDelay(float);
};
```

### `PhaseTrigger`

```cpp
struct PhaseTrigger {
    int fromPhase, toPhase;
    PhaseTriggerType type;
    float value;
    AbilityId castOnTransition;
    uint32_t summonCreatureId;
    int summonCount;
    float summonSpacing;
    PhaseTrigger& WithCast(AbilityId);
    PhaseTrigger& WithSummon(uint32_t, int, float);
};
```

`CreatureTemplate` (`AI/CreatureTemplate.h`) bundles base stats + an `AbilityRule` list + `PhaseTrigger` list. Mobs without a hand-written script use the data-driven path; bosses register a `ScriptedAI` subclass via `ScriptRegistry`.

## Combat

Damage types (`Shared/Source/Types/Types.h`):
```cpp
enum class DamageType : uint8_t { PHYSICAL = 0, FIRE = 1, FROST = 2, HOLY = 3 };
```

Projectiles (`Map/MapDefines.h`):
```cpp
struct Projectile {
    EntityId id, sourceId, targetId;
    AbilityId abilityId;
    Vec2 position, targetPosition;
    float speed;
    int32_t damage;
    DamageType damageType = DamageType::PHYSICAL;
    bool isHeal = false;
    AuraType auraType;
    int32_t auraValue;
    float auraDuration;
};
```

Entry points: `MapInstance::SpawnProjectile`, `UpdateProjectiles`, `GetProjectiles`.

## Spell effects and aura system

Spell effects compose abilities (AzerothCore-style). See [mmogame-shared.md](mmogame-shared.md#spells-spellsspelldefinesh) for `AbilityData` and `SpellEffect`.

### `AuraType` (`Shared/Source/Spells/SpellDefines.h`)

| Group | Values |
|---|---|
| Movement | `MOD_SPEED_PCT` |
| Immunity | `DAMAGE_IMMUNITY`, `SCHOOL_IMMUNITY` |
| Damage | `MOD_DAMAGE_TAKEN_PCT`, `PERIODIC_DAMAGE`, `PERIODIC_HEAL`, `PERIODIC_MANA` |
| Stats | `MOD_STAT`, `MOD_HEALTH_PCT`, `MOD_MANA_PCT` |
| Combat power | `MOD_ATTACK_POWER`, `MOD_SPELL_POWER` |
| Crowd control | `STUN`, `ROOT`, `SILENCE` |

### `AuraComponent`

```cpp
uint32_t AddAura(Aura aura);
void RemoveAura(uint32_t auraId);
void RemoveAurasByType(AuraType);
void RemoveAurasByAbility(AbilityId);
void RemoveAurasByCaster(EntityId);
void ClearAllAuras();

bool HasAuraType(AuraType) const;
bool IsImmune(DamageType) const;
bool IsImmuneToSchool(...) const;
float GetSpeedModifier() const;
float GetDamageTakenModifier() const;
bool IsStunned() const;
bool IsRooted() const;
bool IsSilenced() const;

const std::vector<Aura>& GetAuras() const;
bool IsDirty() const;
void ClearDirty();
```

Periodic effects (`PERIODIC_DAMAGE`, `PERIODIC_HEAL`, `PERIODIC_MANA`) tick at configurable intervals.

### Aura packet sync

- `S_AURA_UPDATE` — single aura change with `AuraUpdateType { ADD, REMOVE, REFRESH, STACK }`.
- `S_AURA_UPDATE_ALL` — full aura sync on zone enter.

## Inventory & equipment

`InventoryComponent::slots` — `array<InventorySlot, 20>`. `EquipmentComponent::slots` — `array<ItemInstance, 14>` indexed by `EquipmentSlot` (HEAD, NECK, SHOULDERS, BACK, CHEST, WRISTS, HANDS, WAIST, LEGS, FEET, RING1, RING2, WEAPON, OFFHAND).

## Loot system (`Map/MapDefines.h`)

```cpp
struct LootItem {
    uint8_t slotId;          // server-assigned
    ItemId templateId;
    uint32_t stackCount;
    bool looted;
};

struct LootData {
    EntityId corpseId;
    uint32_t money;
    EntityId killerEntityId;
    float despawnTimer;
    bool moneyLooted;
    std::vector<LootItem> items;
    bool IsEmpty() const;
    int GetUnlootedItemCount() const;
    LootItem* GetItemBySlot(uint8_t slotId);
};
```

Entry points (`MapInstance`): `GenerateLoot(mob, killerEntityId)`, `TakeLootMoney`, `TakeLootItem(slotId)`. Proximity check enforced server-side.

## Tick rates

- World simulation: **20 Hz** state broadcast.
- Client input: **60 Hz**.
- Aura periodic effects: per-aura interval.
