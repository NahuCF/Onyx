# MMOEditor3D

3D world editor for terrain, lights, and static-object placement. Authors `.chunk` and material files; runs the [export pipeline](export-pipeline.md) to produce client-ready `Data/`.

Run with `./build/bin/MMOEditor3D`.

## Source layout (`MMOGame/Editor3D/Source/`)

```
Main.cpp                              # Application + Editor3DLayer
Editor3DLayer.cpp/h                   # Top-level editor layer, file menu, panel registration
Commands/EditorCommand.cpp/h          # Command base + TransformCommand, MeshTransformCommand,
                                      #   CreateObjectCommand, DeleteObjectCommand, CommandHistory
Gizmo/TransformGizmo.cpp/h            # Translate / rotate / scale gizmo with axis picking
Map/
├── EditorMapRegistry.cpp/h           # maps.json wrapper around Shared MapRegistry
└── MapBrowserDialog.cpp/h            # Map selection / creation modal
Panels/
├── EditorPanel.h                     # Base class
├── PanelManager.h                    # Registration + ImGui rendering
├── ViewportPanel.cpp/h               # 3D viewport, scene render, picking, gizmo input
├── HierarchyPanel.cpp/h              # Object tree (search, drag, multi-select)
├── InspectorPanel.cpp/h              # Property editor per object type
├── AssetBrowserPanel.cpp/h           # File browser, material previews
├── StatisticsPanel.cpp/h             # Chunk count / FPS / draw stats (hidden by default)
├── LightingPanel.cpp/h               # Directional + ambient + shadow controls
├── TerrainPanel.cpp/h                # Brush tools (raise/lower/smooth/flatten/ramp/paint/holes)
├── ToolbarPanel.cpp/h                # Transform mode + snap toggles
├── MaterialEditorPanel.cpp/h         # Material edit (albedo/normal/RMA/tiling)
└── FileDialog.cpp/h                  # Cross-platform file browser
Rendering/EditorVisuals.h             # IconVisual, WireframeVisual, LightGizmoVisual, PathVisual
Terrain/
├── EditorTerrainSystem.cpp/h         # (legacy — present in source, NOT compiled)
├── TerrainChunk.cpp/h                # Per-chunk GPU resources + mesh
├── TerrainMaterialLibrary.cpp/h      # GPU texture arrays + delegating storage to AssetManager
├── MaterialSerializer.cpp/h          # .material / .terrainmat JSON I/O
└── ChunkLight.h                      # Per-chunk light enums/structs
World/
├── EditorWorld.cpp/h                 # Scene graph (StaticObjects, Lights, Triggers, Portals, …)
├── EditorWorldSystem.cpp/h           # Chunk streaming, mesh-gen thread, terrain editing, export
├── WorldChunk.cpp/h                  # Container: terrain + lights + objects + sounds
└── WorldTypes.h                      # EditorLight, ChunkObject, SoundEmitter, MeshMaterialEntry
```

`EditorTerrainSystem` is no longer compiled — `EditorWorldSystem` is the primary chunk manager. The legacy header is kept for transitional reference.

## Editor3DLayer

`Editor3DLayer.h/cpp` is the top layer pushed by `Main.cpp`.

### Lifecycle

- `OnAttach()` — register panels, load map registry.
- `OnUpdate()` — keyboard shortcuts (when a map is loaded).
- `OnImGui()` — render dockspace, panels, dialogs, file menu.

### Panel registration order

1. `ViewportPanel` (primary 3D view, terrain material library)
2. `HierarchyPanel`
3. `InspectorPanel` (linked to ViewportPanel)
4. `AssetBrowserPanel` (material callbacks)
5. `StatisticsPanel` (initially hidden)
6. `LightingPanel`
7. `TerrainPanel`
8. `MaterialEditorPanel` (initially hidden)

### Default dockspace layout

| Region | Panel |
|---|---|
| Center | Viewport |
| Left-top | Hierarchy |
| Left-bottom | Lighting |
| Right-top | Inspector |
| Right-bottom | Terrain |
| Bottom-center | Asset Browser |

### File menu

