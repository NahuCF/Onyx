# Trigger Volumes

Static spatial volumes authored in the editor that fire scripted callbacks when an entity enters, stays inside, or leaves them. Modeled after AzerothCore's `AreaTrigger` (table + name-keyed C++ scripts) but server-driven (no client trust).

## TL;DR

A trigger volume is **geometry + a script-name reference**. The volume itself does nothing; the named C++ script attached to it does. Authoring a volume in the editor is "place + set Script Name + save". Behavior beyond the four data-side knobs (shape, size, `triggerOnce`, players/creatures filter) requires writing a script.

## End-to-end flow

```
Editor: TriggerVolume (Inspector: shape, size, ScriptName, …)
   │
   │ Ctrl+S → Editor3DLayer::SyncWorldToDatabase
   ▼
DB: trigger_volume row (per-map, guid keyed)
   │
   │ WorldServer boot → Database::LoadTriggerVolumes(mapId)
   ▼
MapTemplate.triggerVolumes (immutable for the instance lifetime)
   │
   │ MapInstance ctor → BuildTriggerCellIndex
   ▼
Per-cell bucket: cell → indices into triggerVolumes[]
   │
   │ Every entity move > 0.0001 units (and on player spawn)
   ▼
CheckTriggers(entityId, oldPos, newPos)
   │ ▸ pull only volumes overlapping the entity's current cell
   │ ▸ filter by triggerPlayers / triggerCreatures
   │ ▸ skip volumes already fired if triggerOnce
   │ ▸ point-in-volume test (sphere or oriented box)
   │ ▸ diff "now inside" vs "previously inside" set per-entity
   ▼
TriggerScriptRegistry.Get(scriptName) → script->OnEnter / OnExit / OnStay
```

## Detection model

| Aspect | Choice | Why |
|---|---|---|
| Authority | Server-side | Custom client has no DBC equivalent; no trust required. |
| Trigger | Movement-driven | Idle players cost zero; check only fires when position changes. |
| Spatial filter | Grid-cell bucketing | At 32-unit cells + typical 5-50-unit volumes, 1–9 candidates per check. |
| Edge detection | Per-entity "currently inside" set | OnEnter/OnExit fire exactly once per crossing — no per-tick spam. |
| Spawn case | Synthetic check at `CreatePlayer` | Players who spawn *inside* a volume get OnEnter immediately, not on first step. |

## Authored knobs (no code required)

Set these in the Inspector; they cover most cases without writing C++.

| Field | Effect |
|---|---|
| **Shape** | `BOX` (oriented half-extents + yaw), `SPHERE` (radius), `CAPSULE` (radius + half-height). |
| **Half Extents** / **Radius** | Volume size. Box uses XYZ half-extents; sphere/capsule uses radius (capsule also uses Z half-extent for height). |
| **Position** / **Rotation** | Standard transform gizmo. Editor Y-up is converted to server Z-up at save time. |
| **Trigger Event** | `ON_ENTER` (default), `ON_EXIT`, `ON_STAY`. Enter and exit always fire on edges; selecting `ON_STAY` *additionally* calls `OnStay` every move while inside. |
| **Trigger Once** | After the first OnEnter fires in this `MapInstance`'s lifetime, the volume becomes inert. Useful for one-shot reveals / quest credits. |
| **Trigger Players** / **Trigger Creatures** | Filter — only fires for entities of those types. |
| **Script Name** | The string key looked up in `TriggerScriptRegistry`. Empty → no callback (volume is silently inert). |
| **Event Id** | Free integer the script can read off `trigger.eventId` to disambiguate when one script handles many volumes. |

## What scripts can do

A `TriggerScript` callback receives `MapInstance&`, `Entity& entity`, `const ServerTriggerVolume&`. From there, you have the full server-side API. Practical recipes:

| Use | What the script does |
|---|---|
| **Quest credit** | `entity.GetQuestLog()->Complete(questId)` (when quest log exists) |
| **Teleport / portal** | `MapManager::Instance().TransferPlayer(entity.GetId(), map.GetInstanceId(), destMap, destPos, …)` |
| **Apply zone aura** | `entity.GetAuras()->Add(AuraType::REST, duration)` (rested XP, sanctuary, etc.) |
| **Open a door / activate gameobject** | resolve a nearby `gameobject_spawn` and toggle its state |
| **Boss intro / ambush** | `map.CreateMob(creatureTemplateId, position, /*spawnPointId=*/0)` at preset positions |
| **Damage zone** | `OnStay` → `combat.ApplyDamage(entity, value, DamageType::FIRE)` every tick |
| **Analytics / debug** | what `log` does — print, write CSV, hit a Prometheus counter |
| **PvP zone toggle** | `entity.SetByteFlag(UNIT_BYTE2_FLAG_FFA_PVP)` on enter, clear on exit |
| **Cinematic checkpoint** | record progress via `InstanceScript`, gate boss doors based on it |
| **Ambient music change** | broadcast a custom `GameEvent` to all nearby players via `map.BroadcastEvent(...)` |

