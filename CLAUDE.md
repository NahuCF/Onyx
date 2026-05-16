# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with this repository.

## Project overview

Onyx is a C++/OpenGL game engine with an integrated 3D editor (`MMOEditor3D`). On top of the engine the repository hosts a fully functional multiplayer RPG (MMOGame) — login server, world server, game client, and world editor.

Detailed documentation is split under [docs/](docs/) — see the index below.

## Engineering philosophy

**Build the best long-term solution, not the cheapest short-term one.** When proposing approaches, optimize for the engine and game we want to ship — not for what's fastest to wire up today. Do not recommend "good enough" shortcuts (ImGui for shipping HUDs, pre-rendered glyph PNGs, hand-rolled one-offs) when the project will need a proper system anyway. Surface the real long-term path first; if a shortcut is genuinely warranted (throwaway prototype, unblocking a dependency), call it out explicitly as a stop-gap with the proper plan named.

This applies to architecture decisions, library choices, and tooling: prefer the right foundation over a quick patch, even when the patch would land sooner.

## Quick build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Binaries land in `./build/bin/` (`MMOEditor3D`, `MMOClient`, `MMOLoginServer`, `MMOWorldServer`, `OnyxEditor`). There is **no** `build.sh` — use CMake directly. CMake presets (`linux-debug`, `linux-release`, `windows-debug`, etc.) are also available — see [docs/build.md](docs/build.md).

### Canonical tree & build (read this — the old WSL→rsync flow is retired)

The active development tree is the **Windows copy** at `C:\Users\god\Desktop\Onyx`, on branch
`feature/spawn-pack-trigger-and-doc-fixes`, tracked on `origin` (`git@github.com:NahuCF/Onyx.git`).
The UI library and all current feature work live here and on origin — **not** in the WSL tree.

