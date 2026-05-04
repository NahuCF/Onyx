# Release Pipeline (Design)

> **Status: design — not yet implemented.** This document defines the workflow for shipping authored content from the Editor3D to live game servers and player machines, including the launcher that delivers updates. As of 2026-05-03 nothing in this doc is built; it serves as the design contract.

## Goals

1. Designer iterates locally with a 5-second test loop ("Run Locally" mode).
2. Designer produces a self-contained release artifact ("Build Release Bundle") and hands it off via PR (later) or zip (today).
3. Production updates are atomic, verifiable, signed, and resumable.
4. Same artifact format for local test and production deploy — no "works on my machine" gap.
5. Maintenance-window-acceptable for v1 (no live hot-reload).
6. Asset signing is **first-class from day one** — retrofitting it later is hard.

## Two export modes

The Editor3D exposes two export buttons. Both produce the **same artifact shape** — only the destination and post-actions differ.

### Mode A — "Run Locally"

Designer's daily iteration loop.

1. Editor produces a release artifact at `dev/`:
   - `dev/Data/` — chunks, models, materials, json data files
   - `dev/migration.sql` — idempotent UPSERT/DELETE for authored DB rows
   - `dev/manifest.json` — version, file hashes, signature
2. Editor connects to the designer's **local Postgres** (already installed) and runs `migration.sql`.
3. Editor spawns `MMOLoginServer` + `MMOWorldServer` as subprocesses, pointing them at `dev/Data/` and the local DB.
4. Editor spawns `MMOClient`, optionally with an auto-login token for a known test account.
5. UI shows a "Stop" button that kills the spawned processes and (optionally) rolls back the local DB to a baseline.

Goal: edit → test in ~5 seconds.

### Mode B — "Build Release Bundle"

Designer handoff path.

1. Editor produces a release artifact at `releases/v{X.Y.Z}/`:
   - `Data/`
   - `migration.sql`
   - `manifest.json` (signed)
   - Optionally `binaries/` if this release ships a code change (Win + Linux subfolders).
2. Designer zips the folder and sends it.
3. Engineer reviews the diff, tags the release, and runs the production deploy (see "Workflow lifecycle" below).
4. **Future**: an "Open PR" button creates a Git branch and PR via GitHub API, replacing the zip handoff.

The artifact is fully self-contained. The engineer always has the option to apply `migration.sql` by hand against production — the launcher and CDN are conveniences, not requirements.

## Designer environment

Hard requirements:

- **Postgres 14+** installed locally with a `mmogame_dev` database. (User has confirmed designers will install Postgres themselves.)
- **All four game binaries** (`MMOLoginServer`, `MMOWorldServer`, `MMOClient`, `MMOEditor3D`) for the designer's platform — bundled with the editor distribution.

The editor on first launch:
- Detects existing `mmogame_dev` DB; runs schema bootstrap (`MMOGame/Database/migrations/*.sql` + `schema.sql`) if missing.
- Validates the Postgres connection; surfaces a clear error if not reachable.

## Cross-platform support

Both Windows and Linux are first-class targets:

- Editor + game binaries already build for both (`linux-debug` / `windows-debug` CMake presets).
- Subprocess commands resolve `.exe` vs no extension based on `_WIN32` / `__linux__`.
- All path handling uses `std::filesystem::path`.
- Launcher is built and shipped per platform: `launcher.exe` (Windows) and `launcher` (Linux).
- The release bundle's `binaries/` folder, when present, contains both platforms.

## Manifest format

`manifest.json` is the keystone. Schema:

```json
{
  "version": "1.4.2",
  "type": "content+code",
  "timestamp": "2026-05-03T14:22:11Z",
  "compat": {
    "min_client_version": "1.4.0",
    "min_server_version": "1.4.0"
  },
  "files": [
    {
      "path": "Data/maps/001/chunks/chunk_0_0.chunk",
      "size": 12345,
      "sha256": "..."
    },
    {
      "path": "Data/maps/001/spawns.json",
      "size": 234,
      "sha256": "..."
    }
  ],
  "migration_sql_sha256": "...",
  "signature": {
    "algorithm": "ed25519",
    "key_id": "release-2026",
    "value": "<base64>"
  }
}
```

