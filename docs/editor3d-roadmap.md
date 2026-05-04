# Editor3D — Roadmap

Status of the Editor3D as of 2026-05-03 plus a prioritized roadmap to MVP.

## Current state

Editor3D is **feature-complete for visual map authoring** — terrain, static objects, lights, materials, sounds, particle emitters. All major editing affordances work (multi-select, undo/redo, copy/paste, hierarchy with grouping, snap to grid, gizmos). Client-side runtime export of `.omdl` models, `.chunk` terrain, materials, and textures is shipped. See [editor3d.md](editor3d.md) for current structure and [export-pipeline.md](export-pipeline.md) for the shipped export.

**Current iteration loop (today):** to test placed content, the designer manually exports the runtime data, restarts the WorldServer (because spawns are hardcoded in `MapTemplates.cpp`), and re-launches the client. There is **no in-editor playtest mode**. Closing this loop is the primary motivation for [release-pipeline.md](release-pipeline.md) "Run Locally" mode (Tier 1) and, longer-term, an in-editor playtest mode (Tier 3).

The two real gaps to a usable MVP:

1. **Editor → server pipeline.** Authored gameplay data (mob spawns, portals, triggers, player spawns) does not reach the WorldServer. Spawns are hardcoded in `MapTemplates.cpp`. The full design for closing this — including a launcher that delivers updates to player machines — lives in [release-pipeline.md](release-pipeline.md).
2. **MMO entity coverage + authoring UI.** Several MMO-essential entity types aren't placeable today:
   - GameObjects / interactables (doors, chests, mining nodes, mailboxes, AH terminals, banks).
   - NPC role on spawns (vendor / quest-giver / trainer vs hostile mob — currently undifferentiated).
   - Waypoints / patrol paths.
   - Creature template authoring (today edited in `CreatureTemplate.cpp`).
   - NavMesh (required for mob pathing).

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
| **9. Export** | 🟡 Partial | Client export shipped. **Server data export missing** — full design in [release-pipeline.md](release-pipeline.md). NavMesh missing. Lightmap baking missing. |
| **10. Polish** | 🟡 Partial | Hierarchy search/filter ✅. **Auto-save, recent files, preferences, minimap missing**. |

## Roadmap tiers

### Tier 1 — Ship-blocking (must have to call this an MVP editor)

| Item | Why it blocks MVP | Where designed |
|---|---|---|
| **Server data export** + production pipeline | Editor output never reaches WorldServer today; spawns are hardcoded. Closing this is the keystone of the editor's value. | [release-pipeline.md](release-pipeline.md) |
| **Auto-save** | Prevents lost work on crash. | (TBD) |
| **Recent files** | UX baseline; the editor opens maps via a dialog every time today. | (TBD) |

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

1. **Verify spawn points + player movement work end-to-end** (current state, in progress).
2. **Tier 1 — schema migration system + first migration + spawn writer + WorldServer DB loader for spawns + Run Locally minimal mode.** Steps 1-6 of [release-pipeline.md](release-pipeline.md) Implementation order. **First milestone where the editor produces playable content without engineer involvement.**
3. **Tier 1 — auto-save + recent files.** Daily-comfort wins; parallelizable with #2.
4. **Tier 2 — broaden Run Locally to portals, triggers, gameobjects, player spawns.** Each is a writer + loader pair; PlayerSpawn requires the race/class Inspector binding.
5. **Tier 2 — creature template editor + spawn linking + NPC role.** Same workflow, do them together. Unlocks vendors, quest-givers, hostile mobs as differentiated entities from a single placement type.
6. **Tier 2 — GameObject / interactable type.** New top-level entity in `EditorWorld`. Connects to `gameobject_template` (created in step 2's `0002_creature_npc_gameobject.sql`).
7. **Tier 2 — NavMesh.** Required for MMO mob pathing. Editor bake step writes to the artifact; WorldServer loads at startup.
8. **Tier 2 — waypoints, snap-to-surface, billboard icons.** Independent items; pick by need.
9. **Tier 3** — as time allows.

## Pointers

- [release-pipeline.md](release-pipeline.md) — release pipeline (export → bundle → launcher → production), not yet implemented.
- [export-pipeline.md](export-pipeline.md) — client-side runtime export (already shipped).
- [editor3d.md](editor3d.md) — current Editor3D structure.

## Where to pick up next

(Self-note for future sessions.) Open work as of 2026-05-03:

- User is verifying that spawn points and player movement work end-to-end. Note: today this requires manual export → WorldServer restart → client relaunch (no in-editor preview). Confirm working before proceeding.
- After that, **step 1 of [release-pipeline.md](release-pipeline.md) Implementation order** — schema migration system + `0001_baseline.sql` + `0002_creature_npc_gameobject.sql` (the locked schema). Then editor `migration.sql` writer for spawns. Then WorldServer DB loader for spawns. Then Run Locally minimal mode. The vertical slice for the spawn entity end-to-end gets the editor's iteration loop alive.
- **Locked design decisions in this session (2026-05-03)** — refer to [release-pipeline.md](release-pipeline.md) for full text:
  - DB-only architecture (no JSON intermediates for DB-bound entities); single `migration.sql` is the source of truth.
  - Single `creature_template` table for mobs + NPCs with `npcflag` BIGINT bitfield (AzerothCore-style).
  - GameObjects on a parallel track (`gameobject_template` / `gameobject_spawn`).
  - PlayerSpawn → `player_create_info` keyed by `(race, class)`. Empty classes = "all classes for the listed races".
  - Asset signing via Ed25519 + libsodium, first-class from day one.
  - Cross-platform: Windows + Linux as first-class targets.
  - Maintenance-window-acceptable for v1 (no hot-reload).
- Tier 2 work (entity placing + property association in the Inspector) is queued but should follow #2 in Order of attack — broader entity types come after the spawn vertical slice proves the architecture.
