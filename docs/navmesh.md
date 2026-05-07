# NavMesh — definitive design

Recast/Detour-backed navigation for the Onyx engine and the MMO. This document
is the canonical spec. Everything here either ships now or is on the agreed
path to the definitive version — no "good enough" shortcuts. Sections marked
**[shipped]** match the current code; **[planned]** is queued work; **[open]**
needs a decision.

This is the implementation behind Phase 1 NavMesh in
[tech-demo-checklist.md](tech-demo-checklist.md#phase-1--mobs-that-walk-and-attack).

---

## 1. Pipeline at a glance

```
              ┌──────────────────────── editor ─────────────────────────┐
              │                                                          │
              │  Source FBX/OBJ ─► ExportForRuntime ─► Data/models/*.omdl│
              │                                                          │
              │   chunks  +  *.omdl  +  off-mesh links  +  area markers  │
              │                              │                           │
              │                              ▼                           │
              │            NavMeshBakeService::Bake (per tile)           │
              │              hash → reuse OR rebuild Recast              │
              │                              │                           │
              │                              ▼                           │
              │           Data/maps/<id>/navmesh.nav (multi-tile)        │
              └──────────────────────────────────────────────────────────┘
                                            │
                                            ▼ on Run Locally
              ┌──────────────────── WorldServer ───────────────────────┐
              │  MapInstance ctor                                      │
              │    NavMesh::LoadFromFile (params + addTile per record) │
              │                                                        │
              │  IMapContext::FindPath  → dtNavMeshQuery::findPath +   │
              │                          findStraightPath              │
              │                                                        │
              │  MapInstance::UpdateMobAI                              │
              │    chase target via AggroComponent::chasePath          │
              │    repath when target moves > CHASE_REPATH_DIST        │
              └────────────────────────────────────────────────────────┘
```

---

## 2. File format — `.nav` v1 [shipped]

Lives in [`MMOGame/Shared/Source/Navigation/NavFile.h`](../MMOGame/Shared/Source/Navigation/NavFile.h)
so the editor (writer) and WorldServer (reader) agree on layout without either
side dragging Recast/Detour into MMOShared.

```
MMO::NavFileHeader   header  { magic='ONAV', version=1, flags=0,
                                origin[3], tileWidth, tileHeight,
                                maxTiles, maxPolys, tileCount }
for each tile:
    MMO::NavTileRecord record { tileX, tileY, contentHash, dataSize }
    uint8_t            data[dataSize]    // raw dtCreateNavMeshData output
```

The header carries the `dtNavMeshParams` the runtime uses for
`dtNavMesh::init(const dtNavMeshParams*)`. Each tile's bytes go verbatim to
`dtNavMesh::addTile` with `DT_TILE_FREE_DATA`, so Detour owns and frees them.

`contentHash` fingerprints the inputs that produced this tile. The editor
uses it to skip re-baking unchanged tiles; the runtime ignores it.

Output path: `Data/maps/<padded-3-digit-id>/navmesh.nav` — adjacent to the
`chunks/` directory written by `EditorWorldSystem::ExportForRuntime`.

Versioning policy: this is v1 (the first definitive cut). Future format
changes bump the version field; readers reject mismatches with a clear log
line, no migration shims.

---

## 3. Build inputs

The bake gathers walkable surface geometry from three sources:

### 3a. Terrain chunks [shipped]

For every chunk on disk (loaded or not — see [§5b](#5b-streaming-aware-walk)),
emit a `RES×RES` (default 65×65) position grid from the heightmap, then two
triangles per quad. Quads inside the chunk's 8×8 hole grid are skipped.

### 3b. Static-object meshes [planned: .omdl-driven]

**Definitive design: bake reads `.omdl` files, not source FBX.**

`.omdl` is the existing "compiled" mesh format used by the runtime. Reading
it is microseconds (zero-copy via `OmdlReader`) vs hundreds of ms per FBX
through Assimp. Same bytes the runtime uses → no risk of bake/runtime
divergence. No separate cache format to maintain.

Bake preflight: for every static-object model path, compare source file
mtime vs the corresponding `.omdl` mtime. Stale `.omdl`s get re-exported
through the existing per-model export path before the bake reads them. The
manual *File ▸ Bake NavMesh* trigger gains this preflight; the other two
trigger paths (*Export Runtime Data*, *Run Locally*) already export
everything first, so the preflight is a no-op for them.

The bake reads `MeshVertex.position` (first 12 bytes of the v3 OMDL vertex
record) and the index buffer; ignores normals, UVs, tangents, attachments.

### 3c. Nav-only meshes [planned]

Some authoring needs an invisible collision/nav surface that doesn't render —
classic example: a clean ramp under decorative stair-step geometry, so the
visible stairs look right but the navmesh sees a smooth ramp.

`StaticObject` gains `bool navOnly`. Renderer skips it; bake includes its
mesh as nav input. Same `.omdl` pipeline; same per-tile hash inputs.

---

## 4. Authoring primitives [planned]

Beyond static meshes, the bake needs first-class support for two more
authoring objects so designers can shape the navmesh without engineering
intervention.

### 4a. Off-mesh connections — `OffMeshLink` WorldObject

For jumps, ladders, teleport pads, roof-to-roof leaps. Detour has direct
support via `dtNavMeshCreateParams.offMeshCon*`.

```
class OffMeshLink : public WorldObject {
    glm::vec3 fromPos;
    glm::vec3 toPos;
    float     radius        = 0.6f;   // agent radius for the link
    bool      bidirectional = true;
    uint16_t  flags         = 1;      // dtQueryFilter include flags
    uint8_t   areaId        = RC_WALKABLE_AREA;
};
```

Editor: two transform handles per link, drawn as a colored arc between the
endpoints. Inspector exposes radius / bidi / flags / areaId.

Bake: collected per tile (link is "in" tile T iff fromPos.xz is inside T's
bounds), serialised into `dtNavMeshCreateParams.offMeshConVerts/Rad/Dir/Flags/Areas/UserId/Count`.

### 4b. Convex area markers — `NavArea` WorldObject

For "swimmable," "no-go for faction X," "cost-2x for slow units," etc. The
runtime `dtQueryFilter` reads the area id stored on each polygon and applies
include/exclude/cost behaviour.

```
class NavArea : public WorldObject {
    std::vector<glm::vec3> polygon;   // CCW convex polygon, XZ plane
    float                  hMin, hMax; // vertical range (Y)
    uint8_t                areaId;     // 0..63
    uint16_t               flags;      // dtQueryFilter flags
};
```

Editor: 2D polygon authored in the viewport (click to add vertices; convex-
hull check on commit). Inspector exposes hMin/hMax/areaId/flags.

Bake: `rcMarkConvexPolyArea` runs between `rcBuildCompactHeightfield` and
`rcBuildDistanceField` for every NavArea overlapping the tile.

---

## 5. Editor — bake

### 5a. Trigger paths [shipped]

All three call `Editor3D::NavMeshBakeService::Bake`:

1. **File ▸ Bake NavMesh** — manual, for iterating after edits.
2. **File ▸ Export Runtime Data…** — automatic, after chunk export.
3. **Run Locally** — automatic, before launching the WorldServer.

### 5b. Streaming-aware walk [shipped]

`EditorWorldSystem::ForEachKnownChunkTransient` iterates every `.chunk` file
on disk, temp-loading any that aren't currently in memory and unloading them
when the callback returns. The bake uses this — chunks unloaded by camera
streaming still contribute to the navmesh.

### 5c. Per-tile content hash + reuse [shipped]

Tile size defaults to 32 m. World origin snaps to integer tile multiples so
adding chunks later doesn't reshuffle existing tile identities (which would
bust the hash reuse).

Hash inputs (FNV-1a 64-bit):
- Per-tile build settings (cell size, walkable*, region, slope angle, …)
- For every chunk overlapping the tile: `(cx, cz, file_mtime, file_size)`
- For every static object overlapping the tile: `(guid, position, rotation, scale, model_mtime, model_size)`
- **[planned]** For every off-mesh link in the tile: `(fromPos, toPos, radius, bidi, flags, areaId)`
- **[planned]** For every nav area overlapping the tile: `(polygon[], hMin, hMax, areaId, flags)`

On bake: read prior `.nav`, decode tile records into `(tx, ty) → (hash, bytes)`.
For each tile in the new grid, compute current hash; if it matches the prior
record, reuse the bytes verbatim — no Recast work. Otherwise rebuild.

Net effect: edit one chunk → only the 1–4 overlapping tiles rebuild. The
rest are byte-copied from the prior file.

### 5d. Settings panel [planned]

`NavMeshBuildSettings` is currently hard-coded. Definitive: a per-map
settings panel in the editor UI exposing the full Recast/Detour parameter
set with sliders + tooltips.

| Field | Default | Effect |
|---|---|---|
| `cellSize` | 0.3 m | XZ voxel resolution; finer = more accurate, slower bake |
| `cellHeight` | 0.2 m | Y voxel resolution; ditto |
| `walkableHeight` | 2.0 m | Agent stand-up clearance |
| `walkableClimb` | 0.5 m | Max step-up — set ≥ tallest stair step to merge stepped staircases |
| `walkableRadius` | 0.5 m | Agent radius — drives erosion that creates corridor gaps |
| `walkableSlopeAngle` | 45° | Max walkable slope |
| `maxEdgeLen` | 12 m | Contour simplification edge cap |
| `maxSimplificationError` | 1.3 | Contour fitting tolerance |
| `minRegionArea` | 8² cells | Smallest walkable patch to keep (smaller → noise; bigger → loses thin corridors) |
| `mergeRegionArea` | 20² cells | Coalesce small regions into neighbours |
| `maxVertsPerPoly` | 6 | Polygon vertex cap (Detour requires ≤6) |
| `detailSampleDistance` | 6.0 (×cellSize) | Detail-mesh sampling density |
| `detailSampleMaxError` | 1.0 (×cellHeight) | Detail-mesh height tolerance |
| `tileSize` | 32 m | Tile granularity for incremental rebake |

Settings stored per-map (not global) — different maps need different agent
profiles. Persistence layer mirrors `EditorPreferences` but scoped to the
loaded map; serialized into the map's metadata file alongside chunk data.

UI: new `NavMesh` panel (or section in the existing Terrain panel) with
collapsible sections for *Voxelisation*, *Agent*, *Region*, *Detail*, *Tiles*.
Live "Bake" button uses current values. Tooltip on every slider explains
the effect ("Increase to merge stepped stairs into a continuous walkable
ramp" etc.).

### 5e. Background-thread bake [planned]

Recast itself is single-threaded, but it doesn't have to block the editor
frame. Bake runs on a worker thread; main thread shows "Baking… (X/Y tiles)"
in the status bar. User keeps editing while it runs. Result swap is atomic
on the next editor frame (file write is the natural sync point — once the
new `.nav` lands, the visualizer reloads).

### 5f. Visualizer [shipped]

**View ▸ NavMesh** toggles a coloured polygon overlay. Each tile's polygons
get deterministic per-poly colours (stable across rebakes via index hash).
Filled draw + wireframe pass with `glDepthMask(GL_FALSE)` so the overlay
doesn't poison subsequent passes.

Auto-reloads after every bake. Console logs `[NavMesh Viz] N polys, M tris from <path>` on each load.

Source: `MMOGame/Editor3D/Source/Navigation/NavMeshDebugView.{h,cpp}` plus
`navmesh_overlay.{vert,frag}` shaders.

**Known issue:** when the parallel UI library session's test-scene block
runs in `Editor3DLayer::OnAttach`, toggling the visualizer crashes the
driver inside `Onyx::UI::UIRenderer::Flush()`. Trace: `WidgetTree::DrawWidgetRecursive` → `PushScissor` → `Flush` → `glDrawElements` faults
(the UI batcher has incomplete state init; the visualizer triggers it
indirectly via another pass through that path). Workaround: comment out the
test-scene block in `OnAttach` (lines ~166-220 of the user's edited file)
until the UI session lands its M3 buffer/shader fixes.

---

## 6. WorldServer — runtime [shipped]

`MapInstance` owns one `MMO::NavMesh` per instance. The constructor tries
`Data/maps/<id>/navmesh.nav` (CWD-relative, matching the chunk loader's
convention). Missing or malformed files are non-fatal: `HasNavMesh()` returns
false and AI falls back to straight-line motion.

`MMO::NavMesh::LoadFromFile`:
1. Read header → `dtNavMesh::init(dtNavMeshParams*)` with origin/tileWidth/tileHeight/maxTiles/maxPolys.
2. Per tile record: `dtAlloc` + read bytes + `dtNavMesh::addTile` with `DT_TILE_FREE_DATA`.
3. Allocate `dtNavMeshQuery` with `maxNodes=2048`.

`MMO::NavMesh::FindPath(Vec2 start, Vec2 end, std::vector<Vec2>& out)`:
1. `findNearestPoly` for start/end with extents `(2, 1024, 2)` — XZ-tight, very loose Y so we snap to the surface.
2. `findPath` returns the polygon corridor (max 256 polys).
3. `findStraightPath` extracts waypoint corners.
4. Out vector receives `Vec2 (x, z)` per waypoint.

The same query is exposed through `IMapContext::FindPath` so server scripts
get the same surface as the base AI without depending on the concrete
`NavMesh` class.

> Detour query state is per-instance and **single-threaded**. AI ticks happen
> on the server's main loop, so this is fine today. If we ever fan out AI
> work across threads, give each thread its own `dtNavMeshQuery` — the nav
> data itself is shared-readable.

### 6a. Query filter [planned]

`dtQueryFilter` per-area cost / include-mask configuration becomes meaningful
once `NavArea` markers ship. Default filter accepts everything; faction- or
spell-specific queries pass a filter that excludes/penalises specific area
ids ("can't path through fire," "swimming-only route").

### 6b. Off-mesh link traversal [planned]

When `findStraightPath` returns a waypoint with `DT_STRAIGHTPATH_OFFMESH_CONNECTION`
flag, the AI doesn't walk the gap — it triggers a traversal animation/spell
appropriate to the link's `userId` (jump, climb, teleport). New `IEntity`
hook or a separate `OffMeshTraversal` event lets game code react.

---

## 7. AI — chase [shipped]

`MapInstance::UpdateMobAI` replaces straight-line chase with navmesh-driven
movement when a navmesh is loaded. State on `AggroComponent`:

```cpp
std::vector<Vec2> chasePath;
size_t            chasePathIndex = 0;
Vec2              chasePathTargetSnapshot;
constexpr float   CHASE_REPATH_DIST       = 2.0f;  // metres
constexpr float   CHASE_WAYPOINT_REACHED  = 0.6f;  // metres
```

Each tick when target is out of attack range:
1. Repath if `chasePath` empty/exhausted, or target moved > `CHASE_REPATH_DIST`.
2. Aim at `chasePath[chasePathIndex]`. Within `CHASE_WAYPOINT_REACHED`, advance.
3. `velocity = (aim - pos).Normalized() * speed` — existing integrator picks up.

Path cleared on attack-range, evade, or target loss. If `FindPath` fails (no
nav, no route), straight-line fallback — same behaviour as before this work.

---

## 8. Build dependencies [shipped]

`Recast` and `Detour` via `FetchContent` at `v1.6.0` in
[`CMakeLists.txt`](../CMakeLists.txt). Demo/tests/examples disabled. Linkage:

| Target           | Recast | Detour |
|------------------|:------:|:------:|
| `MMOEditor3D`    |   ✓    |   ✓    |
| `MMOWorldServer` |        |   ✓    |
| `MMOShared`      |        |        |

MMOShared stays Recast-free; the `.nav` file header is the only thing it
needs to know about, and that's pure POD.

Build flow on Windows: vcvars64 must be active (Visual Studio's developer
prompt or `cmd /c "vcvars64.bat" && cmake --build ...`). Targets:
`cmake --build build/windows-debug --target MMOEditor3D MMOWorldServer`.

---

## 9. Implementation status

### Shipped
- [x] Recast/Detour FetchContent
- [x] `.nav` v1 file format (multi-tile, content-hashed records)
- [x] `NavMeshBuilder` (Recast pipeline, single-tile output)
- [x] `NavMeshBakeService` — tiled bake, per-tile hash, reuse-unchanged
- [x] Streaming-aware chunk walk (`ForEachKnownChunkTransient`)
- [x] Editor3D bake hooks: File menu + Export Runtime Data + Run Locally
- [x] WorldServer `NavMesh` runtime (multi-tile load + FindPath)
- [x] `IMapContext::FindPath` exposed to scripts
- [x] DataDrivenAI chase rewired to navmesh
- [x] Editor visualizer with View menu toggle + auto-reload after bake

### Pending — order of work for next session
1. **`.omdl`-driven bake** — replace Assimp loads with `OmdlReader` reads in
   `NavMeshBakeService`; add `.omdl` mtime preflight that re-exports stale
   models per source-FBX mtime. Eliminates the dominant per-bake cost.
2. **Per-map settings panel** — new `NavMeshPanel` (or Terrain-panel
   subsection) with sliders for every `NavMeshBuildSettings` field; serialise
   to the map's metadata so values survive editor restart and travel with the
   map. Bake reads the per-map values instead of the hard-coded defaults.
3. **`OffMeshLink` WorldObject** — editor authoring (two transform handles
   + Inspector), bake serialization into `dtNavMeshCreateParams.offMeshCon*`,
   include in per-tile content hash, runtime traversal hook in AI.
4. **`NavArea` WorldObject** — convex polygon authoring in viewport,
   `rcMarkConvexPolyArea` in the bake, default `dtQueryFilter` honours area
   ids, content-hash inclusion.
5. **`navOnly` flag on StaticObject** — Inspector toggle, renderer skip,
   bake includes mesh as nav input. Hashes already cover this via the
   object's mtime/transform fingerprint.
6. **Background-thread bake** — Recast on a worker thread, "Baking… (X/Y tiles)"
   status bar, atomic file swap on completion, visualizer reload on the
   editor's main thread.
7. **`dtQueryFilter` configuration API** — surface area-id costs/exclusions
   to scripts via `IMapContext` so faction/spell-specific queries become
   trivial. Pairs with `NavArea`.
8. **Off-mesh traversal events** — when `findStraightPath` flags a waypoint
   as an off-mesh corner, emit a server event so AI/animations react. Pairs
   with `OffMeshLink`.

### Open questions
- Settings panel: live tweak (re-bake on every slider change) vs explicit
  *Apply* button? Live is best-feedback but bakes are seconds-long. Probably
  *Apply* for now, switch to live once background-thread bake lands.
- `NavArea` polygon authoring: 2D viewport gizmo (top-down only) vs 3D
  prism with explicit hMin/hMax handles? 2D is simpler; 3D is more
  expressive for multi-storey use. Defer to first real use case.
- Tile size default: 32 m matches one chunk's half-side, but may be too fine
  for terrain-only maps with few obstacles. Make it the first tunable in the
  settings panel and let designers find the right value per map.

---

## 10. Authoring guide — common cases

| Issue | Cause | Fix |
|---|---|---|
| Gaps between buildings | `walkableRadius` erosion overlapping (×2 from each wall) | Reduce `walkableRadius`, or widen the corridor |
| Stairs not walkable | Stair risers are vertical (rcMarkWalkableTriangles excludes >slope) | Set `walkableClimb ≥` step height, or author an inclined ramp under the stairs |
| Doorway not walkable | Doorway < 2× `walkableRadius` wide, or ceiling < `walkableHeight` | Widen / raise, or reduce the agent settings |
| Narrow walkable strip drops out | `minRegionArea` too high — Recast culls it as noise | Lower `minRegionArea` |
| Building plinth blocks access | Step height > `walkableClimb` | Lower the plinth or raise `walkableClimb` |
| Multi-storey interior nav missing | Floors too close (< `walkableHeight`) — voxel columns merge | Increase floor separation, or accept loss of one floor's nav |
| Roof not walkable | Slope > `walkableSlopeAngle` | Flatten the roof, or raise the slope cap |

For invisible-helper geometry (clean ramps under stair-step decoration,
collision-only walls): use `navOnly` static objects once that flag ships.

---

## 11. Memory / cross-session reference

This document is the canonical navmesh spec — start here in any future
session. Implementation lives at:

- Shared: `MMOGame/Shared/Source/Navigation/NavFile.h`
- Editor:  `MMOGame/Editor3D/Source/Navigation/NavMeshBuilder.{h,cpp}`,
           `NavMeshBakeService.{h,cpp}`, `NavMeshDebugView.{h,cpp}`,
           `MMOGame/Editor3D/assets/shaders/navmesh_overlay.{vert,frag}`
- Server:  `MMOGame/WorldServer/Source/Navigation/NavMesh.{h,cpp}`
- AI hook: `MMOGame/WorldServer/Source/Map/MapInstance.cpp` `UpdateMobAI`,
           `MMOGame/WorldServer/Source/Entity/Components.h` `AggroComponent::chasePath`
- Scripts: `MMOGame/WorldServer/Source/Scripting/IMapContext.h` `FindPath`

When picking up the work, the natural starting point is the `.omdl`-driven
bake refactor — biggest perf win and unblocks fast iteration on the settings
panel that follows.
