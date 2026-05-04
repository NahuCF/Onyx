# Editor3D — Roadmap

Status of the Editor3D as of 2026-05-03 plus a prioritized roadmap to MVP.

## Current state

Editor3D is **feature-complete for visual map authoring** — terrain, static objects, lights, materials, sounds, particle emitters. All major editing affordances work (multi-select, undo/redo, copy/paste, hierarchy with grouping, snap to grid, gizmos). Client-side runtime export of `.omdl` models, `.chunk` terrain, materials, and textures is shipped. See [editor3d.md](editor3d.md) for current structure and [export-pipeline.md](export-pipeline.md) for the shipped export.

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
| **GameObject / interactable placement** (doors, chests, mining nodes, herbs, mailboxes, AH terminals, banks, quest objects) | MMO-specific. Currently no type exists in `EditorWorld`. Needs a new top-level entity type referencing `gameobject_template`. Designed in [release-pipeline.md](release-pipeline.md). |
| **NavMesh generation** (Recast/Detour) | Bumped from Tier 3 — for an MMO with mob pathing, kiting, and AI movement, navmesh is closer to ship-blocking than polish. Without it, mobs path through walls or stand still. Editor bake step + runtime consumption. |
| **Waypoints / patrol paths** | Mobs without paths just stand still. Required for any non-trivial encounter. |
| **Snap-to-surface** | Placement quality on uneven terrain — currently objects float or clip. |
| **Billboard icons for spawns/triggers/portals/emitters** | Visual disambiguation; today they're undifferentiated cubes/wireframes. |

### Tier 3 — Polish + post-MVP

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

1. **Tier 1 — server data export.** See [release-pipeline.md](release-pipeline.md) for the full design and implementation order. Once spawns flow, encounter design replaces hardcoding and the editor becomes load-bearing for gameplay.
2. **Tier 1 — auto-save + recent files.** Small wins for daily comfort; can be done in parallel with the export work.
3. **Tier 2 — creature template editor + spawn linking + NPC role.** Same workflow, do them together. Unlocks vendors, quest-givers, hostile mobs as differentiated entities from a single placement type.
4. **Tier 2 — GameObject / interactable type.** New entity in `EditorWorld` + `gameobject` / `gameobject_template` DB tables. Required for any interactive content (doors, chests, mining nodes).
5. **Tier 2 — NavMesh.** Required for MMO mob pathing. Editor bake step writes to the artifact; WorldServer loads at startup.
6. **Tier 2 — waypoints, snap-to-surface, billboard icons.** Independent items; pick by need.
7. **Tier 3** — as time allows.

## Pointers

- [release-pipeline.md](release-pipeline.md) — release pipeline (export → bundle → launcher → production), not yet implemented.
- [export-pipeline.md](export-pipeline.md) — client-side runtime export (already shipped).
- [editor3d.md](editor3d.md) — current Editor3D structure.

## Where to pick up next

(Self-note for future sessions.) Open work as of 2026-05-03:

- User is verifying that spawn points and player movement are working before pursuing the server-export pipeline. Confirm those work end-to-end first.
- After that, the natural starting point is **step 1 of [release-pipeline.md](release-pipeline.md)** — schema migration system. Then editor JSON export. Then WorldServer JSON loader.
- Tier 2 work (entity placing + property association) is queued but not started.