Field rationale:

- **`type`** = `"content-only"` lets launchers skip the binary download path. Most updates are content-only — meaningful UX win.
- **`compat`** protects against running stale clients/servers against newer content. Launcher refuses to launch with a clear "please upgrade" message if `installed_client_version < min_client_version`.
- **`files[].sha256`** enables byte-precise delta downloads (only fetch files whose hash changed) and detects CDN/transit corruption.
- **`migration_sql_sha256`** lets ops verify the migration file matches what the editor produced.
- **`signature`** — see "Asset signing" below.

## Asset signing (Ed25519)

Signed from day one. Retrofitting later requires either accepting unsigned legacy artifacts (security hole) or breaking back-compat.

### Algorithm and library

- **Ed25519** signatures (64 bytes; fast verification; small public keys at 32 bytes).
- **libsodium** for both signing and verification (small, audited, MIT). Add as `FetchContent` in editor + launcher CMakeLists.
- OpenSSL is already a project dependency for password hashing — libsodium is a separate, more focused dep for this purpose.

### Key management

| Key | Where it lives | Notes |
|---|---|---|
| **Private** (signing) | Off-repo: 1Password, secure CI secret store, or — for v1 — a local file in `~/.config/onyx/keys/release.key` with `chmod 400`. **Never committed.** | Used by Mode B "Build Release Bundle" and CI. |
| **Public** (verification) | Embedded in launcher source as `constexpr unsigned char[32]` at compile time. | Players cannot replace it without modifying their launcher binary. |
| **`key_id`** in manifest | Identifier (e.g., `"release-2026"`) so the launcher can support multiple keys later. | v1 accepts a single hard-coded key; field is forward-compat for rotation. |

Key rotation is a v2 problem. The `key_id` field exists now so a v2 launcher can accept multiple valid keys without breaking older signed artifacts.

### What gets signed

- The signature covers the entire `manifest.json` **minus the `signature` field**, in canonical JSON form (deterministic key ordering, no extra whitespace).
- Files in `files[]` are protected by their `sha256`, which is *part of* the signed manifest — so trusting the manifest signature transitively trusts every listed file.
- `migration.sql` is protected by `migration_sql_sha256`, also signed via the manifest.

### Verification at the launcher

```
1. Download manifest.
2. Strip signature field; canonicalize JSON.
3. Verify Ed25519 signature against embedded pubkey.
4. If invalid → refuse to apply, surface clear error.
5. Download files; verify each sha256 against the (now-trusted) manifest.
6. Verify migration_sql_sha256 (if applicable to this client — usually only servers run migrations).
7. Apply changes (atomic swap; see "Launcher" below).
```

### What this protects against

- A compromised CDN serving tampered files.
- A leaked artifact getting modified before reaching players.
- Accidental corruption in transit.

It does **not** protect against:

- A compromised build machine (signing key stolen).
- A compromised launcher binary (pubkey replaced).
  - Mitigation: ship the launcher as a code-signed Windows binary or signed Linux package. Out of scope for this doc.

## DB migration model

Two parallel tracks:

| Track | Author | Source location | When run |
|---|---|---|---|
| **Schema migrations** | Engineer | `MMOGame/Database/migrations/NNNN_description.sql` | Before every server start; tracked by `schema_migrations` table |
| **Data migrations** | Editor (designer) | `migration.sql` inside the release artifact | Per release, after schema migrations |

Schema migrations are run **first**, then data migrations.

### Schema migrations

- Numbered: `0001_initial.sql`, `0002_add_collider_columns.sql`, ...
- Tracked on the DB:
  ```sql
  CREATE TABLE IF NOT EXISTS schema_migrations (
      id INTEGER PRIMARY KEY,
      applied_at TIMESTAMPTZ DEFAULT NOW()
  );
  ```
- Server startup applies any unapplied migrations in numeric order.
- Idempotent by ID; safe to re-run.
- The editor never authors schema migrations — they're code changes by the engineer.

### Data migrations

The editor produces idempotent SQL. Per-map, scoped UPSERTs followed by scoped DELETEs:

