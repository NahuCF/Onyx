# Build System

CMake is the primary build system. The legacy `Onyx.sln` works on Windows but is no longer the canonical path.

## Quick build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j $(nproc)
```

## CMake presets (`CMakePresets.json`)

| Preset | Generator | Build type |
|---|---|---|
| `linux-debug` / `linux-release` | Ninja | Debug / Release |
| `windows-debug` / `windows-release` | Ninja (MSVC) | Debug / Release |
| `unix-makefiles-debug` / `unix-makefiles-release` | Unix Makefiles | Debug / Release |
| `linux-profile` | Ninja | RelWithDebInfo (Valgrind/Callgrind) |

Example:
```bash
cmake --preset=linux-debug
cmake --build --preset=linux-debug
```

## Build outputs

After a build, binaries land in `./build/bin/`:

| Binary | Purpose |
|---|---|
| `OnyxEditor` | Engine sandbox (Learning project) |
| `MMOEditor3D` | 3D world editor (terrain, lighting, objects) |
| `MMOClient` | Game client |
| `MMOLoginServer` | Auth + character CRUD (port 7000) |
| `MMOWorldServer` | Game simulation (port 7001) |
| `Learning/*` | Misc learning/demo binaries |

The Onyx engine itself builds as `libOnyx.a` in `./build/lib/`.

## CMake structure

- **Onyx engine** (`Onyx/CMakeLists.txt`) — `file(GLOB_RECURSE …)` over `Source/`. Auto-discovers new files. If a new file isn't picked up, re-run `cmake -B build` (or `touch` the file).
- **MMOGame subprojects** — explicit file lists. New `.cpp`/`.h` files must be added by hand to the relevant `CMakeLists.txt`. Confirmed for `Editor3D`, `Client`, `Shared`, `LoginServer`, `WorldServer`.
- **Precompiled header** — Onyx uses `Source/pch.h`.
- **Optional Assimp** — gated by `ONYX_USE_ASSIMP=ON` (default on).

## Sync to Windows (WSL2 → Windows desktop)

```bash
rsync -av --delete --exclude='build/' --exclude='.git/' \
  /home/god/git/Onyx/ /mnt/c/Users/god/Desktop/Onyx/
```

## Dependencies

Most are vendored under `Onyx/Vendor/` via CMake `FetchContent`:

| Library | Source | Purpose |
|---|---|---|
| GLFW 3.3.9 | FetchContent | Window + input |
| GLEW 2.2.0 | FetchContent | OpenGL extensions |
| GLM 0.9.9.8 | FetchContent | Math (vec/mat/quat) |
| ImGui (docking) | FetchContent | UI |
| stb_image | header-only | Texture decoding |
| Assimp 5.3.1 | FetchContent (optional) | Model import |
| ENet 1.3.18 | FetchContent (MMOGame) | UDP networking |
| ImGuiFileDialog 0.6.7 | FetchContent (Editor3D) | File browser |
| nlohmann/json 3.11.3 | FetchContent (Editor3D, Shared) | JSON I/O |
| libpqxx | system package | PostgreSQL (LoginServer, WorldServer) |
| OpenSSL | system package | Password hashing (LoginServer) |

## Notes

- There is **no** `build.sh` script — invoke CMake directly.
- The `Learning/` subdir is always added to the build (no flag).
- Build outputs (`build/`, `cmake-build-*/`, `Bin/`) are gitignored.