| Item | Action |
|---|---|
| Open Map | `MapBrowserDialog` |
| Save | Save dirty chunks via `EditorWorldSystem::SaveDirtyChunks` |
| Export Runtime Data… | `EditorWorldSystem::ExportForRuntime("Data", mapId)` — see [export-pipeline.md](export-pipeline.md) |
| Export to Database… | (gated by `HAS_DATABASE`) DB export of map metadata |
| Exit | Quit |

`RenderRuntimeExportDialog()` is a modal showing the export log: model / chunk / texture / material counts plus any errors.

## EditorWorldSystem

`World/EditorWorldSystem.h/cpp`. Owns chunk streaming, terrain editing, and the export pipeline.

### Init & frame

```cpp
void Init(const std::string& chunksDirectory);
void Shutdown();
void Update(const glm::vec3& cameraPos, const glm::mat4& viewProj, float dt);
void RenderTerrain(Shader* terrainShader, const glm::mat4& viewProj,
                   PerChunkCallback perChunkSetup = nullptr);
```

The `PerChunkCallback` lets the viewport set per-chunk uniforms (e.g., layer→array-index mapping) before each draw.

### Terrain queries & editing

```cpp
float GetHeightAt(float worldX, float worldZ);
bool  GetHeightAt(float worldX, float worldZ, float& outHeight);
void  SetHeightAt(float worldX, float worldZ, float height);

void RaiseTerrain(x, z, radius, amount);
void LowerTerrain(x, z, radius, amount);
void SmoothTerrain(x, z, radius, strength);
void FlattenTerrain(x, z, radius, targetHeight, hardness = 0.5f);
void RampTerrain(startX, startZ, startH, endX, endZ, endH, width);
void PaintTexture(x, z, radius, int layer, strength);
void PaintMaterial(x, z, radius, const std::string& materialId, strength);
void SetHole(x, z, bool isHole);
```

### Chunk management

```cpp
void EnsureChunkLoaded(int32_t cx, int32_t cz);
WorldChunk* CreateChunk(int32_t cx, int32_t cz);
void DeleteChunk(int32_t cx, int32_t cz);
void SaveDirtyChunks();
void SaveChunk(int32_t cx, int32_t cz);
```

### Edit snapshots (undo/redo)

```cpp
void BeginEdit(float worldX, float worldZ, float radius);
void EndEdit();
void CancelEdit();
```

`BeginEdit` snapshots the affected chunks' `TerrainChunkData` into `m_CurrentEditSnapshot` so commands in `Commands/` can undo terrain ops.

### Configuration

```cpp
void SetDefaultMaterialIds(const std::string ids[MAX_TERRAIN_LAYERS]);
void SetNormalMode(bool sobel, bool smooth);
void SetDiamondGrid(bool enabled);
void SetMeshResolution(int resolution);

StreamingSettings& GetSettings();         // loadDistance, unloadDistance, preloadDistance,
                                          //   maxChunksPerFrame, enableFrustumCulling
const Stats& GetStats() const;            // totalChunks, loadedChunks, visibleChunks, dirtyChunks
const auto& GetChunks() const;
```

### Export

```cpp
struct ExportResult {
    int modelsExported = 0;
    int chunksExported = 0;
    int texturesCopied = 0;
    int materialsExported = 0;
    std::vector<std::string> errors;
    bool success = false;
};

ExportResult ExportForRuntime(const std::string& outputDir, uint32_t mapId);
```

See [export-pipeline.md](export-pipeline.md) for the full flow.

### Internals

- `m_Chunks: unordered_map<int32_t, WorldChunk>` — keyed by `(cx<<16)|cz`.
- `m_KnownChunkFiles: unordered_set<int32_t>` — every chunk known on disk, even if not currently loaded.
- `m_LoadQueue: deque<ChunkLoadRequest>` — distance-priority load queue.
- `m_CurrentEditSnapshot: EditSnapshot` — undo data.
- `m_MeshGenThread`, `m_MeshGenQueue`, `m_MeshReadyQueue` — background mesh generation worker.
- `m_MaterialLayerMap` — material ID → global layer index for shader uniform mapping.
- `m_ObjectChunkMap` — object GUID → chunk key for fast moves.
- `m_Frustum` — for streaming culling.

