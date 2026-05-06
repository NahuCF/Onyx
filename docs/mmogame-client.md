# MMOGame — Client

The game client. Renders a Diablo-style isometric view of the World Server's state, handles input, and loads exported runtime data from `Data/`.

## Source layout (`MMOGame/Client/Source/`)

```
Main.cpp                       # Application + GameLayer
GameClient.h/.cpp              # Network + game state (entities, auras, inventory)
Rendering/
├── IsometricCamera.h/.cpp     # Diablo-style camera
└── GameRenderer.h/.cpp        # Frame orchestration, .omdl model cache
Terrain/
└── ClientTerrainSystem.h/.cpp # Chunk loading, mesh upload, height queries
```

Shaders live under `MMOGame/Client/assets/shaders/` — see [engine-rendering.md](engine-rendering.md#client-shaders-mmogameclientassetsshaders).

## Application

`Main.cpp`:
- `MMOClientApp : public Onyx::Application` with spec `{ 1280, 720, "MMO Client" }`.
- Pushes a single `GameLayer`.

`GameLayer::OnUpdate()`:
- 60 FPS frame limiter.
- Triggers renderer init when GL context is ready.
- Handles zone loading transitions.
- Frame: `BeginFrame()` → `RenderTerrain()` → `RenderStaticObjects()` → `RenderEntities()` → `EndFrame()`.

`GameLayer::OnImGui()`:
- Connect / login / character-select / loading screens.
- Error popup.

`GameLayer::HandleGameInput()`:
- WASD / arrows → movement vector to `GameClient`.
- Q / E → `IsometricCamera::RotateYaw(±45°)`.
- Mouse wheel → `IsometricCamera::Zoom(-scroll * 5.0f)`.
- 1 / 2 / 3 → `GameClient::CastAbility(abilities[i])`.
- Tab → `GameClient::CycleTarget()` (next mob).
- Ignores input when ImGui captures it.

## IsometricCamera

`Rendering/IsometricCamera.h/.cpp`. Defaults:

| Property | Value |
|---|---|
| Yaw | 45.0° |
| Pitch | 40.0° from horizontal |
| Distance | 60.0 units (initial) |
| Zoom range | 20.0 — 200.0 |
| FOV | 45.0° |
| Near / Far | 0.1 / 1000.0 |
| Follow speed | 8.0 |

API:
- `SetTarget(vec3)` — desired focus.
- `Update(dt)` — smoothed interpolation toward target.
- `RotateYaw(deltaDegrees)` — clamped 0–360.
- `Zoom(delta)` — clamped to min/max.
- `GetViewMatrix()`, `GetProjectionMatrix(aspect)`.
- `ScreenToWorldRay(screenX, screenY, vpW, vpH, &rayOrigin, &rayDir)` — unproject via inverse MVP.
- `RayPlaneIntersection(rayOrigin, rayDir, planeY, &hitPoint)`.
- Getters: `GetYaw`, `GetPitch`, `GetDistance`, `GetPosition`, `GetTarget`.

## GameRenderer

`Rendering/GameRenderer.h/.cpp`. Owns three shaders (`m_TerrainShader`, `m_EntityShader`, `m_ModelShader`), the camera, the cube mesh used for entities, and the runtime `.omdl` model cache.

### Public API

```cpp
void Init();                               // Load shaders, white texture, cube mesh
void BeginFrame(playerPos, dt, vpW, vpH);  // Viewport + clear (sky blue) + camera update
void RenderTerrain(ClientTerrainSystem&);  // Bind terrain shader, set uniforms, draw chunks
void RenderStaticObjects();                // Bind model shader, draw m_StaticObjects
void RenderEntities(LocalPlayer,
                    map<EntityId, RemoteEntity>,
                    vector<Portal>, vector<Projectile>,
                    ClientTerrainSystem&);
void EndFrame();
void LoadStaticObjects(ClientTerrainSystem&, dataDir);  // Build m_StaticObjects from .chunk
IsometricCamera& GetCamera();
vec2 ProjectToScreen(vec3 worldPos, vpW, vpH);          // World → screen for UI
```

### Lighting

Hardcoded directional sun:
- `m_SunDirection = normalize({ -0.5f, -1.0f, -0.3f })`
- `m_SunColor = { 1.0f, 0.95f, 0.9f }`
- `m_AmbientStrength = 0.35f`

### Runtime model cache

```cpp
struct RuntimeModel {
    unique_ptr<VertexArray> vao;
    unique_ptr<VertexBuffer> vbo;
    unique_ptr<IndexBuffer>  ebo;
    vector<OmdlMeshInfo>     meshes;
    vector<unique_ptr<Texture>> albedoTextures;  // per mesh
    uint32_t totalIndices;
    uint32_t indexType;        // GL_UNSIGNED_INT or GL_UNSIGNED_SHORT
    uint32_t indexByteSize;    // 4 or 2
};

unordered_map<string, unique_ptr<RuntimeModel>> m_ModelCache;
```

`LoadRuntimeModel(path)`:
1. Cache hit? return.
2. `ReadOmdl(path, data)` from shared library — `data` is an `OmdlMapped`, a zero-copy view into a `mmap`'d file (Win32 `MapViewOfFile` / POSIX `mmap`). No `std::vector` copy.
3. Read `data.header.flags`: `OMDL_FLAG_U16_INDICES` selects `indexType` and `indexByteSize`.
4. Allocate VAO/VBO/EBO. `glBufferData` reads straight from `data.vertexData`/`data.indexData` — straight from the mapped page → VRAM, no intermediate buffer.
5. Set layout via `MeshVertex::GetLayout()` (v2 quantized 28 B layout — pos float3 + snorm16x2 oct-normal + half2 UV + snorm16x2 oct-tangent + snorm16x2 bitangent-sign).
6. Derive base directory (`"Data/models/foo.omdl"` → `"Data/"`).
7. Per-mesh: load albedo from `dataDir + meshInfo.albedoPath`.
8. Cache and return. The `OmdlMapped` goes out of scope here — its destructor unmaps the file (the GPU buffers already hold their own copy).

`LoadStaticObjects(terrain, dataDir)`:
1. Iterate `terrain.GetAllObjects()` — each is a `ChunkObjectData`.
2. `LoadRuntimeModel(fullPath)` (cached).
3. Build model matrix: translate → rotateY → rotateX → rotateZ → scale.
4. Append `StaticWorldObject { model*, modelMatrix }` to `m_StaticObjects`.

`RenderStaticObjects()` binds the model shader, iterates `m_StaticObjects`, and per mesh issues `glDrawElementsBaseVertex(meshInfo.indexCount, model->indexType, meshInfo.firstIndex * model->indexByteSize, meshInfo.baseVertex)`. The model vertex shader decodes oct-encoded normals via `OctDecode(a_OctNormal)` — see [shaders/model.vert](../MMOGame/Client/assets/shaders/model.vert).

## ClientTerrainSystem

`Terrain/ClientTerrainSystem.h/.cpp`. Loads exported runtime chunks from `Data/maps/{mapId}/chunks/`.

```cpp
struct ClientTerrainChunk {
    TerrainChunkData data;             // heightmap + splatmap + holes + bounds
    vector<ChunkObjectData> objects;   // static placements

    unique_ptr<VertexArray> vao;
    unique_ptr<VertexBuffer> vbo;
    unique_ptr<IndexBuffer>  ebo;
    unique_ptr<Texture> splatmapTexture0;  // RGBA0 (channels 0–3)
    unique_ptr<Texture> splatmapTexture1;  // RGBA1 (channels 4–7)
    uint32_t indexCount;
    glm::mat4 modelMatrix;             // identity
};
```

Public API:

```cpp
void LoadZone(uint32_t mapId, string basePath);   // Loads basePath/chunks/*.chunk
void UnloadZone();
void Render(Shader* shader);
float GetHeightAt(float worldX, float worldZ) const;  // bilinear
bool HasChunks() const;
const vector<ChunkObjectData>& GetAllObjects() const; // flat across all chunks
```

`LoadZone`:
1. Iterate `basePath/chunks/*.chunk`.
2. For each: `LoadChunkFile(path, fileData)` from the shared library.
3. Move `terrain` and `objects` into a `ClientTerrainChunk`, call `CreateChunkGPU()`:
   - `GenerateTerrainMesh(...)` → upload VBO/EBO/VAO.
   - `SplitSplatmapToRGBA(...)` → create two GL textures.
4. Store in `m_Chunks[PackKey(chunkX, chunkZ)]`.
5. Build flat `m_AllObjects` from all chunks' `objects` vectors.

The Client always loads from the **exported** `Data/maps/{mapId}/chunks/`, never from raw editor `.chunk` files. See [export-pipeline.md](export-pipeline.md).

## GameClient state

`GameClient.h` owns the network connection and authoritative game state:

### `LocalPlayer`

`EntityId`, `name`, `characterClass`, `position` (Vec2), `rotation`, `health`/`maxHealth`, `mana`/`maxMana`, `level`, `experience`, `targetId`, `money`, `abilities` (`vector<AbilityId>`), `cooldowns`, `isCasting`, `castingAbilityId`, `castProgress`, `pendingInputs` (input prediction), `auras` (`vector<ClientAura>`).

### `RemoteEntity`

`unordered_map<EntityId, RemoteEntity>`. Each: `id`, `type`, `name`, `position`, `velocity`, `rotation`, `health`/`maxHealth`, `level`, `moveState`, `isCasting`, `auras`.

### `ClientAura`

`auraId`, `sourceAbility`, `auraType`, `value`, `duration`/`maxDuration`, `casterId`, `IsDebuff()` helper.

### Misc state

- `vector<ClientPortal>` — `id`, `position`, `size`, `destMapName`.
- `vector<ClientProjectile>` — `id`, `sourceId`, `targetId`, `abilityId`, `position`, `spawnTime`.
- `LootWindowState` — `isOpen`, `corpseId`, `money`, `vector<LootItemInfo> items`.
- `ClientInventory` — `array<ItemNetData, INVENTORY_SIZE> slots`, `array<ItemNetData, EQUIPMENT_SLOT_COUNT> equipment`, `CharacterStats stats`.

### Accessors

`GetLocalPlayer()`, `GetEntities()`, `GetPortals()`, `GetProjectiles()`, `GetLootState()`, `GetMapId()`, `GetZoneName()`.

## Controls

See [mmogame-overview.md](mmogame-overview.md#client-controls).
