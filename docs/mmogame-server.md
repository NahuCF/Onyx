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
| `Map/` | `MapDefines.h/.cpp`, `MapInstance.h/.cpp`, `MapManager.h/.cpp` |
| `Grid/` | `Grid.h/.cpp`, `GridCell.h`, `GridDefines.h` |
| `AI/` | `CreatureAI.h/.cpp`, `CreatureScript.h`, `CreatureScripts.cpp`, `BuiltInAI.h`, `InstanceScript.h`, `InstanceScripts.cpp`, `EventMap.h`, `AIDefines.h`, `ConditionEvaluator.h`, `CreatureTemplate.h`, `CreatureTemplates.h/.cpp`, `SummonList.h` |
| `Scripting/` | `IEntity.h`, `IMapContext.h`, `GameObjectScript.h/.cpp`, `QuestScript.h/.cpp`, `SpellScript.h/.cpp`, `PlayerScript.h/.cpp` |
| `Scripts/` | Hand-written boss AIs — `ShadowLordAI.h` |
| `Triggers/` | `TriggerScript.h`, `TriggerScripts.cpp` |
| top-level | `WorldServer.h/.cpp`, `Main.cpp` |

## Entity components (`Entity/Components.h`)

All POD-style structs. `Entity` (`Entity.h`) owns optional component pointers via factory methods (`AddHealthComponent`, etc.). `Entity` inherits `IEntity` (see Scripting Interfaces below).

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
    std::vector<ServerTriggerVolume> triggerVolumes;
    std::string instanceScriptName;   // maps to map_template.instance_script column
};
```

`MapInstance` (runtime instance) — also implements `IMapContext`:
- Constructor: `MapInstance(uint32_t instanceId, const MapTemplate* tmpl)`.
- Lifecycle: `CreatePlayer`, `CreateMob`, `RemoveEntity`, `Update(dt)`.
- Queries: `GetEntity`, `GetEntitiesInRadius`, `GetPlayersInRadius`.
- Loot: `GenerateLoot(mob, killerEntityId)`, `TakeLootMoney`, `TakeLootItem`.
- Projectiles: `SpawnProjectile`, `UpdateProjectiles`, `GetProjectiles`.
- Grid hooks: `GetGrid`, `MarkPositionDirty`, dirty-flag set helpers.
- Scripting: `FireTriggerScript(entity, volume)` dispatches via cached `resolvedScript` pointer.
- Dungeon: owns `unique_ptr<InstanceState>` if `instanceScriptName` is set; forwards player enter/leave, creature death, and area trigger events to it.

`MapManager` (singleton):
- `Initialize(Database&)` — loads map templates, portals, creature spawns, and trigger volumes from DB; resolves all script pointers at boot.
- `GetMapInstance(templateId)`, `GetInstanceById(instanceId)`, `GetTemplate(templateId)`.
- `TransferPlayer(playerId, destMapId, destPosition)` — inter-map transfer.

Maps are fully DB-driven. Templates are authored in MMOEditor3D (see [editor3d.md](editor3d.md), [release-pipeline.md](release-pipeline.md)).

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

## Scripting system

The scripting system is split across three locations:

| Location | Contents |
|---|---|
| `Shared/Source/Scripting/` | `ScriptObject`, `ScriptRegistry<T>`, `HookRegistry<T>` — used by all script types |
| `WorldServer/Source/Scripting/` | `IEntity`, `IMapContext` interfaces; `GameObjectScript`, `QuestScript`, `SpellScript`, `PlayerScript` |
| `WorldServer/Source/AI/` | `CreatureScript`, `CreatureAI`, `BuiltInAI`, `InstanceScript`/`InstanceState` |
| `WorldServer/Source/Triggers/` | `TriggerScript` |

### Base types (`Shared/Source/Scripting/`)

**`ScriptObject`** — base class for all script singletons. Holds a name string; `GetName() → string_view`.

**`ScriptRegistry<T>`** (C++20 concept: `T` must derive `ScriptObject`) — Meyers singleton owning one `unique_ptr<T>` per name:
```cpp
template<typename T> requires std::derived_from<T, ScriptObject>
class ScriptRegistry {
    T* Register<U>();          // construct U in-place, return raw ptr
    T* Get(string_view name);
    void ForEach(auto&& fn);
    size_t Size();
};
```
One `ScriptRegistry` instance per script type (`CreatureScript`, `TriggerScript`, `InstanceScript`, etc.).

**`HookRegistry<T>`** — non-owning broadcast registry. Used by `PlayerScript` so multiple subscribers can receive the same event:
```cpp
void Subscribe(T* script);
template<auto Method, typename... Args>
void Dispatch(Args&&... args);   // calls Method on every subscriber
```

### Scripting interfaces (`WorldServer/Source/Scripting/`)

Scripts see only `IEntity&` and `IMapContext&` — never `Entity*` or `MapInstance*`. This decouples scripts from server internals and allows unit testing with stubs.

**`IEntity`** — what scripts can read/mutate on a mob or player:
```cpp
EntityId GetId() const;
const std::string& GetName() const;
EntityType GetType() const;
Vec2 GetPosition() const;
float GetHeight() const;
float GetHealthPercent() const;  // 0.0–1.0
float GetManaPercent() const;
bool IsDead() const;
uint32_t AddAura(const Aura& aura);
void RemoveAura(uint32_t auraId);
void RemoveAurasByType(AuraType);
bool HasAuraType(AuraType) const;
```

**`IMapContext`** — what scripts can do at map level:
```cpp
std::string_view GetMapName() const;
float GetTime() const;
IEntity* GetEntity(EntityId);
IEntity* SummonCreature(uint32_t templateId, Vec2 position, EntityId summoner);
void RemoveEntity(EntityId);
void ProcessAbility(EntityId source, EntityId target, AbilityId);
```

`Entity` implements `IEntity`; `MapInstance` implements `IMapContext` (covariant `GetEntity` return: `Entity*` overrides `IEntity*`).

### Script types

#### `CreatureScript` + `CreatureAI` (`AI/`)

**`CreatureScript`** — registered singleton factory, one per named script:
```cpp
class CreatureScript : public ScriptObject {
    virtual unique_ptr<CreatureAI> CreateAI(IMapContext& map, IEntity& mob,
                                             const CreatureTemplate* tmpl) const = 0;
};
```

**`CreatureAI`** — per-mob runtime (NOT a ScriptObject). Constructed by `CreateAI`; owned by `MapInstance`:
```cpp
class CreatureAI {
public:
    CreatureAI(IMapContext& ctx, IEntity& owner, const CreatureTemplate* tmpl);