```sql
BEGIN;

-- Spawns for map_id = 1
INSERT INTO creature_spawn (guid, map_id, position_x, position_y, position_z, ...)
VALUES
    ('<guid_1>', 1, 10.0, 5.0, 0.0, ...),
    ('<guid_2>', 1, 25.0, 8.0, 0.0, ...)
ON CONFLICT (guid) DO UPDATE SET
    position_x = EXCLUDED.position_x,
    position_y = EXCLUDED.position_y,
    position_z = EXCLUDED.position_z,
    ...;

-- Delete spawns removed in this revision
DELETE FROM creature_spawn
WHERE map_id = 1
  AND guid NOT IN ('<guid_1>', '<guid_2>');

-- (same pattern for portal, trigger_volume, player_spawn for this map)

COMMIT;
```

Why this pattern:

- **Idempotent** — re-running is safe; useful when an apply fails mid-way and ops retries.
- **Scoped per map** — editing map 1 never touches map 2's data.
- **Stable GUIDs** — every authored entity carries the editor-side `ChunkObject.guid` (already exists), so identity survives across edits.
- **Single transaction per map** — atomic apply.

## File-vs-DB matrix (which entity goes where)

Decided in design discussion 2026-05-03. See [editor3d.md](editor3d.md) for the editor-side structs.

| Entity | Storage | Rationale |
|---|---|---|
| Static objects (with full collider data) | Chunk file (`OBJS` section, extended) | Visual + spatial; ship-with-client; rarely changed once placed |
| Lights | Chunk file (`LGHT` section) | Visual-only, spatial — already there |
| Sounds / ambient emitters | Chunk file (`SNDS` section) | Visual/audio, spatial — already there |
| Particle emitters | Chunk file (new section, e.g., `PARS`) | Visual-only, spatial |
| Mob spawns | DB (`creature_spawn`) | GM-editable; references `creature_template` by ID; live state is colocated |
| **NPC spawns** (vendors, quest-givers, trainers, flight-masters) | DB (`creature_spawn` + `creature_template.npcflag`) | Same row shape as mob spawns; behavior differentiated by flags + linked tables (vendor stock, gossip, etc.) |
| **GameObjects / interactables** (doors, chests, mining nodes, herbs, fishing nodes, mailboxes, AH terminals, banks, quest objects) | DB (`gameobject` + `gameobject_template`) | Spatially placed but DB-driven behavior; respawn timers, loot, scripts. WoW/AzerothCore-style split. |
| Portals / instance entrances | DB (`portal`) | Server validates use; tooling-friendly |
| Trigger volumes / event zones | DB (new `trigger_volume` table) | Carries event hooks; want to modify event logic without re-export |
| Player spawns | DB (likely extends `player_create_info`) | Small, server-authoritative, tooling-friendly |
| Group objects (editor folders) | Neither — editor-only organizational | Discarded on export |

The chunk format will need new fields in `ChunkObjectData` (collider type/dimensions, animation, lightmap, per-mesh materials) and a new section for particle emitters. The DB will need a new `trigger_volume` table and a `gameobject` / `gameobject_template` pair; `creature_spawn`, `portal`, `player_create_info` already exist.

For the DB-bound entities, the editor exports JSON intermediate files into the artifact (`Data/maps/001/spawns.json`, `gameobjects.json`, `portals.json`, etc.) **and** a SQL migration. WorldServer reads JSON at startup (no DB import step needed for v1); the SQL exists for the production deploy path. Both are derived from the same authored data.

### Decided: creature/NPC schema layout (AzerothCore-style)

A single `creature_template` table holds **all living things** — hostile mobs, vendors, quest-givers, trainers, flight-masters, auctioneers — and an `npcflag` bitfield drives role behavior. GameObjects (interactables) stay on a separate track.

**Why one table for mobs and NPCs:** they share the same row shape (hp, level, faction, model, AI, etc.); a single creature can hold multiple roles (capital-city NPCs are commonly vendor + quest-giver + flight-master); composability via flags beats separate rows or per-role tables. AzerothCore has shipped this pattern at scale.

**Schema:**

