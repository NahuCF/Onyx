# MMOGame Shared Library

Common code linked by LoginServer, WorldServer, MMOClient, and MMOEditor3D. Builds as `libMMOShared.a`.

For terrain and file formats see [terrain-and-formats.md](terrain-and-formats.md).

## Layout (`MMOGame/Shared/Source/`)

| Folder | Purpose |
|---|---|
| `Data/` | `GameDataStore.h/.cpp` — singleton race/class/create-info cache |
| `Database/` | `Database.h/.cpp` — pqxx wrapper |
| `Items/` | `Items.h/.cpp` — `ItemInstance`, `InventorySlot`, item templates |
| `Map/` | `MapRegistry.h/.cpp` — `maps.json` registry of maps |
| `Model/` | `OmdlFormat.h`, `OmdlReader.h/.cpp`, `OmdlWriter.h/.cpp` — `.omdl` model format |
| `Network/` | `Buffer.h/.cpp` (read/write helpers), `ENetWrapper.h/.cpp` (`NetworkClient`, `NetworkServer`) |
| `Packets/` | `Packets.h` — every packet type and payload |
| `Scripting/` | `ScriptObject.h`, `ScriptRegistry<T>.h`, `HookRegistry<T>.h` — base types for all script systems |
| `Spells/` | `SpellDefines.h`, `AbilityData.h/.cpp` — `AuraType`, `SpellEffect`, `AbilityData` |
| `Terrain/` | `TerrainData.h`, `ChunkFormat.h`, `ChunkIO.h`, `ChunkFileReader.h/.cpp`, `ChunkFileWriter.h/.cpp`, `TerrainMeshGenerator.h/.cpp` — see [terrain-and-formats.md](terrain-and-formats.md) |
| `Types/` | `Types.h/.cpp` — `DamageType`, `EquipmentSlot`, `CharacterClass`, primitive type aliases |
| `World/` | Header-only — `Transform`, `WorldObject`, `WorldObjectData`, `StaticObject`, `Light`, `InstancePortal`, `TriggerVolume`, `ParticleEmitter`, `GroupObject`, `SpawnPoint`, `PlayerSpawn`, `WorldTypes` |

## Network

`Network/ENetWrapper.h`:
- `NetworkClient` — client-side ENet wrapper used by MMOClient.
- `NetworkServer` — server-side ENet wrapper used by LoginServer + WorldServer.
- Both expose `PollEvent()` returning `CONNECTED` / `DISCONNECTED` / `DATA_RECEIVED`.

`Network/Buffer.h`:
- `WriteBuffer` — `WriteU8/16/32/64`, `WriteI8/16/32/64`, `WriteF32/64`, `WriteBool`, `WriteString`, `WriteVec2`, `WriteVec3`, `WriteBytes`.
- `ReadBuffer` — matching `ReadX` operations plus `HasData(bytes)`, `RemainingBytes()`, `Position()`, `Reset()`.

## Packets (`Packets.h`)

Three top-level enums identify packet kinds:

- **`LoginPacketType`** — `C_REGISTER_REQUEST`, `C_LOGIN_REQUEST`, `C_CREATE_CHARACTER`, `C_DELETE_CHARACTER`, `C_SELECT_CHARACTER`, `S_REGISTER_RESPONSE`, `S_LOGIN_RESPONSE`, `S_CHARACTER_LIST`, `S_CHARACTER_CREATED`, `S_ERROR`, …
- **`WorldPacketType`** — `C_AUTH_TOKEN`, `C_INPUT`, `C_CAST_ABILITY`, `C_SELECT_TARGET`, `C_USE_PORTAL`, `S_AUTH_RESULT`, `S_ENTER_WORLD`, `S_WORLD_STATE`, `S_ENTITY_SPAWN`, `S_ENTITY_UPDATE`, `S_PLAYER_POSITION`, `S_AURA_UPDATE`, `S_AURA_UPDATE_ALL`, `S_INVENTORY_DATA`, `S_EQUIPMENT_DATA`, `S_LOOT_RESPONSE`, …
- **`GameEventType`** — `DAMAGE`, `HEAL`, `DEATH`, `RESPAWN`, `CAST_START`, `CAST_CANCEL`, `CAST_END`, `ABILITY_EFFECT`, `BUFF_APPLIED`, `BUFF_REMOVED`, `LEVEL_UP`, `PROJECTILE_SPAWN`, `PROJECTILE_HIT`, `XP_GAIN`.

`AuraUpdateType`: `ADD = 0`, `REMOVE = 1`, `REFRESH = 2`, `STACK = 3`.
`ErrorCode`: `NONE`, `INVALID_CREDENTIALS`, `ACCOUNT_EXISTS`, `NAME_TAKEN`, `SERVER_FULL`, `AUTH_FAILED`, …

