# MMOGame Overview

A multiplayer RPG built on the Onyx engine. Three runnable binaries plus a shared static library:

- **MMOLoginServer** — auth + character CRUD
- **MMOWorldServer** — game simulation
- **MMOClient** — game client
- **MMOEditor3D** — world editor (terrain, lights, static objects)
- `MMOShared` — static library linked by all of the above

Detailed system docs:
- [mmogame-shared.md](mmogame-shared.md) — Shared library (Network, Packets, Items, etc.)
- [mmogame-server.md](mmogame-server.md) — LoginServer + WorldServer (entity, AI, grid, map, combat, aura, inventory)
- [mmogame-client.md](mmogame-client.md) — Client app, rendering, input
- [terrain-and-formats.md](terrain-and-formats.md) — `.chunk` and `.omdl` formats, terrain library
- [editor3d.md](editor3d.md) — Editor3D structure, panels, terrain editor
- [export-pipeline.md](export-pipeline.md) — Editor3D → runtime `Data/` export

## Running the game

1. **PostgreSQL** with `mmogame` database and schema applied:
   ```bash
   PGPASSWORD=root psql -h localhost -U root -d mmogame -f MMOGame/Database/schema.sql
   ```
2. **LoginServer** (port 7000): `./build/bin/MMOLoginServer`
3. **WorldServer** (port 7001): `./build/bin/MMOWorldServer`
4. **Client**: `./build/bin/MMOClient`

## Project structure

```
MMOGame/
├── Shared/           # Common code linked by everything (libMMOShared.a)
│   └── Source/
│       ├── Data/        # GameDataStore (race/class/create-info DB cache)
│       ├── Database/    # PostgreSQL wrapper
│       ├── Items/       # Item templates
│       ├── Map/         # MapRegistry (maps.json)
│       ├── Model/       # .omdl format (reader, writer, structs)
│       ├── Network/     # ENet wrapper (NetworkClient, NetworkServer), Buffer helpers
│       ├── Packets/     # All packet definitions and serialization
│       ├── Spells/      # SpellDefines, AbilityData
│       ├── Terrain/     # Shared chunk format, mesh generation, chunk I/O
│       ├── Types/       # Game types (DamageType, EquipmentSlot, etc.)
│       └── World/       # Editor world object structs (StaticObject, Light, Portal, …)
├── LoginServer/      # Authentication + character CRUD (port 7000)
├── WorldServer/      # Game simulation (port 7001)
│   └── Source/
│       ├── Entity/      # Entity + Components (Health, Mana, Aura, …)
│       ├── Map/         # MapTemplate, MapInstance, MapManager
│       ├── Grid/        # Spatial partitioning, dirty flags, visitor pattern
│       ├── AI/          # ScriptedAI, EventMap, data-driven CreatureTemplate
│       ├── Scripts/     # Hand-written boss scripts (e.g., ShadowLordAI)
│       └── Items/       # Server-side item logic
├── Client/           # Game client
│   └── Source/
│       ├── Rendering/   # IsometricCamera, GameRenderer
│       └── Terrain/     # ClientTerrainSystem (chunk loading + GPU upload)
├── Editor3D/         # 3D world editor (see editor3d.md)
├── Editor/           # Legacy 2D tile editor (kept for reference)
├── Data/             # (runtime) exported assets — see export-pipeline.md
└── Database/         # SQL schema + DATABASE.md reference
```

## Classes

| Class | Style | Abilities |
|---|---|---|
| Warrior | Melee | Slash, Shield Bash, Charge |
| Witch | Ranged/magic | Fireball, Heal, Frost Bolt |

## Creatures

| Creature | Notes |
|---|---|
| Werewolf | Claw attack; Howl when low HP |
| Forest Spider | Basic attack |
| Shadow Lord (ID 50) | Boss in Dark Forest, 300s respawn |

## Maps

| ID | Name | Size | Notes |
|---|---|---|---|
| 1 | Starting Zone | 100×100 | Spawn (10,10), portal to Dark Forest at (45,10), 2 Werewolf spawns |
| 2 | Dark Forest | 100×100 | Spawn (20,10), portal back at (10,10), spiders + werewolf + Shadow Lord |

Map definitions are partly database-driven (`map_template`, `portal`, `creature_spawn`) and partly hard-coded in `WorldServer/Source/Map/MapTemplates.cpp`.

## Network protocol

- **LoginServer** (port 7000) — TCP-style ENet: register, login, character CRUD.
- **WorldServer** (port 7001) — ENet: auth-token handshake, input, abilities, world state, events.
- Binary serialization via `WriteBuffer` / `ReadBuffer` (`Shared/Source/Network/Buffer.h`).
- World state updates at **20 Hz**; client input at **60 Hz**.
- See `Packets.h` for packet IDs and structs (full list in [mmogame-shared.md](mmogame-shared.md)).

## Database

Schema in `MMOGame/Database/schema.sql`. Reference in `MMOGame/Database/DATABASE.md`.

Tables: `accounts`, `characters`, `sessions`, `character_inventory`, `character_equipment`, `character_cooldowns`, `map_template`, `portal`, `creature_spawn`, `race_template`, `class_template`, `player_create_info`.

`GameDataStore` (singleton, `Shared/Source/Data/GameDataStore.h`) caches race/class/create-info templates from the database at startup. LoginServer uses it for character creation validation; WorldServer uses it for spawn locations.

## Client controls

| Input | Action |
|---|---|
| WASD / arrows | Move |
| Q / E | Camera yaw rotate (45° steps) |
| Mouse wheel | Zoom (5 units/notch) |
| 1 / 2 / 3 | Use ability slot |
| Tab | Cycle target (next mob) |
| Click | Select target (handled in viewport picker) |
| Click portal | Teleport (when within range) |
| F3 | Debug panel |