    virtual void OnEnterCombat(IEntity& target);
    virtual void OnEvade();
    virtual void OnDeath(IEntity* killer);
    virtual void OnDamageTaken(IEntity& attacker, int32_t damage);
    virtual void OnSummonDied(IEntity& summon);
    virtual void OnPhaseTransition(uint32_t oldPhase, uint32_t newPhase);
    virtual void Update(float dt, IEntity* target, IMapContext& ctx);

protected:
    IMapContext& m_Ctx;
    IEntity& m_Owner;
    const CreatureTemplate* m_Template;
    EventMap m_Events;
    SummonList m_Summons;
    bool m_InCombat;
    float m_CombatTime;
    uint32_t m_CurrentPhase;
};
```

Phase transitions and ability casts call `m_Ctx.ProcessAbility(...)` and `m_Ctx.SummonCreature(...)` directly — no stored callbacks.

**Three-step mob AI resolution** at `MapInstance::CreateMob`:
1. `tmpl->resolvedScript` set → `resolvedScript->CreateAI(map, mob, tmpl)` (engineer-authored `CreatureScript`)
2. `tmpl->aiName` set → `BuiltInAIRegistry().Get(aiName)->CreateAI(map, mob, tmpl)` (data-driven archetype)
3. Fallback → `make_unique<CreatureAI>(map, mob, tmpl)` (default data-driven AI)

#### `BuiltInAI` (`AI/BuiltInAI.h`)

Registry of data-driven AI archetypes keyed by the `ai_name` DB column (e.g. `"aggressor"`, `"passive"`, `"guard"`). Selected as the middle tier of the three-step resolution above. Concrete archetypes TBD.

#### `TriggerScript` (`Triggers/TriggerScript.h`)

Singleton callback for trigger volumes. Multiple volumes may share one script name:
```cpp
class TriggerScript : public ScriptObject {
    virtual void OnEnter(IMapContext&, IEntity&, const ServerTriggerVolume&) {}
    virtual void OnExit (IMapContext&, IEntity&, const ServerTriggerVolume&) {}
    virtual void OnStay (IMapContext&, IEntity&, const ServerTriggerVolume&) {}
};
```

Dispatch goes through `ServerTriggerVolume::resolvedScript` — a raw pointer cached at boot. No hash lookup per trigger event.

For the full trigger volume system see [trigger-volumes.md](trigger-volumes.md).

#### `InstanceScript` + `InstanceState` (`AI/InstanceScript.h`)

**`InstanceScript`** — factory singleton registered by dungeon map name:
```cpp
class InstanceScript : public ScriptObject {
    virtual unique_ptr<InstanceState> CreateState(IMapContext&) const = 0;
};
```

**`InstanceState`** — per-`MapInstance` runtime for dungeon encounter logic:
```cpp
class InstanceState {
    virtual void OnInstanceCreate(IMapContext&);
    virtual void OnPlayerEnter(IEntity&);
    virtual void OnPlayerLeave(IEntity&);
    virtual void OnCreatureCreate(IEntity&);
    virtual void OnCreatureDeath(IEntity&, IEntity* killer);
    virtual void OnAreaTrigger(IEntity&, const ServerTriggerVolume&);
    virtual void Update(float dt);
};
```

`MapInstance` constructs the state in its constructor (if `tmpl->instanceScriptName` is set) and forwards events to it each tick.

#### Global hook scripts (`Scripting/`)

`PlayerScript`, `GameObjectScript`, `QuestScript`, `SpellScript` — all inherit `ScriptObject` and use `ScriptRegistry<T>`. `PlayerScript` additionally uses `HookRegistry<PlayerScript>` for multi-subscriber broadcast. All are stub-registered at startup; concrete implementations TBD.

### Script resolution and registration

**Boot-time resolution** (`MapManager::Initialize`, after all templates are loaded):
- Iterates all `ServerTriggerVolume` entries → caches `TriggerScript*` on `vol.resolvedScript`.
- Iterates all `CreatureTemplate` entries → caches `CreatureScript*` on `tmpl->resolvedScript`.
- Logs a warning for any named script that has no registered implementation.

**Registration order** in `WorldServer::Initialize` (before `MapManager::Initialize`):
```cpp
RegisterAllCreatureScripts();
RegisterAllInstanceScripts();
RegisterAllTriggerScripts();
RegisterAllGameObjectScripts();
RegisterAllQuestScripts();
RegisterAllSpellScripts();
RegisterAllPlayerScripts();
```

Each `RegisterAll*` function lives in its corresponding `*Scripts.cpp` file and calls `ScriptRegistry<T>::Instance().Register<ConcreteClass>()`.

### Adding a new creature script

1. Create `WorldServer/Source/Scripts/MyBossAI.h`:
   ```cpp
   class MyBossAI : public CreatureAI {
   public:
       MyBossAI(IMapContext& ctx, IEntity& owner, const CreatureTemplate* tmpl)
           : CreatureAI(ctx, owner, tmpl) {}
       void OnEnterCombat(IEntity& target) override { ... }
   };

   class MyBossScript : public CreatureScript {
   public:
       MyBossScript() : CreatureScript("my_boss") {}
       unique_ptr<CreatureAI> CreateAI(IMapContext& map, IEntity& mob,
                                        const CreatureTemplate* tmpl) const override {
           return make_unique<MyBossAI>(map, mob, tmpl);
       }
   };
   ```
2. Add `reg.Register<MyBossScript>();` in `CreatureScripts.cpp`.
3. Set `scripts = 'my_boss'` on the `creature_template` DB row.

## AI data model (`AI/`)

### `EventMap`

Timer-based scheduler with phase masks.
- `ScheduleEvent(eventId, delay, phaseMask)`, `CancelEvent`, `ExecuteEvent` (returns `id+1` or `0`).
- Phase ops: `SetPhase`, `AddPhase`, `RemovePhase`, `IsInPhase`.

### Conditions (`AIDefines.h`)

`ConditionType`: `HEALTH_BELOW`, `HEALTH_ABOVE`, `MANA_BELOW`, `MANA_ABOVE`, `TARGET_HEALTH_BELOW`, `TARGET_HEALTH_ABOVE`, `IN_RANGE`, `OUT_OF_RANGE`, `COMBAT_TIME_ABOVE`, `COMBAT_TIME_BELOW`, `SUMMONS_ALIVE`, `NO_SUMMONS_ALIVE`, `HAS_BUFF`, `NOT_HAS_BUFF`, `RANDOM_CHANCE`.

`ConditionEvaluator::Evaluate(cond, self, target, combatTime, hasSummons)` — all parameters are `IEntity*`.

### `AbilityRule`

```cpp
struct AbilityRule {
    AbilityId ability;
    float cooldown;
    float initialDelay;
    uint32_t phaseMask;
    std::vector<Condition> conditions;
};
```

### `PhaseTrigger`

```cpp
struct PhaseTrigger {
    int fromPhase, toPhase;
    PhaseTriggerType type;   // HEALTH_BELOW, SUMMON_DIED, etc.
    float value;
    AbilityId castOnTransition;
    uint32_t summonCreatureId;
    int summonCount;
    float summonSpacing;
};
```

### `CreatureTemplate` (`AI/CreatureTemplate.h`)

```cpp
struct CreatureTemplate {
    uint32_t id;
    std::string name;
    uint8_t level;
    int32_t maxHealth, maxMana;
    float speed, aggroRadius, leashRadius;
    uint32_t baseXP, minMoney, maxMoney;
    CreatureRank rank;
    float corpseDecayTime, respawnTime;
    std::vector<AbilityRule> abilities;
    std::vector<PhaseTrigger> phaseTriggers;
    uint32_t initialPhase;
    std::vector<LootTableEntry> lootTable;

    std::string scriptName;         // 'scripts' DB column → CreatureScript
    std::string aiName;             // 'ai_name' DB column → BuiltInAI archetype
    CreatureScript* resolvedScript; // cached at boot, null if scriptName empty
};
```

## Trigger volumes (`Map/MapDefines.h`)

See [trigger-volumes.md](trigger-volumes.md) for the full system. Summary:

```cpp
struct ServerTriggerVolume {
    std::string guid;
    TriggerShapeKind shape;     // BOX, SPHERE, CAPSULE
    Vec2 position;
    float positionZ, orientation;
    float halfExtentX, halfExtentY, halfExtentZ;
    float radius;
    TriggerEventKind triggerEvent;  // ON_ENTER, ON_EXIT, ON_STAY
    bool triggerOnce, triggerPlayers, triggerCreatures;
    std::string scriptName;
    uint32_t eventId;
    TriggerScript* resolvedScript;  // cached at boot
};
```

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
