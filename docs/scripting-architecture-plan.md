# Scripting Architecture Refactor — Detailed Plan

Self-contained handoff document: a fresh session can pick this up with zero prior conversation context. Captures the design rationale, the seven-step migration, per-step file lists, acceptance criteria, and the gotchas that drove each decision.

## TL;DR

Onyx currently has **two parallel mini-systems** for scripting (`ScriptedAI` for creatures + `TriggerScript` for trigger volumes). They share no code, use different registration ergonomics, and don't compose. Several feature areas — gameobjects, quests, spells, player hooks, instance encounters — will need scripting soon. Replace both with a unified architecture before more types accrue, so we don't pay six rounds of "ad-hoc copy-paste" later.

The new architecture is modeled on AzerothCore's `ScriptObject` family but uses C++20 idioms (concepts, references, `inline` variables) and adds modern bits AzerothCore lacks (interface decoupling for testability, cached pointers, persistence).

**Estimated total effort: ~1.5 days, broken into 7 mergeable steps.**

---

## Background

### Current state (as of 2026-05-05)

| System | File path | Pattern |
|---|---|---|
| Creature AI | [`MMOGame/WorldServer/Source/AI/`](../MMOGame/WorldServer/Source/AI/) | Per-instance object that IS both the registered script and the runtime AI. Macro auto-registration via `REGISTER_CREATURE_SCRIPT`. Header collated in `Scripts/ScriptLoader.h`. |
| Trigger volumes | [`MMOGame/WorldServer/Source/Triggers/`](../MMOGame/WorldServer/Source/Triggers/) | Stateless singleton script per name. Manual registration in `RegisterAllTriggerScripts()`. |

The two systems have **different registries**, **different registration ergonomics**, **different lifetime models**, **different callback signatures** (`Entity*` vs `Entity&`), and **no shared base type**.

### Why now

Six future feature areas all need scripting hooks:

1. **GameObjects** — doors, chests, mining nodes, mailboxes, banks (per [release-pipeline.md](release-pipeline.md))
2. **Quests** — accept/complete/reward hooks
3. **Spells** — OnCast, OnHit, OnCalculateDamage
4. **Player hooks** — OnLogin, OnLevelUp, OnDeath (cross-cutting)
5. **Instance encounters** — phase state, achievement gating, lockout management
6. **Map-level events** — zone enter/exit, weather, world events

If we let each one accrete its own ad-hoc pattern (copying whichever of the two existing systems "felt close enough"), we end up with eight inconsistent mini-systems. Building the right base now costs ~1.5 days; refactoring later costs much more.

---

## Final Architecture

### Class hierarchy

```
                    ScriptObject (base — name + vtable)
                         │
   ┌──────────┬──────────┼──────────┬──────────┬──────────┐
   ▼          ▼          ▼          ▼          ▼          ▼
CreatureScript  TriggerScript  GameObjectScript  QuestScript  SpellScript  PlayerScript
   │ (factory)        (singleton, callbacks)              (callbacks-only types)
   ▼
CreatureAI ◀──── per-mob runtime instance (NOT a ScriptObject)
   │              created by CreatureScript::CreateAI(map, entity)
   │              owns: m_Owner, m_Events, m_Summons, phase state
   ▼
[concrete: ShadowLordAI, RagnarosAI, …]


InstanceScript ◀── per-MapInstance singleton (one per dungeon run)
                   lifecycle bound to MapInstance
                   stateful, persistent
```

### Two dispatch mechanisms

**Direct dispatch** (1 script attached to 1 data row):

```cpp
template<typename T> requires std::derived_from<T, ScriptObject>
class ScriptRegistry {
public:
    static ScriptRegistry& Instance();              // Meyers singleton
    template<typename U> void Register();           // construct + insert
    T* Get(std::string_view name) const;            // O(1) hash lookup
    void ForEach(auto&& fn) const;
    size_t Size() const;
    void Clear();                                   // hot-reload seam
};
```

**Global hook bus** (many scripts subscribe to one event):

```cpp
template<typename T> requires std::derived_from<T, ScriptObject>
class HookRegistry {
public:
    static HookRegistry& Instance();
    void Subscribe(T* script);                      // just stores ptr; ScriptRegistry owns lifetime
    template<typename Event, typename... Args>
    void Dispatch(Args&&... args);                  // invokes Event(args...) on every script
    void Clear();
};
```

`ScriptRegistry<PlayerScript>` owns `PlayerScript*` instances.
`HookRegistry<PlayerScript>` holds non-owning pointers to the same instances and dispatches global hooks (e.g., `OnPlayerLogin`) to all subscribers.

### Cached pointer pattern

Resolve script names to pointers **at load time**, not at every dispatch. Store the resolved pointer on the data row:

```cpp
struct CreatureTemplate {
    // ... existing fields ...
    std::string scriptName;                          // DB column
    CreatureScript* resolvedScript = nullptr;        // resolved at MapManager init
};

struct ServerTriggerVolume {
    // ... existing fields ...
    std::string scriptName;
    TriggerScript* resolvedScript = nullptr;
};
```

Boot-time resolution (in `MapManager::Initialize` after registries are populated):