| Table | Role |
|---|---|
| `creature_template` | Primary. Stats, model, AI hints, `npcflag` bitfield, `faction`, base loot table ref, scripts. |
| `creature_spawn` | Per-placement: `guid`, `map_id`, `position_x/y/z`, `orientation`, `respawn_secs`, equipment override, modelid override. References `creature_template` by ID. |
| `creature_loot_template` | Drop tables (loot table ID is referenced from `creature_template`). |
| `creature_addon` | Non-combat presentation: mount, idle emote, equipped weapons. |
| `npc_vendor` | Joins on `creature_template.entry`; vendor stock (item ID, max count, restock time). |
| `npc_gossip` / `npc_text` | Quest-giver / trainer dialogue. |
| `npc_trainer` | Trainer's available spells/abilities. |

GameObjects use a parallel but separate track:

| Table | Role |
|---|---|
| `gameobject_template` | Primary. Type (door / chest / mining-node / mailbox / etc.), display ID, behavior data per type. |
| `gameobject_spawn` | Per-placement: `guid`, `map_id`, `position`, `orientation`, `respawn_secs`. |
| `gameobject_loot_template` | What chests/nodes drop. |

**`npcflag` is a bitfield, not an enum.** Use `BIGINT` (PostgreSQL) and bitwise ops. Reserve bits like:

```
0x0001  GOSSIP            (has dialogue / right-click menu)
0x0002  QUEST_GIVER
0x0004  VENDOR
0x0008  TRAINER
0x0010  FLIGHT_MASTER
0x0020  AUCTIONEER
0x0040  BANKER
0x0080  REPAIR
0x0100  STABLE_MASTER
... (reserve 64 bits; AzerothCore uses ~25 today)
```

A creature with `npcflag = 0x0007` is a quest-giver + vendor + has-gossip simultaneously. The editor's Inspector shows checkboxes for each bit; conditional sub-panels (vendor stock list, trainer spell list, gossip text) appear when their bit is set.

**Editor implication:** the `SpawnPoint` placement type stays a single concept. The Inspector is what differentiates a hostile wolf from a quest-giving NPC — through the chosen `creature_template` entry plus its `npcflag`. No new top-level entity type is needed for "NPC vs mob."

**Implementation note:** add this schema as a single migration (`0002_creature_npc_gameobject.sql` or similar) once the schema-migrations system from step 1 of "Implementation order" lands. Existing `creature_spawn` will need to be reconciled with this design.

### MMO-specific authoring gaps (not yet in the editor)

The editor today places `SpawnPoint` and `InstancePortal` but doesn't distinguish:

- **NPC vs hostile mob** — both are `SpawnPoint` today. Inspector needs an "NPC role" field (none/vendor/quest-giver/trainer/flight-master) that drives `creature_template.npcflag` at export.
- **GameObject placements** — currently has no type at all in `EditorWorld`. Needs a new top-level type with a `gameobject_template` reference and per-instance overrides (state, faction, respawn time).
- **Dungeon vs open-world maps** — `MapInstanceType` enum already supports `Dungeon` / `Raid` / `Battleground` / `Arena`. The editor currently authors all maps the same way; dungeons want extra knobs (lockout duration, encounter list, group size).

These are tracked in [editor3d-roadmap.md](editor3d-roadmap.md) Tier 2.

## Launcher

A small, fast, single-binary tool. State machine:

```
[start]
  ↓
fetch versions endpoint → manifest URL
  ↓
download manifest → verify signature
  ↓
compare with installed.json
  ↓
needs update? ──no──→ [launch client with args]
   │
  yes
   ↓
diff file lists by path + sha256 → list of (download, delete) ops
  ↓
download missing/changed files to install.staging/   (parallel, resumable)
  ↓
verify each file's sha256
  ↓
atomic swap: rename install/ ↔ install.staging/
  ↓
update installed.json
  ↓
[launch client with args]
```

Key behaviors:

- **Atomic install** — stage in `install.staging/`, then a single `rename()` swap. No half-installed states; if power dies mid-download, next launch resumes cleanly.
- **Delta downloads** — hash comparison means a content-only update of a 50 MB map ships only the chunks that changed.
- **Parallel downloads** — N concurrent file fetches with backoff on 5xx.
- **Background self-update** — launcher updates itself via the same mechanism (manifest entry for the launcher binary itself; a small bootstrapper handles the swap).
- **Launch args** — passes server endpoint, optional auth token, log path. The client never has to know about updates.
- **Failure modes**:
  - Network failure → retryable error with exponential backoff.
  - Hash mismatch → permanent error, "redownload" button.
  - Signature failure → permanent error, "your launcher may be out of date or compromised".
  - `compat.min_client_version` mismatch → "please update your launcher".