Build on Windows with:
```bat
build_win.bat   :: VsDevCmd (VS 18 Community) + cmake --build build\windows-debug -j 8
```
Configured preset: `windows-debug` (Ninja + MSVC 14.5x + vcpkg `x64-windows-static-md`).
Binaries land in `build\windows-debug\bin\`.

> ⚠️ **Do NOT run the old `rsync -av --delete … /home/god/git/Onyx/ … /Desktop/Onyx/`.**
> The WSL tree at `/home/god/git/Onyx` is stale (`master`, ~2026-05-04) and contains
> **none** of the UI library / feature-branch work. That rsync would `--delete`-clobber the
> live Windows working tree; committed work survives via `.git`/origin, but any uncommitted
> WIP would be lost. Commit + push to origin to checkpoint; treat WSL as legacy/abandoned
> for this stream of work.

## Quick run (MMO)

1. PostgreSQL with schema applied:
   ```bash
   PGPASSWORD=root psql -h localhost -U root -d mmogame -f MMOGame/Database/schema.sql
   ```
2. `./build/bin/MMOLoginServer` (port 7000)
3. `./build/bin/MMOWorldServer` (port 7001)
4. `./build/bin/MMOClient`

See [docs/mmogame-overview.md](docs/mmogame-overview.md) for full project structure, controls, classes, creatures, and the network protocol.

## Coding conventions

- Member vars `m_PascalCase`, methods `PascalCase`, locals `camelCase`, constants/enums `UPPER_SNAKE_CASE`.
- Engine namespace `Onyx`; Editor3D top-level namespace `MMO` and `Editor3D` for terrain.
- `IndexBuffer` constructor takes byte count, not element count: `new IndexBuffer(data, count * sizeof(uint32_t))`.
- `Texture::Bind(slot)` + `Shader::SetInt("u_…", slot)` (no `SetBool`).
- `const` wherever possible; references over pointers when null isn't valid; smart pointers for ownership.

Full conventions in [docs/conventions.md](docs/conventions.md).

## Commit conventions

- Conventional Commits style: lowercase prefix with optional scope — `feat:`, `fix(scope):`, `chore(tooling):`, `docs:`, `refactor:`, `style:`, `perf:`.
- **Never add `Co-Authored-By: Claude` (or any other Claude/AI attribution) to commit messages, PR descriptions, or code comments.** Author the work as the human committer.
- Subject ≤ ~70 chars. Use the body to explain *why*.

## Documentation index

### Build
- [docs/build.md](docs/build.md) — CMake presets, binaries, dependencies, sync.
- [docs/conventions.md](docs/conventions.md) — naming, code style, file org.

### Engine (`Onyx/`)
- [docs/engine-core.md](docs/engine-core.md) — `Application`, `Layer`, `EntryPoint`, vendor libs.
- [docs/engine-rendering.md](docs/engine-rendering.md) — `SceneRenderer`, `AssetManager`, `Model`/`AnimatedModel`, shadows, shader inventory, texture slot conventions, GPU buffers.

### MMOGame
- [docs/mmogame-overview.md](docs/mmogame-overview.md) — running the game, project structure, classes, creatures, maps, network protocol, controls.
- [docs/mmogame-shared.md](docs/mmogame-shared.md) — Shared library: Network, Packets, Spells, Items, Data, World, Types, Scripting base types.
- [docs/mmogame-server.md](docs/mmogame-server.md) — LoginServer + WorldServer: entity components, scripting architecture (ScriptObject/ScriptRegistry/IEntity/IMapContext/CreatureAI/TriggerScript/InstanceScript), AI data model, grid, map, combat, aura, inventory, loot.
- [docs/trigger-volumes.md](docs/trigger-volumes.md) — Server-side trigger volume system: shapes, events, script dispatch.
- [docs/navmesh.md](docs/navmesh.md) — Recast bake (editor) + Detour runtime (WorldServer) + chase-AI integration. `.nav` file format in `MMOShared`.
- [docs/mmogame-client.md](docs/mmogame-client.md) — Client app: `Main.cpp`/`GameLayer`, `IsometricCamera`, `GameRenderer`, `ClientTerrainSystem`, `GameClient` state.

### Editor3D & runtime data
- [docs/editor3d.md](docs/editor3d.md) — Editor3D structure, panels, `EditorWorldSystem`, terrain editor, gizmo, shaders.
- [docs/terrain-and-formats.md](docs/terrain-and-formats.md) — shared terrain library, `.chunk` and `.omdl` file formats, mesh generation.
- [docs/export-pipeline.md](docs/export-pipeline.md) — Editor3D → runtime `Data/` export flow (shipped).
- [docs/editor3d-roadmap.md](docs/editor3d-roadmap.md) — Phase status + tier-prioritized roadmap to MVP (design).
- [docs/release-pipeline.md](docs/release-pipeline.md) — Two-mode export, manifest format with Ed25519 signing, DB migration model, launcher state machine, MMO-specific concerns. Implementation order steps 1-6 (migrations + editor writer + WorldServer DB loader + Run Locally) shipped 2026-05-04; steps 7-13 (broader DB-bound entities, release bundle, manifest, signing, launcher, deploy) still design.

### Reference / design docs
- [docs/SceneRenderer_AssetManager_Spec.md](docs/SceneRenderer_AssetManager_Spec.md) — **Historical implementation plan (2026-02-17).** Has known drifts; prefer `engine-rendering.md` for current API. Useful for design rationale.
- [docs/rendering-pipeline.md](docs/rendering-pipeline.md) — Tutorial on Editor3D viewport rendering, MSAA, SSAO chain (2026-02-22).
- [docs/frustum-culling-math.md](docs/frustum-culling-math.md) — Math derivation for plane/AABB tests (timeless).
- [docs/race-class-system-design.md](docs/race-class-system-design.md) — Race/class design intent (2026-02-19); may not reflect current implementation.

## Memory references

External notes kept under Claude's memory (not in repo):

- **AzerothCore reference** (terrain, pathfinding, ADT/WMO/M2 formats, grid loading, liquid types): `~/.claude/projects/-home-god-git-Onyx/memory/azerothcore-reference.md`
- **Editor3D phase-by-phase development checklist** (Phases 1–10: foundation, viewport, gizmos, panels, terrain, object placement, selection, templates, export, polish): `~/.claude/projects/-home-god-git-Onyx/memory/editor3d-checklist.md`

The MMO and engine follow several AzerothCore/TrinityCore patterns: grid-based visibility, visitor pattern (`ForEachPlayerNear`), spell effects composed from `SpellEffect` structs, auras with periodic ticks, dirty-flag system for delta updates, and cell activation by player proximity.

## Legacy 2D editor

`MMOGame/Editor/` is the legacy isometric 2D tile editor (predecessor to `MMOEditor3D`). Kept for reference; not actively developed. Its `Source/` includes the chunk-based tile map, tileset/painting tools, and panel architecture documented inside that folder.
