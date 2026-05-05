# Editor3D — Roadmap

Status of the Editor3D as of 2026-05-05 plus a prioritized roadmap to MVP.

## Current state

Editor3D is **feature-complete for visual map authoring** — terrain, static objects, lights, materials, sounds, particle emitters. All major editing affordances work (multi-select, undo/redo, copy/paste, hierarchy with grouping, snap to grid, gizmos). Client-side runtime export of `.omdl` models, `.chunk` terrain, materials, and textures is shipped. See [editor3d.md](editor3d.md) for current structure and [export-pipeline.md](export-pipeline.md) for the shipped export.

**Current iteration loop (today):** "Run Locally" (File ▸ Run Locally) builds the runtime data, applies the migrations + the editor's idempotent UPSERT/DELETE for `creature_spawn` + `player_create_info`, and spawns LoginServer + WorldServer + Client as subprocesses — single button, no manual restart. There is **no in-editor playtest mode** yet (running the game inside the editor's GL viewport, Unity-Play / Unreal-PIE style, is tracked in Tier 3).

The remaining gap to a usable MVP:

**MMO entity coverage + authoring UI.** Several MMO-essential entity types aren't placeable today:
- GameObjects / interactables (doors, chests, mining nodes, mailboxes, AH terminals, banks).
- NPC role on spawns (vendor / quest-giver / trainer vs hostile mob — currently undifferentiated).
- Waypoints / patrol paths.
- Creature template authoring (today edited in `CreatureTemplate.cpp`).
- NavMesh (required for mob pathing).
- Portals + trigger volumes — placement works in the editor but the writer + WorldServer loader aren't wired yet (next step in [release-pipeline.md](release-pipeline.md) "Implementation order" #7).

## Phase-by-phase status

| Phase | Status | Remaining items |
|---|---|---|
| **1. Core Foundation** | ✅ Done | All `World/` structs, `EditorWorld` class, chunk serialization for terrain + lights + objects. |
| **2. Viewport** | ✅ Done | Picking, selection outline, frustum culling, focus-on-selection. *Minor*: only lights have billboard icons; spawns/triggers/portals/emitters render as cubes/wireframes via `EditorVisuals`. |
| **3. Gizmos & Transform** | 🟡 Partial | Move/rotate/scale + grid snap + full undo/redo. **Snap-to-surface missing**. |
| **4. Panels** | ✅ Done | All 11 panels live. |
| **5. Terrain** | 🟡 Mostly done | Heightmap + 8-layer splatmap, all sculpt brushes, holes, brush controls. **Water/liquid surfaces missing**. |
| **6. Object Placement** | 🟡 Mostly done | Static, spawn, light, particle, trigger, portal placement. **Waypoints/patrol paths missing**. |
| **7. Selection & Editing** | ✅ Done | Multi-select, copy/paste/duplicate, delete, group, lock, type-visibility. (Box-select intentionally not pursued — multi-select via shift-click + hierarchy is sufficient.) |
| **8. Templates** | ❌ Not started | No creature template editor, no particle template editor, no spawn↔template linking, no template panel. |
| **9. Export** | 🟡 Partial | Client export shipped. **Server data export shipped for `creature_spawn` + `player_create_info`** (Tier 1, 2026-05-04). **Editor ↔ DB round-trip on map open / save shipped 2026-05-05** (loads existing rows back into `EditorWorld` and re-applies the `MigrationSqlWriter` output via `pqxx::nontransaction` on Ctrl+S/autosave — no JSON sidecar). Portals, triggers, gameobjects pending (Tier 2). NavMesh missing. Lightmap baking missing. |
| **10. Polish** | 🟡 Partial | Hierarchy search/filter ✅. Auto-save ✅. Recent files ✅. Editor preferences (JSON) ✅. **Minimap missing**. |

## Roadmap tiers

### Tier 1 — Ship-blocking (must have to call this an MVP editor)