```cpp
for (auto& tmpl : creatureTemplates) {
    if (!tmpl.scriptName.empty()) {
        tmpl.resolvedScript = ScriptRegistry<CreatureScript>::Instance().Get(tmpl.scriptName);
        if (!tmpl.resolvedScript) {
            std::cerr << "[Scripts] creature_template entry=" << tmpl.entry
                      << " references unknown script '" << tmpl.scriptName << "'\n";
        }
    }
}
```

Dispatch becomes a pointer null-check + direct vtable call. No string lookup, no hash, no allocation.

### `ai_name` vs `scripts` — keep them distinct

Schema already has both ([0002_add_creature_npc_gameobject.sql](../MMOGame/Database/migrations/0002_add_creature_npc_gameobject.sql:28,35)):

- `ai_name VARCHAR(64)` — names a **data-driven AI archetype** ("AggressorAI", "PassiveAI", "GuardAI"). Parameters come from other DB columns (aggro radius, leash radius, etc.). Author edits DB, not code.
- `scripts VARCHAR(64)` — names a **hand-coded `CreatureScript`**. Author writes C++.

Resolution priority in `MapInstance::CreateMob`:
1. If `scripts` is non-empty → use `ScriptRegistry<CreatureScript>::Get(scripts)`
2. Else if `ai_name` is non-empty → use `BuiltInAIRegistry::Get(ai_name)`
3. Else → default `CreatureAI` (current behavior)

Keep these as two separate registries. They serve different audiences (designers vs engineers).

### Interface decoupling

Scripts should depend on interfaces, not concrete types — so they can be unit-tested with stubs:

```cpp
class IEntity {
public:
    virtual ~IEntity() = default;
    virtual EntityId GetId() const = 0;
    virtual std::string_view GetName() const = 0;
    virtual Vec2 GetPosition() const = 0;
    virtual float GetHeight() const = 0;
    virtual int32_t GetHealth() const = 0;
    virtual void SetHealth(int32_t) = 0;
    virtual void ApplyAura(const Aura&) = 0;
    // ... only what scripts actually need
};

class IMapContext {
public:
    virtual ~IMapContext() = default;
    virtual std::string_view GetMapName() const = 0;
    virtual float GetTime() const = 0;
    virtual IEntity* GetEntity(EntityId) = 0;
    virtual IEntity* CreateMob(uint32_t templateId, Vec2 pos) = 0;
    virtual void BroadcastEvent(const GameEvent&) = 0;
    virtual InstanceScript* GetInstanceScript() = 0;
    // ...
};
```

`Entity` and `MapInstance` implement these interfaces. Scripts only see the interface in their callback signatures:

```cpp
class CreatureAI {
public:
    virtual void OnEnterCombat(IEntity& target) {}
    virtual void Update(float dt, IMapContext& ctx) {}
    // ...
};
```

This is the **biggest single "if you don't do it day-1 you'll regret it"** item. Retrofitting later means touching every script.

---

## Migration Plan — 7 Steps

Each step is a self-contained chunk that leaves the build green. Steps can be merged independently. Order matters; don't reorder without thinking.

### Step 1 — Foundation (`ScriptObject`, `ScriptRegistry<T>`, `HookRegistry<T>`)

**Goal:** Land the base classes without wiring anything to them. Both new and existing systems coexist.

**New files:**
- `MMOGame/Shared/Source/Scripting/ScriptObject.h` — base class (name + virtual dtor)
- `MMOGame/Shared/Source/Scripting/ScriptRegistry.h` — templated direct-dispatch registry
- `MMOGame/Shared/Source/Scripting/HookRegistry.h` — templated global-hook registry

**Why Shared/:** so the editor can introspect registered script names later (Inspector dropdown for `script_name` field). For now editor doesn't link this code; it's there for the future.

**Modified files:**
- `MMOGame/Shared/CMakeLists.txt` — add the three new headers to `SHARED_HEADERS`

**Sketches:**

```cpp
// ScriptObject.h
namespace MMO {
    class ScriptObject {
    public:
        explicit ScriptObject(std::string name) : m_Name(std::move(name)) {}
        virtual ~ScriptObject() = default;
        std::string_view GetName() const { return m_Name; }
    private:
        std::string m_Name;
    };
}
```

```cpp
// ScriptRegistry.h
namespace MMO {
    template<typename T> requires std::derived_from<T, ScriptObject>
    class ScriptRegistry {
    public:
        static ScriptRegistry& Instance() { static ScriptRegistry r; return r; }

        template<typename U> requires std::derived_from<U, T>
        T* Register() {
            auto script = std::make_unique<U>();
            T* raw = script.get();
            const std::string name(script->GetName());
            m_Scripts[name] = std::move(script);
            return raw;
        }

        T* Get(std::string_view name) const {
            auto it = m_Scripts.find(std::string(name));
            return it == m_Scripts.end() ? nullptr : it->second.get();
        }

        size_t Size() const { return m_Scripts.size(); }

        void ForEach(auto&& fn) const {
            for (auto& [_, s] : m_Scripts) fn(*s);
        }

        void Clear() { m_Scripts.clear(); }

    private:
        std::unordered_map<std::string, std::unique_ptr<T>> m_Scripts;
    };
}
```