## Workflow lifecycle

```
DESIGNER MACHINE                     PR / GIT                   PRODUCTION
================                     =========                  ==========
Edit content
    ↓
Run Locally
    ↓
Local DB + Local Data/
    ↓
Spawned processes
    ↓
Test
    ↓
[iterate]
    ↓
Build Release Bundle
    ↓
releases/v1.4.2/
    ↓
zip + send (today)
or "Open PR" button (future)
    ↓
                ──────────→  Engineer reviews diff
                                    ↓
                             Tag release
                                    ↓
                             CI: schema migrations  (engineer-authored)
                                    ↓
                             CI: apply data migration to prod DB
                                    ↓
                             CI: upload Data/ + manifest to CDN  ──→  CDN: Data + manifest at v1.4.2/
                                    ↓
                             CI: update versions endpoint        ──→  Versions API: latest = v1.4.2
                                                                          ↓
                                                                      Player launchers detect on
                                                                      next start:
                                                                        1. Fetch manifest
                                                                        2. Verify signature
                                                                        3. Diff files
                                                                        4. Download deltas
                                                                        5. Verify hashes
                                                                        6. Atomic swap
                                                                        7. Launch client
```

Two important properties:

1. **Designer never touches production.** Handoff is a zip (now) or a PR (later). CI handles the rest.
2. **The "manual SQL import" path is just `migration.sql` in the artifact.** Always present; you can review and apply by hand. CI is convenience, not a requirement.

## Implementation order

The work is large but breaks into a clear sequence. Rough effort estimates: S = 1 day, M = 2-4 days, L = 1+ week.

1. **(S) Schema migration system.** `MMOGame/Database/migrations/NNNN_*.sql`, `schema_migrations` table, runner integrated into WorldServer + LoginServer startup.
2. **(M) Editor JSON export.** Extend `ExportForRuntime` to write `spawns.json`, `portals.json`, `triggers.json`, `player_spawns.json` to `Data/maps/{id}/`. Add new section tags or extend existing ones to chunk format for collider/animation/lightmap data on static objects.
3. **(M) WorldServer JSON loader.** Replace hardcoded `MapTemplates.cpp` with a loader that reads the JSON files at startup and populates the same in-memory `MapTemplate` / spawn structures.
4. **(M) Editor migration.sql writer.** Per-map UPSERT/DELETE generation against the DB schema.
5. **(M) "Run Locally" mode.** Subprocess management, local DB connection, log streaming UI panel.
6. **(M) "Build Release Bundle" mode.** Versioned output folder, manifest generation with file hashes.
7. **(S) Manifest format finalized.** JSON schema, canonicalization library.
8. **(S) libsodium integration.** Editor + launcher Ed25519 signing/verification.
9. **(L) Launcher.** Download + verify + atomic install state machine. Both Windows and Linux builds.
10. **(M) Production deploy automation.** CI pipeline that takes a tagged release artifact and pushes to CDN + DB + versions endpoint.
11. **(L, deferred) "Open PR" button.** GitHub API integration in editor.

Steps 1-4 close the editor → server gap and unblock encounter design.
Steps 5-6 enable the designer's iteration + handoff workflow.
Steps 7-10 enable the player-facing launcher path.

## MMO-specific concerns

The release pipeline above is general; a few items deserve special attention because Onyx is targeting an MMO.

### Player schema migrations

When the engineer adds or changes columns on player-state tables (`characters`, `character_inventory`, `character_equipment`, `character_cooldowns`, etc.), those migrations are part of the schema-migrations track described above. Two added concerns at MMO scale:

- **Backfill cost** — `ALTER TABLE characters ADD COLUMN new_currency BIGINT DEFAULT 0` is cheap; computing a derived value across millions of rows is not. Schema migrations that need backfill should ship as a two-phase change (column added with default → background backfill job → column required).
- **Operational risk** — a botched player-state migration is harder to roll back than a botched content migration. Take a DB snapshot before applying.