The only knobs that *don't* require code are the data-side ones above. Behavior beyond that = write a script.

## Adding a new script

Three steps. All in [MMOGame/WorldServer/Source/Triggers/TriggerScripts.cpp](../MMOGame/WorldServer/Source/Triggers/TriggerScripts.cpp).

```cpp
// 1) Declare a class derived from TriggerScript.
class TeleportTriggerScript : public TriggerScript
{
public:
    void OnEnter(MapInstance& map, Entity& e, const ServerTriggerVolume& v) override
    {
        // Use v.eventId to look up the destination, or hardcode it for a
        // single-purpose volume.
        MapManager::Instance().TransferPlayer(
            e.GetId(), map.GetInstanceId(),
            /*destMapId=*/2, /*destPos=*/Vec2(0, 0),
            /*peerId=*/0, /*charId=*/0, /*accountId=*/0);
    }
};
```

```cpp
// 2) Register in RegisterAllTriggerScripts()
void RegisterAllTriggerScripts()
{
    auto& reg = TriggerScriptRegistry::Instance();
    reg.Register("log", std::make_unique<LogTriggerScript>());
    reg.Register("teleport_to_dark_forest", std::make_unique<TeleportTriggerScript>());
}
```

```
3) In the editor, set the volume's Script Name field to "teleport_to_dark_forest"
   and Ctrl+S. Restart Run Locally and walk into the volume.
```

Unknown script names are diagnosed at edge time — WorldServer prints `[Trigger] Unknown script '<name>' on volume '<guid>'` and the volume goes silent.

## Schema

`trigger_volume` table — defined in [`MMOGame/Database/migrations/0002_add_creature_npc_gameobject.sql`](../MMOGame/Database/migrations/0002_add_creature_npc_gameobject.sql).

```
guid              TEXT PRIMARY KEY    -- editor-stable identity
map_id            INTEGER             -- which map this belongs to
shape             SMALLINT            -- 0=BOX, 1=SPHERE, 2=CAPSULE
position_x/y/z    REAL                -- center (Z-up server coords)
orientation       REAL                -- yaw (radians) — BOX only
half_extent_x/y/z REAL                -- BOX half-extents (per-axis)
radius            REAL                -- SPHERE/CAPSULE radius
trigger_event     SMALLINT            -- 0=ON_ENTER, 1=ON_EXIT, 2=ON_STAY
trigger_once      BOOLEAN
trigger_players   BOOLEAN
trigger_creatures BOOLEAN
script_name       VARCHAR(64)         -- key into TriggerScriptRegistry
event_id          INTEGER             -- script-defined (0 by default)
```

DB stores Z-up; the editor authors in Y-up. Axis swap happens once on save (in `MigrationSqlWriter::EmitTriggerVolumesForMap`) and once on load (in `Editor3DLayer::LoadWorldFromDatabase` and `MapManager::Initialize`).

## Pointers

- Runtime detection: [`MapInstance::CheckTriggers`](../MMOGame/WorldServer/Source/Map/MapInstance.cpp), [`MapInstance::BuildTriggerCellIndex`](../MMOGame/WorldServer/Source/Map/MapInstance.cpp), [`MapInstance::FireTriggerScript`](../MMOGame/WorldServer/Source/Map/MapInstance.cpp)
- Geometry: [`ServerTriggerVolume::Contains`](../MMOGame/WorldServer/Source/Map/MapDefines.cpp)
- Script base + registry: [`TriggerScript`](../MMOGame/WorldServer/Source/Triggers/TriggerScript.h)
- Editor authoring: [`TriggerVolume`](../MMOGame/Shared/Source/World/TriggerVolume.h) + Inspector panel
- Save/load: [`MigrationSqlWriter::EmitTriggerVolumesForMap`](../MMOGame/Editor3D/Source/Export/MigrationSqlWriter.cpp), [`Database::LoadTriggerVolumes`](../MMOGame/Shared/Source/Database/Database.cpp)