```cpp
// HookRegistry.h
namespace MMO {
    template<typename T> requires std::derived_from<T, ScriptObject>
    class HookRegistry {
    public:
        static HookRegistry& Instance() { static HookRegistry r; return r; }
        void Subscribe(T* script) { m_Subscribers.push_back(script); }
        void Clear() { m_Subscribers.clear(); }

        template<auto Method, typename... Args>
        void Dispatch(Args&&... args) {
            for (T* s : m_Subscribers)
                (s->*Method)(std::forward<Args>(args)...);
        }

    private:
        std::vector<T*> m_Subscribers;
    };
}
```

**Acceptance:** project builds clean. No behavioral change.

**Effort:** 30 min.

---

### Step 2 — Migrate TriggerScript onto the new base + cache resolved pointer

**Goal:** Convert the existing trigger-volume system to use `ScriptObject` + `ScriptRegistry<TriggerScript>`. Resolve `script_name` → `TriggerScript*` once at load time; dispatch via cached pointer.

**Modified files:**
- `MMOGame/WorldServer/Source/Triggers/TriggerScript.h` — change `class TriggerScript : public ScriptObject`. Constructor takes name. Drop the local `TriggerScriptRegistry`.
- `MMOGame/WorldServer/Source/Triggers/TriggerScript.cpp` — delete (registry no longer here).
- `MMOGame/WorldServer/Source/Triggers/TriggerScripts.cpp` — change `LogTriggerScript` constructor to `TriggerScript("log")`. Switch to templated `Register<LogTriggerScript>()`.
- `MMOGame/WorldServer/Source/Map/MapDefines.h` — add `TriggerScript* resolvedScript = nullptr;` to `ServerTriggerVolume`.
- `MMOGame/WorldServer/Source/Map/MapManager.cpp` — after templates loaded, resolve `script_name` → pointer for each volume.
- `MMOGame/WorldServer/Source/Map/MapInstance.cpp` — `FireTriggerScript` uses `trigger.resolvedScript` directly, no `Get()` lookup.
- `MMOGame/WorldServer/CMakeLists.txt` — drop `TriggerScript.cpp` from sources.