Authored content migrations (`creature_spawn`, `gameobject`, `portal`, `trigger_volume`) are operationally safer because the editor regenerates them deterministically from the source map files; rolling back is "redeploy the previous version".

### Server cluster restart coordination

Production typically runs multiple WorldServer processes (one per zone shard, or sharded by player range). The deploy needs to:

1. Drain players from the affected processes (or schedule a maintenance window — v1 acceptable).
2. Apply schema + data migrations once (DB is shared).
3. Restart all processes pointing at the new `Data/` location and DB.
4. Open the gates back up.

The artifact is the same regardless of how many processes consume it. Coordination is a deploy-script concern, not a manifest concern.

### Phasing (post-MVP)

WoW-style phasing — the same physical map looks different to different players based on quest progression — affects how `WorldChunk` and entity visibility resolve per-player. Not in v1, but worth being plan-aware:

- Spawns and gameobjects gain a `phase_mask` column or join table.
- Visibility queries (`Grid::ForEachPlayerNear` etc.) filter by player phase.
- Editor needs a "phase" field on placed entities.

The chunk format itself can remain phase-agnostic — visual props and lights don't phase in the typical case. Phasing lives in the DB-bound entity tier.

### Out of scope (separate authoring tracks)

These are needed for an MMO but are **not** authored in the world editor:

- **Loot tables** — `creature_loot_template`, `gameobject_loot_template`. Authored in spreadsheets / SQL; referenced by ID from `creature_template` and `gameobject_template`.
- **Quest data** — quest text, objectives, rewards. Separate quest editor or spreadsheet pipeline.
- **Localization** — string tables for NPC names, item names, dialogue. DB-driven or DBC-style files, separate tooling.
- **Combat balance** — spell tuning, stat curves, level scaling. Spreadsheet → SQL → DB.
- **Achievements / titles / reputation factions** — DB rows, separate from world placement.

The world editor's job is **placement and spatial association**: "this entity exists at this position in this map" and "this spawn point references creature_template ID 4500". The behavior of template 4500 is authored elsewhere.

### Hot reload

Not in v1. Maintenance windows are acceptable. When/if hot reload becomes a requirement:

- WorldServer subscribes to a notification channel (e.g., Postgres `LISTEN/NOTIFY`).
- On notify, swap in-memory tables (creature_template, etc.) atomically.
- Spawns currently in-flight keep their old template until they respawn.

Designable later without breaking the v1 artifact format.

## Open questions for v1

These do not block the design but should be settled before launch:

- **Versions endpoint**: serve a static `latest.json` from CDN, or a real API? Static is simpler and probably enough.
- **CDN choice**: Cloudflare R2, S3, B2? Pricing and egress matter.
- **Per-file compression**: gzip individual files or bundle into a tarball? Per-file enables delta downloads; tarball reduces request count.
- **Telemetry**: does the launcher report install success/failure to a metrics endpoint?
- **Beta channel**: does the launcher support a `beta` versions endpoint for designer-only previews? (Same launcher, different endpoint URL.)
- **Auto-start at boot**: launcher as a system service or only user-launched?

## Pointers

- [editor3d-roadmap.md](editor3d-roadmap.md) — overall Editor3D MVP roadmap; this doc is referenced from Tier 1.
- [export-pipeline.md](export-pipeline.md) — existing client-side `.omdl` + `.chunk` export (already shipped); this design extends it.
- [editor3d.md](editor3d.md) — current Editor3D structure.
- [terrain-and-formats.md](terrain-and-formats.md) — `.chunk` format spec; will need extension for new entity sections.

## Where to pick up next

(Self-note for future sessions.) As of 2026-05-03 nothing in this doc is implemented. The user is verifying that spawn points and player movement work end-to-end before starting the server-export pipeline.

Natural starting point: **step 1 of "Implementation order"** — schema migration system. That establishes the contract for how DB structure changes flow, then steps 2-4 close the editor → server gap. Tier 2 work (entity placing + property association in the editor) is parallel-tracked but not blocking.