### Key payload structs

- `EntityState` — `id`, `type`, `position`, `rotation`, `moveState`, `health/maxHealth`, `mana/maxMana`, `targetId`, `isCasting`, `castingAbilityId`, `castProgress`.
- `S_WorldState` — `serverTick`, `yourLastInputSeq`, `yourPosition`, `entities[]`.
- `S_EntityUpdate` — `id`, `updateMask` (bits: `UPDATE_POSITION`, `UPDATE_HEALTH`, `UPDATE_MANA`, `UPDATE_TARGET`, `UPDATE_CASTING`, `UPDATE_MOVE_STATE`).
- `S_PlayerPosition` — `serverTick`, `lastInputSeq`, `position` (used for input prediction reconciliation).
- `AuraInfo` — `auraId`, `sourceAbility`, `auraType`, `value`, `duration`, `maxDuration`, `casterId`.
- `S_AuraUpdate` — single aura change (target, type, aura).
- `S_AuraUpdateAll` — full aura sync on zone enter.
- `S_InventoryData { array<ItemNetData, INVENTORY_SIZE> slots }`.
- `S_EquipmentData { array<ItemNetData, EQUIPMENT_SLOT_COUNT> slots }`.

## Spells (`Spells/SpellDefines.h`)

`SpellEffect` — single effect within an ability:
- `EffectType`: `DIRECT_DAMAGE`, `DIRECT_HEAL`, `APPLY_AURA`, `SUMMON_CREATURE`, `LAUNCH_PROJECTILE`, …
- Payload depends on type (damage value, aura type, summon entry, projectile speed, etc.).

`AbilityData` (`AbilityData.h/.cpp`) — full ability descriptor:
- `castTime`, `cooldown`, `range`, `manaCost`.
- `vector<SpellEffect> effects`.

`AuraType` enum (32+ values; full list in [mmogame-server.md](mmogame-server.md)).

## Items

`Items/Items.h`:

```cpp
struct ItemInstance {
    ItemInstanceId instanceId;
    ItemId templateId;
    uint32_t stackCount = 1;
    bool IsValid() const;
    void Clear();
};

struct InventorySlot {
    ItemInstance item;
    bool IsEmpty() const;
    void Clear();
};
```

Item templates (stats, slot, name, …) are loaded from the database / templates file. Item logic (use, equip side effects) lives server-side under `WorldServer/Source/Items/`.

## Data store

`Data/GameDataStore.h` — singleton (`Instance()`) caching race/class/create-info templates from the DB:

- `GetRaceTemplate(race)` / `GetClassTemplate(cls)` / `GetPlayerCreateInfo(race, cls)`
- `GetAllRaces()` / `GetAllClasses()`
- `GetMapName(mapId)`
- `LoadFromDatabase()` — populates the cache (gated by `HAS_DATABASE`).

Used by:
- LoginServer for character creation validation.
- WorldServer for spawn position lookup.

## Map registry

`Map/MapRegistry.h`:

```cpp
enum class MapInstanceType { OpenWorld=0, Dungeon=1, Raid=2, Battleground=3, Arena=4 };

struct MapEntry {
    uint32_t id;
    std::string internalName;
    std::string displayName;
    MapInstanceType instanceType;
    uint32_t maxPlayers;
};
```

Singleton registry with JSON load/save from `maps.json`. Used by Editor3D (`EditorMapRegistry` wrapper) and the game servers.

## World object structs (`World/`)

Header-only structs used by Editor3D and serialization:

- `Transform` — position, rotation (quat), scale
- `WorldObjectData` — base type (POD form of editor objects)
- `StaticObject` — model + transform + material
- `Light` — type + position/direction/color/range/angles
- `InstancePortal`, `TriggerVolume`, `ParticleEmitter`, `GroupObject`
- `SpawnPoint`, `PlayerSpawn` — gameplay-relevant placement

`WorldToChunkX/Z` helpers live in `WorldObjectData.h`.

## Database wrapper

`Database/Database.h` — pqxx wrapper. Used directly by LoginServer for accounts/sessions/characters; WorldServer uses it for character snapshot save/load and `GameDataStore` population.

## Types (`Types/Types.h`)

Common enums used across packets and components:

```cpp
enum class DamageType : uint8_t {
    PHYSICAL = 0, FIRE = 1, FROST = 2, HOLY = 3
};

enum class EquipmentSlot : uint8_t {
    HEAD, NECK, SHOULDERS, BACK, CHEST, WRISTS, HANDS,
    WAIST, LEGS, FEET, RING1, RING2, WEAPON, OFFHAND,
    COUNT = 14, NONE = 255
};
```

Plus `CharacterClass`, `MoveState`, `EntityType`, ID type aliases (`EntityId`, `AbilityId`, `ItemId`, `ItemInstanceId`).