## EditorWorld

`World/EditorWorld.h/.cpp` is the editor-side scene graph that wraps editable objects (independent of the chunk container).

### Object types & creation

`CreateStaticObject`, `CreateSpawnPoint`, `CreateLight`, `CreateParticleEmitter`, `CreateTriggerVolume`, `CreateInstancePortal`, `CreatePlayerSpawn`, `CreateGroup`.

### Selection / hierarchy / clipboard

- Selection: `Select`, `SelectByGuid`, `Deselect`, `DeselectAll`, `SelectAll`, `GetSelectedObjects`, plus mesh-level selection.
- Hierarchy: `SetParent`, `Unparent`, `GetParentGroup`, `GetWorldMatrix`, `GetRootObjects`, `MoveInDisplayOrder`.
- Grouping: `GroupSelected`, `UngroupSelected`.
- Clipboard: `Copy`, `Paste`, `Duplicate`, `CopyMesh`, `PasteMesh`.

### Visibility filters & gizmo state

- `SetTypeVisible`, `IsTypeVisible` per object type.
- `SetGizmoMode`, `IsSnapEnabled`, `GetSnapValue`, snap for rotation/scale.

## WorldChunk and primary structs

`World/WorldChunk.h` — container for one 64×64m cell:

```cpp
class WorldChunk {
    int32_t m_ChunkX, m_ChunkZ;
    TerrainChunk* m_Terrain;
    std::vector<EditorLight> m_Lights;
    std::vector<ChunkObject> m_Objects;
    std::vector<SoundEmitter> m_Sounds;
};
```

`World/WorldTypes.h`:

```cpp
struct EditorLight {
    LightType type;        // Point = 0, Spot = 1
    glm::vec3 position, direction, color;
    float intensity, range, innerAngle, outerAngle;
    bool castShadows;
};

struct ChunkObject {
    uint64_t guid, parentGuid;
    std::string name, modelPath, materialId;
    glm::vec3 position;
    glm::quat rotation;
    float scale;
    // Collider
    uint8_t colliderType;     // NONE=0, BOX=1, SPHERE=2, CAPSULE=3, MESH=4
    glm::vec3 colliderCenter, colliderHalfExtents;
    float colliderRadius, colliderHeight;
    // Rendering
    bool castsShadow, receivesLightmap;
    uint32_t lightmapIndex;
    glm::vec4 lightmapScaleOffset;
    // Per-mesh materials
    std::vector<MeshMaterialEntry> meshMaterials;
    // Animation
    std::vector<std::string> animationPaths;
    std::string currentAnimation;
    bool animLoop;
    float animSpeed;
};

struct SoundEmitter {
    std::string soundPath;
    glm::vec3 position;
    float volume, minRange, maxRange;
    bool loop;
};
```

## Terrain editor internals

### `TerrainChunk` (`Terrain/TerrainChunk.h`)

```cpp
TerrainChunk(int32_t chunkX, int32_t chunkZ);
void Load(const std::string& basePath);
void LoadFromData(const TerrainChunkData& data);
void Unload();
void CreateGPUResources();   // legacy: CPU + GPU in one
void DestroyGPUResources();

// Split CPU/GPU mesh path
void PrepareMeshCPU(PreparedMeshData& out, const HeightSampler& heightSampler) const;
void UploadPreparedMesh(PreparedMeshData& data);
void SetHeightSampler(HeightSampler sampler);

// Layer management
const std::string& GetLayerMaterial(int layer) const;
void SetLayerMaterial(int layer, const std::string& materialId);
int FindMaterialLayer(const std::string& materialId) const;
int FindUnusedLayer() const;
int FindLeastUsedLayer() const;

// Dirty tracking
void MarkSplatmapDirty();
void MarkMeshDirty();
bool IsDirty() const;
bool IsModified() const;
void ClearDirty();
void ClearModified();
```

### `TerrainMaterialLibrary` (`Terrain/TerrainMaterialLibrary.h`)

