# Onyx Engine — Core

The Onyx engine builds as `libOnyx.a`. Application binaries (Editor, MMOClient, MMOEditor3D) link it and provide their own `CreateApplication()`.

## Source layout (`Onyx/Source/`)

| Folder | Purpose |
|---|---|
| `Core/` | `Application`, `Layer`, `LayerStack`, `ImGuiLayer`, `EntryPoint`, base `Ref<T>` |
| `Graphics/` | Window, OpenGL rendering, Renderer2D, SceneRenderer, shaders, models, animation, post-process, asset manager — see [engine-rendering.md](engine-rendering.md) |
| `Maths/` | `Vector2D`, `Vector3`, `Vector4D`, helper functions |
| `Physics/` | Stub — only `Precision.h` (float/double config) |

## Entry point

`EntryPoint.h` provides the actual `main()`. Each application implements:

```cpp
extern Onyx::Application* Onyx::CreateApplication();
```

The engine's `main` constructs the application, runs the layer stack, and shuts down on window close.

## Application

`Onyx::Application` (`Onyx/Source/Core/Application.h`):

- Constructed with `ApplicationSpec { width, height, name }`.
- Owns the window and `AssetManager`.
- `PushLayer(Layer*)` adds a layer (calls `OnAttach()`); the layer stack drives the per-frame loop.
- `GetWindow()` → `Ref<Onyx::Window>`.
- `GetAssetManager()` → `AssetManager&`.
- Static `GetInstance()` for singleton access.

## Layer stack

`Onyx::Layer` is the abstract base:

- Pure `OnUpdate()` — called every frame.
- Optional `OnAttach()`, `OnDetach()`, `OnImGui()`.
- The layer stack is the primary unit of organization for game/editor logic. Each application typically pushes one game/editor layer plus an `ImGuiLayer` for UI.

## ImGui

Docking + viewports are enabled. `ImGuiLayer` handles begin/end frame and rendering. App-side panels live in their respective subprojects (Editor3D panels under `MMOGame/Editor3D/Source/Panels/`).

## Window + OpenGL

- GLFW for windowing/input.
- GLEW for OpenGL extension loading.
- 4× MSAA backbuffer by default.
- Window class is in `Graphics/` (lifecycle owned by `Application`).

## Vendor libraries (`Onyx/Vendor/`)

Fetched via CMake `FetchContent`:

- **GLFW 3.3.9** — window + input
- **GLEW 2.2.0** (glew-cmake) — OpenGL extensions
- **GLM 0.9.9.8** — math
- **ImGui (docking branch)** — UI
- **stb_image** — texture loading
- **Assimp 5.3.1** — model loading (optional, `ONYX_USE_ASSIMP=ON`)

MMOGame subprojects pull additional deps:
- **ENet 1.3.18** — UDP networking
- **ImGuiFileDialog 0.6.7** — Editor3D file browser
- **nlohmann/json 3.11.3** — Editor3D + Shared JSON
- **libpqxx**, **OpenSSL** — system packages, used by LoginServer / WorldServer

See [build.md](build.md) for build commands.
