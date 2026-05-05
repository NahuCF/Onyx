# Coding Conventions

## Naming

| Kind | Style | Examples |
|---|---|---|
| Member variables | `m_PascalCase` | `m_Position`, `m_Health` |
| Methods | `PascalCase` | `GetPosition()`, `ProcessInput()` |
| Local variables / parameters | `camelCase` | `playerPosition`, `deltaTime` |
| Constants / enum values | `UPPER_SNAKE_CASE` | `MAX_PLAYERS`, `TICK_RATE` |
| Types (classes, structs, enums) | `PascalCase` | `MapInstance`, `AuraType` |

## Namespaces

- **Engine**: `Onyx`
- **Editor3D**: `MMO` (top-level) and `Editor3D` (terrain subsystem)
- **MMOGame Shared/Server/Client**: top-level (no enclosing namespace) for game-side code

## Class structure

```cpp
class Example {
public:
    // Public methods first
    void PublicMethod();
    int GetValue() const { return m_Value; }

private:
    // Private methods
    void HelperMethod();

    // Private member variables
    int m_Value;
    std::string m_Name;
};
```

## Style

- `const` wherever possible.
- References over pointers when null is not a valid state.
- Smart pointers (`std::unique_ptr`, `std::shared_ptr`) for ownership; raw pointers for non-owning views.
- Keep methods small and focused on a single responsibility.
- `Texture::Bind(slotNumber)` plus `Shader::SetInt("u_Sampler", slotNumber)` — no `SetBool`; pass `0`/`1` to `SetInt` for booleans.
- `IndexBuffer` constructor takes **byte count**, not element count: `new IndexBuffer(data, count * sizeof(uint32_t))`.

## File organization

- Header-only types (PODs, simple structs) live in `*.h`.
- Implementation lives in `*.cpp` next to the header.
- One main type per `.h`/`.cpp` pair when practical.
- Editor3D adds new files manually to its `CMakeLists.txt` (explicit list); the engine's `Onyx/CMakeLists.txt` uses `GLOB_RECURSE` so new files are picked up on re-configure.

## Database migrations

Files in `MMOGame/Database/migrations/` follow `NNNN_<verb>_<subject>.sql`.

- `NNNN` — zero-padded 4-digit ordinal. The runner reads only the numeric prefix; gaps and renames are safe.
- `<verb>` — one word from this fixed set:
  - `baseline` — bootstrap; reserved for `0001`.
  - `add` — new tables, columns, or indexes.
  - `alter` — modify an existing column type, default, or constraint.
  - `seed` — data-only inserts (`INSERT … ON CONFLICT DO NOTHING`).
  - `drop` — removals.
  - `rename` — renames.
- `<subject>` — the primary table for a single-table change, or a feature/group name for multi-table changes. Use `snake_case`.

Examples (forward-only — historical files predating this rule are not renamed):

| Filename | Reads as |
|---|---|
| `0001_baseline.sql` | bootstrap (special case) |
| `0005_add_portal_writer.sql` | new column or table for portal authoring |
| `0006_seed_quest_giver_npcs.sql` | data inserts only |
| `0007_alter_characters_money_bigint.sql` | type widening |
| `0008_drop_legacy_inventory_v1.sql` | removal |

Each migration runs in its own transaction (Postgres rolls back on failure). Always use `IF NOT EXISTS` / `IF EXISTS` / `ON CONFLICT DO NOTHING` so re-running against a populated DB is a no-op.

## Git commits

Conventional Commits — `<type>(<scope>)?: <subject>`.

**Subject line rule**: imperative mood, lowercase first letter, no trailing period, ≤72 chars. The subject describes what the commit *does*, not what you did ("add Run Locally", not "added Run Locally").

**Type set** (use one of these, no others):

| Type | Use for |
|---|---|
| `feat` | new user-facing capability |
| `fix` | bug fix |
| `refactor` | code restructure with no behavior change |
| `perf` | performance win |
| `docs` | docs only |
| `test` | tests only |
| `build` | CMake, vcpkg, dependencies |
| `ci` | CI workflows / scripts |
| `chore` | release tags, `.gitignore`, file moves with no other intent |

**Scope** (optional): a single short noun naming the subsystem. Common scopes in this repo: `editor3d`, `worldserver`, `loginserver`, `client`, `engine`, `shared`, `db`, `terrain`, `release-pipeline`. Skip the scope when the change is broad or the type already says enough.

**Body** (optional, separated by blank line, wrapped at 72): the *why*, not the *what* — the diff already shows what.

**Footer** (optional):
- `Closes #123` — links issue.
- `BREAKING CHANGE: <description>` — signals API or wire-protocol breaks (e.g., the `S_EnterWorld` packet bump would have used this).

Examples:

```
feat(editor3d): add Run Locally subprocess launcher
fix(worldserver): prevent crash on null target
refactor(engine): split AssetManager texture upload from cache
docs(release-pipeline): align status banner with shipped state
build: bump vcpkg toolchain to x64-windows-static-md
chore(db): rename migrations to <verb>_<subject> convention
```

**Type-choice discipline**: don't tag refactors as `feat`. If the change doesn't let the user/designer do something they couldn't before, it's not a `feat:`. `cleanup AssetManager`, `refactor terrain chunk`, `organize chunk system function/classes` are all `refactor:`.