**Acceptance:**
- Build clean.
- WorldServer logs `[Scripts] Registered 1 TriggerScript: log` at startup (replace existing `[Triggers] Registered scripts: log`).
- For each volume in DB, log either `[Scripts] resolved volume <guid> → log` or `[Scripts] WARNING: volume <guid> references unknown script '<name>'`.
- Existing trigger-volume detection still fires on player spawn / movement.
- Profile a CheckTriggers tick: confirm zero string allocation per dispatch (via debugger or a `_NEW`-counting allocator if you're paranoid).

**Effort:** 1 hour.

**Dependencies:** Step 1.

---

### Step 3 — Split `ScriptedAI` into `CreatureScript` + `CreatureAI`; cache pointer; honor `ai_name` vs `scripts`

**Goal:** This is the biggest single change. ScriptedAI today is owner-bound + factory-bound in one class; split it into:

- **`CreatureScript`** — singleton, registered by name, has `CreateAI(IMapContext&, IEntity&) → unique_ptr<CreatureAI>` factory method. Lives in registry.
- **`CreatureAI`** — per-mob instance, owns runtime state (`m_Owner`, `m_Events`, `m_Summons`, phase, combat). NOT a `ScriptObject`. Owned by `MapInstance::m_MobAIs`.

Honor the schema's `ai_name` vs `scripts` distinction: `scripts` (engineer-coded `CreatureScript`) wins; `ai_name` (data-driven archetype) is fallback; default base `CreatureAI` is last resort.

**New files:**
- `MMOGame/WorldServer/Source/AI/CreatureScript.h` — registered factory class
- `MMOGame/WorldServer/Source/AI/CreatureAI.h` — base AI runtime class (renamed from current `ScriptedAI`)
- `MMOGame/WorldServer/Source/AI/CreatureAI.cpp` — moved bodies
- `MMOGame/WorldServer/Source/AI/BuiltInAI.h` — registry for `ai_name` archetypes (AggressorAI, PassiveAI, GuardAI). Implementations come later; just the registry now.
- `MMOGame/WorldServer/Source/AI/CreatureScripts.cpp` — `RegisterAllCreatureScripts()` body (replaces ScriptLoader.h pattern).

**Modified files:**
- `MMOGame/WorldServer/Source/Scripts/ShadowLordAI.h` — refactor into two pieces:
  - `ShadowLordAI` (extends `CreatureAI`, holds combat state)
  - `ShadowLordScript` (extends `CreatureScript`, has `CreateAI` factory returning `make_unique<ShadowLordAI>(...)`)
- `MMOGame/WorldServer/Source/Map/MapInstance.cpp::CreateMob` — three-step resolution: `scripts` → `ai_name` → default. Cache resolved `CreatureScript*` on `CreatureTemplate` at MapManager init.
- `MMOGame/WorldServer/Source/Map/MapDefines.h` — add `CreatureScript* resolvedScript` to `CreatureTemplate` (currently lives in [`AI/CreatureTemplate.h`](../MMOGame/WorldServer/Source/AI/CreatureTemplate.h) — modify there).
- `MMOGame/WorldServer/Source/AI/ScriptRegistry.h` and `.cpp` — **DELETE** (replaced by `ScriptRegistry<CreatureScript>`).
- `MMOGame/WorldServer/Source/Scripts/ScriptLoader.h` — **DELETE** (replaced by `RegisterAllCreatureScripts`).
- `MMOGame/WorldServer/Source/AI/ScriptedAI.h` and `.cpp` — **DELETE** (split into CreatureScript + CreatureAI).
- `MMOGame/WorldServer/CMakeLists.txt` — update sources.
- `MMOGame/WorldServer/Source/WorldServer.cpp` — call `RegisterAllCreatureScripts()` alongside `RegisterAllTriggerScripts()`.

**Sketches:**

```cpp
// CreatureScript.h
class CreatureScript : public ScriptObject {
public:
    using ScriptObject::ScriptObject;
    virtual std::unique_ptr<CreatureAI> CreateAI(IMapContext& map, IEntity& mob) const = 0;
};
```

```cpp
// ShadowLordScript.h (split from old ShadowLordAI.h)
class ShadowLordAI : public CreatureAI {
public:
    ShadowLordAI(IMapContext& map, IEntity& mob) : CreatureAI(map, mob) {}
    void OnEnterCombat(IEntity& target) override { ... }
    void OnPhaseTransition(uint32_t old, uint32_t newPhase) override { ... }
};

class ShadowLordScript : public CreatureScript {
public:
    ShadowLordScript() : CreatureScript("shadow_lord") {}
    std::unique_ptr<CreatureAI> CreateAI(IMapContext& m, IEntity& e) const override {
        return std::make_unique<ShadowLordAI>(m, e);
    }
};
```

```cpp
// CreatureScripts.cpp
void RegisterAllCreatureScripts() {
    auto& reg = ScriptRegistry<CreatureScript>::Instance();
    reg.Register<ShadowLordScript>();
    // reg.Register<RagnarosScript>();
    // ...

    std::cout << "[Scripts] Registered " << reg.Size() << " CreatureScripts:";
    reg.ForEach([](const auto& s) { std::cout << " " << s.GetName(); });
    std::cout << '\n';
}
```

**MapInstance::CreateMob** logic:

```cpp
Entity* MapInstance::CreateMob(uint32_t templateId, Vec2 position, uint32_t spawnPointId) {
    const CreatureTemplate* tmpl = CreatureTemplates::GetTemplate(templateId);
    if (!tmpl) return nullptr;
    // ... existing entity construction ...

    std::unique_ptr<CreatureAI> ai;
    if (tmpl->resolvedScript) {
        ai = tmpl->resolvedScript->CreateAI(*this, *ptr);
    } else if (!tmpl->ai_name.empty()) {
        if (auto* arch = BuiltInAIRegistry::Instance().Get(tmpl->ai_name)) {
            ai = arch->CreateAI(*this, *ptr);
        }
    }
    if (!ai) {
        ai = std::make_unique<CreatureAI>(*this, *ptr);  // default
    }
    m_MobAIs[id] = std::move(ai);
    // ...
}
```

**Acceptance:**
- Build clean.
- ShadowLord boss still works (manual test: spawn it, it transitions phases, applies invulnerability, drops it when adds die).
- WorldServer startup logs registered creature scripts.
- Add a `CreatureTemplate.scripts = "shadow_lord"` row in DB and verify `[Scripts] resolved entry=N → shadow_lord` log fires.

**Effort:** 3-4 hours. Test thoroughly — touches the most complex existing system.

**Dependencies:** Step 1.

---

### Step 4 — `InstanceScript` with `MapInstance` lifecycle wiring

**Goal:** First-class per-instance encounter state. One `InstanceScript` per `MapInstance` (per dungeon run). Fires forwarded events from inside the instance — boss kills, area triggers, gameobject interactions — and tracks state.

**New files:**
- `MMOGame/WorldServer/Source/AI/InstanceScript.h` — base class. Holds `MapInstance*`. Virtuals: `OnPlayerEnter`, `OnPlayerLeave`, `OnCreatureCreate`, `OnCreatureDeath`, `OnGameObjectStateChange`, `OnAreaTrigger`, `Update(dt)`.
- `MMOGame/WorldServer/Source/AI/InstanceScripts.cpp` — `RegisterAllInstanceScripts()` (empty for now).

**Modified files:**
- `MMOGame/WorldServer/Source/Map/MapDefines.h` — add `std::string instanceScriptName` to `MapTemplate` (and `0002_*.sql` migration column).
- `MMOGame/Database/migrations/0004_add_instance_script.sql` — new migration:
  ```sql
  ALTER TABLE map_template ADD COLUMN instance_script VARCHAR(64) NOT NULL DEFAULT '';
  ```
- `MMOGame/Shared/Source/Database/Database.cpp::LoadAllMapTemplates` — read new column.
- `MMOGame/WorldServer/Source/Map/MapInstance.h` — own a `std::unique_ptr<InstanceScript> m_InstanceScript`.
- `MMOGame/WorldServer/Source/Map/MapInstance.cpp` — construct `InstanceScript` if template has one. Forward events: `RegisterPlayer` → `OnPlayerEnter`, `UnregisterPlayer` → `OnPlayerLeave`, `CreateMob` → `OnCreatureCreate`, etc.
- `MMOGame/WorldServer/Source/WorldServer.cpp` — call `RegisterAllInstanceScripts()`.

**Sketch:**

```cpp
class InstanceScript : public ScriptObject {
public:
    using ScriptObject::ScriptObject;

    // Lifecycle (called by MapInstance)
    virtual void OnInstanceCreate(IMapContext& map) {}
    virtual void OnInstanceDestroy() {}

    // Events
    virtual void OnPlayerEnter(IEntity& player) {}
    virtual void OnPlayerLeave(IEntity& player) {}
    virtual void OnCreatureCreate(IEntity& creature) {}
    virtual void OnCreatureDeath(IEntity& creature, IEntity* killer) {}
    virtual void OnAreaTrigger(IEntity& player, const ServerTriggerVolume& v) {}

    // Tick
    virtual void Update(float dt) {}

    // Persistence (Step 7)
    virtual void Save(WriteBuffer&) const {}
    virtual void Load(ReadBuffer&) {}
};
```

Note: `InstanceScript` is in `ScriptRegistry<InstanceScript>` as a *factory-like* singleton. `MapInstance` does:

```cpp
if (auto* tmpl = ScriptRegistry<InstanceScript>::Instance().Get(m_Template->instanceScriptName)) {
    m_InstanceScript = tmpl->Clone();   // virtual clone factory method
    m_InstanceScript->OnInstanceCreate(*this);
}
```

The `Clone` virtual method is the only quirk — different from CreatureScript's `CreateAI` because InstanceScript inherits from itself (clone returns same type). Alternative: make `InstanceScript` a factory and define `InstanceScriptInstance` as the runtime class (matches the CreatureScript/CreatureAI split). Pick one and stay consistent.

**Recommendation:** mirror the CreatureScript pattern — `InstanceScript` is the registered factory, `InstanceState` is the per-MapInstance runtime. Avoids the awkward `Clone()` method.

**Acceptance:**
- Build clean.
- Migration applies cleanly.
- Verify `MapInstance` constructs an `InstanceState` when `map_template.instance_script` is set.
- Smoke-test by registering a stub `LogInstanceScript` that prints `[Instance] player entered <map>` and confirm it fires.

**Effort:** 2-3 hours.

**Dependencies:** Steps 1, 3 (uses `IMapContext`/`IEntity` interfaces).

---

### Step 5 — Stub `GameObjectScript`, `QuestScript`, `SpellScript`, `PlayerScript`

**Goal:** Add the empty registries and base classes for script types that don't have hooks yet. This prevents drift later — when GameObjects need scripting, the type already exists; you just add callbacks.

**New files:**
- `MMOGame/WorldServer/Source/Scripting/GameObjectScript.h`
- `MMOGame/WorldServer/Source/Scripting/QuestScript.h`
- `MMOGame/WorldServer/Source/Scripting/SpellScript.h`
- `MMOGame/WorldServer/Source/Scripting/PlayerScript.h`
- One `RegisterAll<X>()` stub per type, in matching `.cpp` files.

**Modified files:**
- `MMOGame/WorldServer/Source/WorldServer.cpp` — call all the new `RegisterAll*()` stubs.

**Sketch:**

```cpp
// PlayerScript.h
class PlayerScript : public ScriptObject {
public:
    using ScriptObject::ScriptObject;
    virtual void OnLogin(IEntity& player) {}
    virtual void OnLogout(IEntity& player) {}
    virtual void OnLevelUp(IEntity& player, uint32_t oldLevel, uint32_t newLevel) {}
    virtual void OnDeath(IEntity& player, IEntity* killer) {}
    virtual void OnQuestComplete(IEntity& player, uint32_t questId) {}
};
```

`PlayerScript` is the type most likely to use the **HookRegistry pattern** — many scripts subscribe to `OnLogin`, all of them fire. Add this in the same step:

```cpp
// In RegisterAllPlayerScripts:
// 1. Register concrete scripts in ScriptRegistry<PlayerScript> (owns lifetime).
// 2. Subscribe each to HookRegistry<PlayerScript> (broadcasts events).
auto* welcomeScript = reg.Register<WelcomeScript>();
HookRegistry<PlayerScript>::Instance().Subscribe(welcomeScript);
```

Then dispatching from WorldServer when a player logs in:

```cpp
HookRegistry<PlayerScript>::Instance().Dispatch<&PlayerScript::OnLogin>(*entity);
```

**Acceptance:**
- Build clean.
- WorldServer startup logs `[Scripts] Registered 0 GameObjectScripts / 0 QuestScripts / 0 SpellScripts / 0 PlayerScripts`.
- No behavioral change.

**Effort:** 1-2 hours total (skeleton only — no actual scripts implemented).

**Dependencies:** Steps 1, 6 (uses interfaces).

---

### Step 6 — `IMapContext` / `IEntity` interfaces; switch `CreatureAI` to use them

**Goal:** Decouple scripts from the concrete `MapInstance` and `Entity` types so individual scripts can be unit-tested with stubs.

**New files:**
- `MMOGame/WorldServer/Source/Scripting/IEntity.h` — minimal interface for what scripts read/mutate on an entity.
- `MMOGame/WorldServer/Source/Scripting/IMapContext.h` — minimal interface for map-level operations scripts perform.

**Modified files:**
- `MMOGame/WorldServer/Source/Entity/Entity.h/.cpp` — `Entity` inherits from `IEntity`. Implement the virtuals (most are one-line forwards).
- `MMOGame/WorldServer/Source/Map/MapInstance.h/.cpp` — `MapInstance` inherits from `IMapContext`. Implement the virtuals.
- `MMOGame/WorldServer/Source/AI/CreatureAI.h/.cpp` — change all callbacks: `Entity*` → `IEntity&`, `MapInstance*` → `IMapContext&`. Update existing scripts (ShadowLord etc).
- `MMOGame/WorldServer/Source/Triggers/TriggerScript.h` — same change to `OnEnter/OnExit/OnStay`.
- All `InstanceScript`, `GameObjectScript`, `QuestScript`, `SpellScript`, `PlayerScript` — same change.

**What the interfaces should and shouldn't expose:**

```cpp
// IEntity — what scripts read/mutate. Minimal, NOT a full Entity API.
class IEntity {
public:
    virtual ~IEntity() = default;

    // Identity
    virtual EntityId GetId() const = 0;
    virtual std::string_view GetName() const = 0;
    virtual EntityType GetType() const = 0;

    // Position
    virtual Vec2 GetPosition() const = 0;
    virtual float GetHeight() const = 0;
    virtual void SetPosition(Vec2) = 0;

    // Combat
    virtual int32_t GetHealth() const = 0;
    virtual int32_t GetMaxHealth() const = 0;
    virtual void ModifyHealth(int32_t delta, IEntity* source) = 0;
    virtual bool IsDead() const = 0;

    // Auras
    virtual uint32_t AddAura(const Aura&) = 0;
    virtual void RemoveAura(uint32_t auraId) = 0;
    virtual bool HasAuraType(AuraType) const = 0;

    // ... only what scripts ACTUALLY call. Don't pre-add stuff.
};
```

**Test scaffolding** (at least one):

```cpp
// MMOGame/WorldServer/Tests/MockEntity.h
class MockEntity : public IEntity {
public:
    EntityId id = 1;
    std::string name = "mock";
    int32_t health = 100, maxHealth = 100;
    Vec2 position{0, 0};

    EntityId GetId() const override { return id; }
    std::string_view GetName() const override { return name; }
    int32_t GetHealth() const override { return health; }
    // ... rest as one-liners
};

// MMOGame/WorldServer/Tests/ShadowLordAI_test.cpp
TEST(ShadowLordAI, EntersPhase2WhenHealthBelow50) {
    MockMapContext map;
    MockEntity boss; boss.health = 100; boss.maxHealth = 100;
    ShadowLordAI ai(map, boss);

    boss.health = 40;  // below 50% threshold
    ai.Update(0.1f, ...);

    ASSERT_TRUE(ai.HasAuraType(AuraType::DAMAGE_IMMUNITY));
    ASSERT_EQ(ai.GetCurrentPhase(), 2);
}
```

Add a small test framework if you don't have one (Google Test via CMake FetchContent is the easy choice). One test file per script, with two or three tests each.

**Acceptance:**
- Build clean.
- All existing scripts (ShadowLord, log trigger) compile against new interface signatures.
- At least one unit test for one script runs and passes.
- Existing manual test (run editor + spawn boss) still works end-to-end.

**Effort:** 3-4 hours.

**Dependencies:** Steps 1, 3.

**This is the highest-value step long-term.** Do it well.

---

### Step 7 — Persistence interface + one concrete usage

**Goal:** Define how scripts that need state across WorldServer restarts handle save/load. Implement for one type (likely `InstanceState`) to prove the pattern.

**New files:**
- None — extends existing types.

**Modified files:**
- `MMOGame/WorldServer/Source/Scripting/ScriptObject.h` — add virtual `Save(WriteBuffer&)` / `Load(ReadBuffer&)` (default no-ops).
- `MMOGame/Database/migrations/0005_add_instance_state.sql`:
  ```sql
  CREATE TABLE instance_state (
      instance_id  INTEGER PRIMARY KEY,
      map_id       INTEGER NOT NULL,
      script_name  VARCHAR(64) NOT NULL,
      state_blob   BYTEA NOT NULL,
      saved_at     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
  );
  ```
- `MMOGame/Shared/Source/Database/Database.cpp` — `LoadInstanceState(uint32_t instanceId)` + `SaveInstanceState(...)`.
- `MMOGame/WorldServer/Source/Map/MapInstance.cpp` — on construction with non-empty `instance_script`, `LoadInstanceState`. On destruction, `SaveInstanceState`.

**Pattern:**

Scripts opt in to persistence by overriding `Save`/`Load`:

```cpp
class MyInstanceState : public InstanceState {
public:
    void Save(WriteBuffer& buf) const override {
        buf.WriteU32(m_BossesKilled);
        buf.WriteU32(m_PlayersDead);
        buf.WriteFloat(m_LongestEncounterTime);
    }

    void Load(ReadBuffer& buf) override {
        m_BossesKilled = buf.ReadU32();
        m_PlayersDead = buf.ReadU32();
        m_LongestEncounterTime = buf.ReadFloat();
    }

private:
    uint32_t m_BossesKilled = 0;
    uint32_t m_PlayersDead = 0;
    float m_LongestEncounterTime = 0.0f;
};
```

**Acceptance:**
- Build clean.
- Migration applies cleanly.
- Manual test: spawn an instance, modify state via a stub script, restart WorldServer, verify state restored.

**Effort:** 2-3 hours.

**Dependencies:** Step 4 (InstanceScript exists).

---

## Summary table

| Step | Goal | Effort | Depends on |
|---|---|---|---|
| 1 | ScriptObject + ScriptRegistry<T> + HookRegistry<T> | 30m | — |
| 2 | Migrate TriggerScript onto new base + cached pointer | 1h | 1 |
| 3 | Split CreatureScript / CreatureAI; ai_name vs scripts | 3-4h | 1 |
| 4 | InstanceScript with MapInstance lifecycle | 2-3h | 1, 3 |
| 5 | Stub GameObjectScript / QuestScript / SpellScript / PlayerScript | 1-2h | 1, 6 |
| 6 | IMapContext + IEntity interfaces | 3-4h | 1, 3 |
| 7 | Persistence interface + one concrete impl | 2-3h | 4 |
| | **Total** | **~1.5 days** | |

Suggested merge order: 1 → 2 → 3 → 6 → 4 → 5 → 7. Step 6 before 5 because the stubs in 5 need the interfaces. Step 4 could swap with 6 if you prefer — minor.

---

## What NOT to do

- **Don't add a CRTP auto-registration template.** Looks clever, breaks in surprising ways (linker stripping in static libs, static-init order across TUs), and the explicit per-type loader is clearer.
- **Don't add a script lookup language (Lua, Python).** Not needed yet. C++ scripts compile fast enough on this codebase. Type safety is gold for an MMO. Defer until content authors prove they need it.
- **Don't add an event bus.** Keep callbacks direct. Event buses are great for decoupling, terrible for tracing in debugger when something fires that you didn't expect.
- **Don't unify `CreatureAI` with `TriggerScript` under one "callback object" base.** Different lifecycles, different APIs — forcing them into one hierarchy creates a god-class. The shared base is `ScriptObject`, not anything richer.
- **Don't try to support multiple scripts per data row.** AzerothCore allows comma-separated `ScriptName` fields. It's messy, rarely actually needed, and obscures dispatch order. Stick with one script per row; use C++ inheritance for "extend behavior".
- **Don't pre-implement hot reload.** Mark seams (`Clear()`/`Reload()` on registries) but don't pretend it's free. Compiled C++ hot reload is a Tier 3 problem with cross-cutting concerns (vtable layout, in-flight virtual calls).
- **Don't over-design `IEntity` / `IMapContext`.** Add only methods scripts ACTUALLY call. Resist the urge to expose every getter. Adding methods later is cheap; removing them isn't.

---

## Glossary

Easy-to-confuse terms — fix terminology before refactoring.

| Term | Meaning |
|---|---|
| **ScriptObject** | Common base class. Just identity (name) + virtual destructor. |
| **ScriptRegistry<T>** | Templated singleton owning `unique_ptr<T>` map keyed by name. One per script type. |
| **HookRegistry<T>** | Templated singleton holding non-owning pointers to subscribed `T`s. Dispatch broadcasts to all subscribers. Different from ScriptRegistry. |
| **CreatureScript** | Singleton, registered, factory-like. Owns name; produces per-mob `CreatureAI` via `CreateAI(...)`. |
| **CreatureAI** | Per-mob runtime object. NOT a `ScriptObject`. Owns combat state, references owner via `IEntity&`. |
| **TriggerScript** | Singleton, callback-only. One instance handles all volumes that name it. |
| **GameObjectScript / QuestScript / SpellScript / PlayerScript** | Singleton callback types, like TriggerScript. |
| **InstanceScript** | Factory for per-MapInstance state. |
| **InstanceState** | Per-MapInstance runtime object created by `InstanceScript`. Owns encounter state. |
| **BuiltInAI** | Data-driven AI archetype (`AggressorAI`, `PassiveAI`, …) named by `creature_template.ai_name`. Different registry from `CreatureScript`. |
| **`scripts` field** | DB column on `creature_template` / `gameobject_template` / `trigger_volume` naming a hand-coded `*Script`. |
| **`ai_name` field** | DB column on `creature_template` naming a `BuiltInAI` archetype. |
| **resolved pointer** | The `*Script*` cached on a data row at boot, replacing per-dispatch name lookup. |
| **direct dispatch** | One script attached to one data row (CreatureScript, TriggerScript). |
| **global hook** | Many scripts subscribe to one event (PlayerScript via HookRegistry). |

---

## AzerothCore mapping

For anyone with AzerothCore background reading this:

| Onyx (proposed) | AzerothCore equivalent |
|---|---|
| `ScriptObject` (base) | `ScriptObject` |
| `ScriptRegistry<T>` | `ScriptRegistry<T>` (templated, per script type) |
| `HookRegistry<T>` | `ScriptMgr` global hooks (`OnPlayerLogin`, etc.) |
| `CreatureScript` + `CreatureAI` | `CreatureScript::GetAI()` + `CreatureAI` |
| `TriggerScript` | `AreaTriggerScript` (Onyx adds OnExit/OnStay) |
| `InstanceScript` + `InstanceState` | `InstanceMapScript::GetInstanceScript()` + `InstanceScript` |
| `BuiltInAI` registry | Hardcoded `SelectAI()` switch in core |
| `IMapContext` / `IEntity` | (Onyx-specific — AzerothCore has no equivalent) |
| `RegisterAllX()` central loaders | `AddSC_*()` functions |
| `ai_name` vs `scripts` | `AIName` vs `ScriptName` columns on `creature_template` |

Onyx's deviations from AzerothCore:
1. **C++20 idioms** (concepts, references, `std::derived_from`).
2. **Interface decoupling** for testability (`IEntity`/`IMapContext`) — AzerothCore exposes concrete classes directly; very few of their scripts have unit tests.
3. **Cached pointer pattern** — AzerothCore looks up scripts by name at every dispatch. Onyx caches at load time.
4. **No multi-script-per-row support** — AzerothCore allows comma-separated `ScriptName`. Onyx doesn't (deliberate simplification).

---

## Future considerations (not in scope)

These are *post-refactor* concerns. Don't try to design for them now; just don't paint into a corner.

- **Hot reload** — DLL-loaded scripts that can be reloaded without restarting WorldServer. Architecture supports it via `Clear()` on registries; implementation is hard (vtable layout, in-flight calls, per-entity AI invalidation).
- **Lua / Python embedding** — for content authors who can't iterate at C++ build speeds. Add a Lua-based `LuaCreatureScript : CreatureScript` adapter when needed. The script-object pattern accommodates it.
- **Modding API / sandbox** — only matters when third parties write scripts. Defer until that's a real requirement.
- **Profiling per-script overhead** — at scale (10k mobs), the virtual dispatch cost matters. Solve with batching / spatial indexing before reaching for "remove virtuals".
- **Scripted events crossing instance boundaries** — global world-events that affect all running map instances. Solve with `HookRegistry<WorldScript>` when needed.

---

## Pointers to current code

For the next session, here's the existing surface area that needs to change:

### Read first (understand current state)
- [`MMOGame/WorldServer/Source/AI/ScriptedAI.h`](../MMOGame/WorldServer/Source/AI/ScriptedAI.h) + `.cpp`
- [`MMOGame/WorldServer/Source/AI/ScriptRegistry.h`](../MMOGame/WorldServer/Source/AI/ScriptRegistry.h) + `.cpp`
- [`MMOGame/WorldServer/Source/Scripts/ScriptLoader.h`](../MMOGame/WorldServer/Source/Scripts/ScriptLoader.h)
- [`MMOGame/WorldServer/Source/Scripts/ShadowLordAI.h`](../MMOGame/WorldServer/Source/Scripts/ShadowLordAI.h)
- [`MMOGame/WorldServer/Source/Triggers/TriggerScript.h`](../MMOGame/WorldServer/Source/Triggers/TriggerScript.h) + `.cpp`
- [`MMOGame/WorldServer/Source/Triggers/TriggerScripts.cpp`](../MMOGame/WorldServer/Source/Triggers/TriggerScripts.cpp)
- [`MMOGame/WorldServer/Source/Map/MapInstance.cpp`](../MMOGame/WorldServer/Source/Map/MapInstance.cpp) — search for `m_MobAIs`, `CheckTriggers`, `FireTriggerScript`, `CreateMob`
- [`MMOGame/WorldServer/Source/AI/CreatureTemplate.h`](../MMOGame/WorldServer/Source/AI/CreatureTemplate.h) — sees current `scriptName` field

### Database schema
- [`MMOGame/Database/migrations/0002_add_creature_npc_gameobject.sql`](../MMOGame/Database/migrations/0002_add_creature_npc_gameobject.sql) — already has `ai_name` and `scripts` columns on `creature_template`, plus full `trigger_volume` schema.
- New migrations 0004 (instance_script column) and 0005 (instance_state table) added by Steps 4 and 7.

### Build system
- [`MMOGame/WorldServer/CMakeLists.txt`](../MMOGame/WorldServer/CMakeLists.txt) — add new sources under `WORLDSERVER_SOURCES`, group under `source_group("Scripting")`.
- [`MMOGame/Shared/CMakeLists.txt`](../MMOGame/Shared/CMakeLists.txt) — add `Scripting/` headers to `SHARED_HEADERS`.

---

## Acceptance for the whole refactor

When all 7 steps are done, the system should satisfy:

1. **Single base class** for all script types (`ScriptObject`).
2. **Two dispatch mechanisms** — direct (script-on-data-row) and global (hook bus) — both available.
3. **Cached pointer dispatch** — no string lookup in any per-tick path.
4. **`ai_name` vs `scripts`** distinction implemented and tested.
5. **InstanceScript / InstanceState** lifecycle wired into MapInstance.
6. **At least one unit test** for a script using the interface decoupling.
7. **Persistence pattern** demonstrated end-to-end (instance state survives WorldServer restart).
8. **Stub registries** for GameObjectScript / QuestScript / SpellScript / PlayerScript exist and log themselves at boot.
9. **Boot-time discovery log** for every registry (e.g., `[Scripts] Registered N TriggerScripts: log, teleport_to_dark_forest`).
10. **Existing functionality preserved** — ShadowLord boss + log trigger still work end-to-end via the new architecture.

When all 10 are green, the foundation is ready for the next year of feature work without another refactor.