| Item | Status | Where designed |
|---|---|---|
| **Server data export** (creature_spawn + player_create_info) | ✅ Shipped 2026-05-04 | [release-pipeline.md](release-pipeline.md) |
| Schema migration runner + baseline + creature/NPC schema + race/class seed | ✅ Shipped 2026-05-04 (`MMOGame/Database/migrations/0001..0003*.sql`, `MMOGame/Shared/Source/Database/MigrationRunner.{h,cpp}`) | [release-pipeline.md](release-pipeline.md) |
| WorldServer reads new `creature_spawn` shape (incl. guid, position_z, orientation) + delete `MapTemplates.cpp` | ✅ Shipped 2026-05-04 | — |
| `RaceClassRegistry` + Inspector dropdown for `PlayerSpawn` | ✅ Shipped 2026-05-04 (`MMOGame/Editor3D/Source/Data/RaceClassRegistry.{h,cpp}`, `InspectorPanel::RenderPlayerSpawnProperties`) | [release-pipeline.md](release-pipeline.md) |
| Editor `migration.sql` writer (UPSERT/DELETE-keyed, idempotent) | ✅ Shipped 2026-05-04 (`MMOGame/Editor3D/Source/Export/MigrationSqlWriter.{h,cpp}`) | [release-pipeline.md](release-pipeline.md) |
| "Run Locally" minimal mode (export → apply migration → spawn 3 children) | ✅ Shipped 2026-05-04 (`MMOGame/Editor3D/Source/Runtime/{Subprocess,LocalRunSession}.{h,cpp}`) | [release-pipeline.md](release-pipeline.md) |
| **Auto-save** | ✅ Shipped 2026-05-04 (`Editor3DLayer::OnUpdate` autosave check, hooked to `EditorPreferences`) | — |
| **Recent files menu + editor preferences file** | ✅ Shipped 2026-05-04 (`MMOGame/Editor3D/Source/Settings/EditorPreferences.{h,cpp}`) | — |

### Tier 2 — Strongly desired (needed for a usable workflow on real maps)

| Item | Why |
|---|---|
| **Creature template editor** | Today creature behavior (abilities, AI rules, phases) is authored in `CreatureTemplate.cpp`. Editor panel lets the designer iterate without recompiling. |
| **Spawn ↔ template linking** | Spawn points need to reference creature templates by ID. |
| **NPC role on spawns** (vendor / quest-giver / trainer / flight-master / hostile) | MMO-specific. Spawn points today are undifferentiated; the editor needs to set `creature_template.npcflag` semantics so the same `SpawnPoint` type can express both a wolf and a quest-giver. |
| **PlayerSpawn race/class binding** | Inspector affordance — checkbox grid for races + classes (empty classes = "all classes for the listed races"). Determines `player_create_info` row(s) emitted at export. See [release-pipeline.md](release-pipeline.md) "Decided: PlayerSpawn ↔ player_create_info layout". |
| **GameObject / interactable placement** (doors, chests, mining nodes, herbs, mailboxes, AH terminals, banks, quest objects) | MMO-specific. Currently no type exists in `EditorWorld`. Needs a new top-level entity type referencing `gameobject_template`. Designed in [release-pipeline.md](release-pipeline.md). |
| **NavMesh generation** (Recast/Detour) | Bumped from Tier 3 — for an MMO with mob pathing, kiting, and AI movement, navmesh is closer to ship-blocking than polish. Without it, mobs path through walls or stand still. Editor bake step + runtime consumption. |
| **Waypoints / patrol paths** | Mobs without paths just stand still. Required for any non-trivial encounter. |
| **Snap-to-surface** | Placement quality on uneven terrain — currently objects float or clip. |
| **Billboard icons for spawns/triggers/portals/emitters** | Visual disambiguation; today they're undifferentiated cubes/wireframes. |

### Tier 3 — Polish + post-MVP

