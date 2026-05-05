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
rsync -av --delete \
  --exclude='build/' --exclude='.git/' --exclude='.vs/' \
  --exclude='Visual Studio*/' --exclude='cmake-build-*/' --exclude='out/' \
  /home/god/git/Onyx/ /mnt/c/Users/god/Desktop/Onyx/
```

(With user editing directly on Windows, this is no longer the canonical path; left here for reference.)

## Windows build via Visual Studio (recommended for IDE work)

The repo is set up for **manifest-mode vcpkg** + **CMake presets**. After cloning:

1. **One-time** — populate the `vcpkg/` directory:
   ```powershell
   git clone --depth 1 https://github.com/microsoft/vcpkg.git vcpkg
   vcpkg\bootstrap-vcpkg.bat
   ```
2. **Open the project folder** in Visual Studio: *File → Open → Folder…* → repo root. **Do not open any `.sln`/`.slnx`** — VS reads `CMakePresets.json` directly.
3. Pick the **`windows-debug`** configuration. First configure auto-installs libpqxx + OpenSSL via vcpkg (~5–10 min). Subsequent configures use the cache.
4. Pick **`MMOEditor3D.exe`** (or any other target) as the Startup Item.
5. **F5** = build + launch. Cwd is set to the source root so asset paths resolve.

The Ninja generator is what's wired up — no Visual Studio `.sln` is generated, but Solution Explorer shows the same target tree thanks to VS's CMake integration.

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
| libpqxx 8.0.1 | vcpkg manifest (Windows) / `apt install libpqxx-dev` (Linux) | PostgreSQL (LoginServer, WorldServer; Editor's Run Locally) |
| OpenSSL 3.6 | vcpkg manifest (Windows) / `apt install libssl-dev` (Linux) | Password hashing (LoginServer) |

## Triplet choice (Windows)

The Windows presets use `x64-windows-static-md`:
- **static** — libpqxx is linked into each binary (no DLL). Avoids DLL `std::string_view` template-export collisions that occur with the default `x64-windows` (DLL) triplet under MSVC C++20.
- **-md** — dynamic CRT (`/MD` debug `/MDd`). Matches the project's MSVC default; avoids CRT-mismatch link errors.

## Notes

- There is **no** `build.sh` script — invoke CMake directly.
- The `Learning/` subdir is always added to the build (no flag).
- Build outputs (`build/`, `cmake-build-*/`, `Bin/`) are gitignored.