```cpp
void Init(const std::string& assetRoot, Onyx::AssetManager* assetManager);

// Material access — delegates to AssetManager
const Onyx::Material* GetMaterial(const std::string& id) const;
Onyx::Material* GetMaterialMutable(const std::string& id);
std::string CreateMaterial(const std::string& name, const std::string& directory = "");
void SaveMaterial(const std::string& id);
void UpdateMaterial(const std::string& id, const Onyx::Material& mat);
std::vector<std::string> GetMaterialIds() const;

// Per-channel textures
Onyx::Texture* GetDiffuseTexture(const std::string& id);
Onyx::Texture* GetNormalTexture(const std::string& id);
Onyx::Texture* GetRMATexture(const std::string& id);
Onyx::Texture* GetDefaultDiffuse() / GetDefaultNormal() / GetDefaultRMA();
Onyx::Texture* LoadOrGetCachedTexture(const std::string& path);

// Texture arrays for terrain shader
void RebuildTextureArrays();
int GetMaterialArrayIndex(const std::string& id) const;
Onyx::TextureArray* GetDiffuseArray() / GetNormalArray() / GetRMAArray();
bool IsArraysDirty() const;
void MarkArraysDirty();
```

### `MaterialSerializer` (`Terrain/MaterialSerializer.h`)

```cpp
bool SaveMaterial(const Onyx::Material& mat, const std::string& path);
bool LoadMaterial(const std::string& path, Onyx::Material& out);
void ScanAndRegisterMaterials(const std::string& assetRoot, Onyx::AssetManager& assets);
void EnsureDefaultMaterials(const std::string& materialsDir, Onyx::AssetManager& assets);
```

`LoadMaterial` reads both `"albedo"` and `"diffuse"` JSON keys for backward compatibility. Materials are saved as `.material` or `.terrainmat` JSON files.

## Map browser

`Map/EditorMapRegistry.cpp/h` — wraps `Shared::MapRegistry`, manages map directories on disk.

`Map/MapBrowserDialog.cpp/h` — ImGui modal for selecting an existing map or creating a new one (with display name, internal name, instance type, max players).

## Shaders (`Editor3D/assets/shaders/`)

| File | Purpose |
|---|---|
| `basic3d.vert/.frag` | Simple colored 3D primitives (cubes, debug shapes) |
| `billboard.vert/.frag` | Camera-facing 2D billboards (icons, markers) |
| `gizmo.vert/.frag` | Transform gizmo (colored axes) |
| `infinite_grid.vert/.frag` | Infinite floor grid with perspective fade |
| `infinite_grid_test.frag` | Variant of the grid shader |
| `model.vert/.frag` | Non-batched model shader (32 point + 8 spot lights, Blinn-Phong) |
| `model_batched.vert` | Batched variant when MDI path is used |
| `picking.vert/.frag` | Object-ID picking (encodes GUID + mesh + type into RGBA) |
| `picking_billboard.vert/.frag` | Billboard picking with alpha test |
| `shadow_depth.vert/.frag` | Per-object shadow pass |
| `shadow_depth_batched.vert` | Batched shadow pass |
| `skinned.vert` | Non-batched skinned vertex (uniform bones) |
| `terrain.vert/.frag` | PBR + Blinn-Phong toggle, 8-layer splatmap, Sobel normals, 32 point + 8 spot lights |

The `model.frag` and `terrain.frag` shaders include `MAX_POINT_LIGHTS=32`, `MAX_SPOT_LIGHTS=8` uniform arrays with per-light range and inner/outer cone falloff (Blinn-Phong, plus PBR variants in terrain).

## Editor3D development notes

- `nlohmann/json` (FetchContent) handles material and chunk JSON.
- `ImGuiFileDialog` (FetchContent) provides the file browser.
- Always rsync to Windows after building (see [build.md](build.md)).
- For ImGui 1.92, image-style widgets use `ImTextureID texId = (ImTextureID)(uintptr_t)(tex->GetTextureID())`.
- A phase-by-phase development checklist is kept in `~/.claude/projects/-home-god-git-Onyx/memory/editor3d-checklist.md`.