- **In-editor playtest mode** — run the game *inside the editor's viewport* without spawning separate processes (Unity Play / Unreal PIE style). Different work from "Run Locally": shares the engine's GL context, reuses loaded assets, hot-reloads code. Bigger effort, but the deepest possible iteration loop.
- Lightmap baking — visual quality, big effort.
- Minimap preview.
- Editor preferences/settings.
- Particle template editor.
- Water/liquid surfaces.
- Texture compression in export pipeline (BC1/BC3/BC7).
- LOD chains in `.omdl`.
- **Phasing** support (per-player conditional entity visibility, WoW-style) — see [release-pipeline.md](release-pipeline.md#phasing-post-mvp).
- **Dungeon-template authoring** — `MapInstanceType::Dungeon`/`Raid`/`Battleground`/`Arena` exist server-side, but the editor authors all maps the same way. Needs lockout duration, encounter list, group size knobs.

## Order of attack

Mirrors [release-pipeline.md](release-pipeline.md) Implementation order, with editor-side priorities woven in:

1. ✅ **Verify spawn points + player movement work end-to-end** — done.
2. ✅ **Tier 1 — schema migration system + first migration + spawn writer + WorldServer DB loader for spawns + Run Locally minimal mode + PlayerSpawn race/class binding** — shipped 2026-05-04. **First milestone where the editor produces playable content without engineer involvement.**
3. ✅ **Tier 1 — auto-save + recent files + editor preferences** — shipped 2026-05-04.
4. **Tier 2 — broaden Run Locally to portals, triggers, gameobjects, player spawns.** Each is a writer + loader pair. PlayerSpawn already covered in Tier 1 (single-combo per spawn); multi-combo broadcast is the remaining bit.
5. **Tier 2 — creature template editor + spawn linking + NPC role.** Same workflow, do them together. Unlocks vendors, quest-givers, hostile mobs as differentiated entities from a single placement type.
6. **Tier 2 — GameObject / interactable type.** New top-level entity in `EditorWorld`. Connects to `gameobject_template` (created in step 2's `0002_creature_npc_gameobject.sql`).
7. **Tier 2 — NavMesh.** Required for MMO mob pathing. Editor bake step writes to the artifact; WorldServer loads at startup.
8. **Tier 2 — waypoints, snap-to-surface, billboard icons.** Independent items; pick by need.
9. **Tier 3** — as time allows.

## Pointers

- [release-pipeline.md](release-pipeline.md) — release pipeline (export → bundle → launcher → production). Implementation order steps 1-6 shipped 2026-05-04 (migrations + editor writer + WorldServer loader + Run Locally); steps 7-13 (broader DB-bound entities, release-bundle mode, manifest/signing, launcher, deploy automation) still design.
- [export-pipeline.md](export-pipeline.md) — client-side runtime export (already shipped).
- [editor3d.md](editor3d.md) — current Editor3D structure.

## Where to pick up next

(Self-note for future sessions.) Open work as of 2026-05-05:

**Tier 1 is complete and the iteration loop is live end-to-end.** A character created from the client now spawns at the position+orientation the designer placed in the editor, with the placed static model rendered in front of it.

**Shipped 2026-05-05 (this session):**
- **Full 3D position + orientation through every layer of the stack.** Previously the editor placed PlayerSpawn at an `(x, z, height, orientation)` tuple but only `(x, z)` survived to the client.
  - DB: new migration `MMOGame/Database/migrations/0004_character_position_z_orientation.sql` adds `position_z REAL NOT NULL DEFAULT 0.0` and `orientation REAL NOT NULL DEFAULT 0.0` to `characters`. `Database::CharacterData` carries both; all four character SQL paths (`GetCharactersByAccountId`, `GetCharacterById`, `CreateCharacter`, `SaveCharacter`) read/write them.
  - LoginServer: passes `createInfo->positionZ` / `createInfo->orientation` from `player_create_info` into `Database::CreateCharacter`.
  - WorldServer: `MapInstance::CreatePlayer` takes `(Vec2 position, float height, float orientation, ...)`. `MapManager::TransferPlayer` preserves the existing `MovementComponent.height`/`rotation` across map handoff. `WorldServer::SaveCharacter` writes `entity->GetMovement()->height` / `->rotation` back to the row.
  - Network protocol: `S_EnterWorld` gained `spawnHeight` + `spawnOrientation`. `EntityState`, `S_EntitySpawn`, and `S_EntityUpdate.UPDATE_POSITION` body all carry `height`. **Bumping any of these breaks wire compat with old clients — clean rebuild required.**
  - Client: `RemoteEntity` and `LocalPlayer` carry `height`. `GameRenderer::RenderEntities` honors it (falls back to terrain snap when `0.0f`). Q/E camera rotation (`Main.cpp:149-155`) removed per user request.
- **Editor ↔ DB round-trip for placed entities (no JSON intermediates).** `Editor3DLayer::LoadMap` calls `LoadWorldFromDatabase(mapId)` which queries `Database::LoadCreatureSpawns(mapId)` and `Database::LoadPlayerCreateInfo()` (filtered to the active map by exact `(pos, ori)` grouping), reconstructs `SpawnPoint` and `PlayerSpawn` objects in `EditorWorld`, and inverse-swaps server Z-up → editor Y-up. Save (menu / Ctrl+S / autosave) routes through new `Editor3DLayer::HandleSave()` which calls existing `SaveDirtyChunks()` then `SyncWorldToDatabase()` — the latter invokes `MigrationSqlWriter::EmitCreatureSpawnsForMap` + `EmitPlayerCreateInfo` in-memory and applies the SQL via `pqxx::nontransaction`. Closing and reopening the editor preserves placements without ever touching disk JSON. The previous JSON sidecar prototype was reverted at the user's instruction ("spawn points should be in db").
- **Static model export bug fixes** (`EditorWorldSystem::ExportForRuntime` in `Editor3D/Source/World/EditorWorldSystem.cpp`):
  - Sync-load the model via `Onyx::Model exportModel(modelPath.c_str(), /*loadTextures=*/true)` instead of pulling from the asynchronous `AssetManager` cache. Async-loaded models upload merged GPU buffers but leave per-mesh `m_Vertices`/`m_Indices`/`m_Textures` empty, which is why `Celtic_Idol.omdl` was previously written as 84 bytes.
  - New `resolveTextureSrc` lambda does three-step path resolution (assimp-reported absolute → relative to model dir → heuristic scan of model dir for filenames containing `albedo`/`basecolor`/`normal`/`rma`). Fixes the FBX-references-renamed-asset case (FBX referenced `celtic_idol_03_Unreal_BaseColor.tga`, on disk it's `Celtic_Idol Albedo.png`).
- **Editor close-crash fixed.** Heap corruption on shutdown (CRT block #689) was due to destructor ordering between `LocalRunSession`, `Database`, and the rest of the layer state. New explicit `Editor3DLayer::~Editor3DLayer()` resets `m_RunSession.reset()` first, then calls `m_Database.Disconnect()` inside try/catch, before the rest of the layer's members tear down. Diagnostic CRT debug heap flags were added to `Main.cpp` during the investigation and removed once confirmed fixed.
- **Windows is now the source of truth** for the working tree (`/mnt/c/Users/god/Desktop/Onyx`). The WSL clone (`/home/god/git/Onyx`) is kept as a backup. Build via `build_win.bat` at the project root (calls `VsDevCmd.bat` then `cmake --build build\windows-debug -j 8`). Build presets and rsync flow unchanged otherwise — see [build.md](build.md).

**Outstanding pending user decision:** the existing character `asdfasd` (id=3, account `NahuCF1`) was created before migration `0004` and has a stale position `(5.5, 7.7, 0)` baked into its row. Existing characters do **not** re-read `player_create_info` on login, so the new placement only takes effect for newly-created characters. The user can either `DELETE FROM characters WHERE id = 3` and recreate, or `UPDATE characters SET position_x=…, position_y=…, position_z=…, orientation=… WHERE id = 3`. No action taken in this session.

**Tier 1 prior session (2026-05-04) — still current:** Editor + LoginServer + WorldServer + Client all build cleanly on Windows via the new vcpkg-driven CMake preset, with libpqxx + OpenSSL fetched and built automatically. Run Locally on Windows successfully:
- exports the runtime data (chunks, .omdl models, textures, migration.sql),
- applies the schema migrations (`0001..0004`) against the local Postgres,
- applies the editor's idempotent UPSERT/DELETE for `creature_spawn` and `player_create_info`,
- spawns `MMOLoginServer.exe` + `MMOWorldServer.exe` + `MMOClient.exe` from the editor's own directory (resolved via `Onyx::Platform::GetExecutableDir()`).

**Windows build infra (new in 2026-05-04 session — see [build.md](build.md)):**
- `vcpkg.json` at project root declares libpqxx + openssl as dependencies.
- `vcpkg/` is a shallow clone of `microsoft/vcpkg` (intentionally not a submodule yet — convert later).
- `CMakePresets.json` `windows-debug` / `windows-release` set `toolchainFile` to vcpkg + `VCPKG_TARGET_TRIPLET=x64-windows-static-md` (static lib + dynamic CRT — avoids DLL `std::string_view` template-export collisions seen with `x64-windows`).
- Project bumped to **C++20** across all CMakeLists (libpqxx 8.x requires it).
- `NOMINMAX` + `WIN32_LEAN_AND_MEAN` added to Onyx PUBLIC compile defs (kills Windows.h `min`/`max` macro grief).
- `Onyx/Source/Graphics/Shader.cpp` hardened to fail-fast on shader compile errors with flushed stderr.
- `Onyx/Assets/Shaders/{model,skinned,shadow_depth,shadow_depth_skinned}_batched.vert` downgraded from `#version 460 core` to `#version 450 core + #extension GL_ARB_shader_draw_parameters` (`gl_DrawID` → `gl_DrawIDARB`) — Mesa llvmpipe (WSL2) caps at GL 4.5 and crashes during link otherwise.
- New helper `Onyx/Source/Core/Platform.{h,cpp}` exposes `Onyx::Platform::GetExecutableDir()` (uses `GetModuleFileNameW` on Windows, `readlink("/proc/self/exe", …)` on POSIX).
- `MMOEditor3D` target sets `VS_DEBUGGER_WORKING_DIRECTORY` to `${CMAKE_SOURCE_DIR}` so VS launches with cwd = source root (assets resolve).

**On WSL2 dev machine specifically:** export `GALLIUM_DRIVER=d3d12` to use Mesa's D3D12 backend, which exposes the host NVIDIA GPU and gives full HW-accelerated GL 4.6. Default WSL2 GL is software llvmpipe, capped at 4.5 — too slow for interactive editor work. Already in user's `.bashrc`.

**Next up — Tier 2 (in suggested order):**
1. Broaden Run Locally writers to **portals + trigger volumes + gameobject_spawn**. Same `MigrationSqlWriter` + `Database::Load*` pattern as creature_spawn. Reuses the migration-runner foundation.
2. **Creature template editor panel** — Inspector affordance for `creature_template` rows (level, faction, npcflag bitfield, ai_name, model, loot table). Today these live in `CreatureTemplate.cpp`.
3. **Spawn ↔ template linking** — Inspector dropdown on creature_spawn that picks an `entry` from the editor-known `creature_template` list.
4. **NPC role** (`npcflag` UI: vendor / quest-giver / trainer / flight-master / hostile) — derived from creature_template; surfaced as checkbox row.
5. **GameObject placement** as a new entity type in `EditorWorld` with its own placeable affordance + Inspector + writer.
6. **NavMesh** (Recast/Detour) bake step.

**Locked design decisions** — see [release-pipeline.md](release-pipeline.md) for full text:
- DB-only architecture (no JSON intermediates for DB-bound entities); single `migration.sql` is the source of truth.
- Single `creature_template` table for mobs + NPCs with `npcflag` BIGINT bitfield (AzerothCore-style).
- GameObjects on a parallel track (`gameobject_template` / `gameobject_spawn`).
- PlayerSpawn → `player_create_info` keyed by `(race, class)`. Tier 1 ships single-combo per spawn (Inspector dropdown of 9 race/class pairs); multi-combo broadcast is queued.
- Asset signing via Ed25519 + libsodium — design only, not implemented (Tier 3 of release-pipeline).
- Cross-platform: Windows + Linux as first-class targets — both build clean as of this session.
- Maintenance-window-acceptable for v1 (no hot-reload).
