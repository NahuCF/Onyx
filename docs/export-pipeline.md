# Export Pipeline

The Editor3D exports compiled runtime data to a `Data/` folder that ships alongside the client binary. Inspired by AzerothCore's compiled-data approach: ship custom GPU-ready formats, not raw source assets.

## `Data/` folder structure

```
Data/
├── maps/
│   └── 001/                      # mapId, zero-padded to 3 digits
│       └── chunks/
│           ├── chunk_0_0.chunk   # runtime .chunk (terrain + lights + objects)
│           ├── chunk_0_-1.chunk
│           └── ...
├── models/
│   ├── tree.omdl                 # GPU-ready merged mesh
│   └── rock.omdl
├── materials/                    # editor materials referenced by chunks
│   └── matId/
│       ├── *_albedo.png
│       ├── *_normal.png
│       └── *_rma.png
└── textures/
    ├── tree_diffuse.png          # textures referenced by .omdl files
    └── rock_diffuse.png
```

## Entry point

`MMOGame/Editor3D/Source/World/EditorWorldSystem.cpp`:

```cpp
ExportResult EditorWorldSystem::ExportForRuntime(const std::string& outputDir,
                                                 uint32_t mapId);
```

Triggered from the Editor3D file menu — **File → Export Runtime Data…** — which opens a modal showing the result counts and any errors.

`ExportResult`:

```cpp
struct ExportResult {
    int modelsExported = 0;
    int chunksExported = 0;
    int texturesCopied = 0;
    int materialsExported = 0;
    std::vector<std::string> errors;
    bool success = false;        // == errors.empty()
};
```

## Phases

### 1. Save & gather

```text
SaveDirtyChunks()                  # persist any unsaved editor changes
for each loaded chunk:
    GatherObjectsForChunk(chunk)   # populate chunk's object list from EditorWorld
```

### 2. Create output directories

```
{outputDir}/maps/{mapId:03}/chunks/
{outputDir}/models/
{outputDir}/materials/
```

### 3. Export models → `.omdl`

Collect every unique `modelPath` referenced by chunks and the editor world's static objects. Detect duplicate `.omdl` names produced by different source files (logged as an error).

For each unique source model:

1. Load via `AssetManager` (already cached from editor use).
2. Build an `OmdlData`:
   - `header.meshCount = meshes.size()`
   - `header.totalVertices`, `header.totalIndices` — sums.
   - Allocate `vertexBlob` (`totalVertices * sizeof(MeshVertex) = 56` bytes/vertex).
   - Allocate `indexBlob` (`totalIndices * sizeof(uint32_t) = 4` bytes/index).
   - For each source mesh:
     - Fill `OmdlMeshInfo { indexCount, firstIndex, baseVertex, boundsMin/Max, albedoPath, normalPath }`.
     - Copy referenced textures into `materials/{modelStem}/`, remap paths to be relative to `Data/`.
     - `memcpy` mesh vertices/indices into the blobs at the right offsets.
   - Set global bounds.
3. `WriteOmdl(modelsDir + "/" + omdlName, omdl)`.
4. Record the remap: `modelPathRemap[sourcePath] = "models/" + omdlName`.
5. `result.modelsExported++`.

### 4. Export editor materials

Walk every chunk's `ChunkObject.materialId` plus per-mesh `MeshMaterialEntry.materialId`. For each unique ID, copy `albedoPath`, `normalPath`, `rmaPath` to `materials/{matId}/` and increment `result.materialsExported`.

### 5. Export chunks → runtime `.chunk`

Iterate **`m_KnownChunkFiles`**, not just currently-loaded chunks — the export covers the whole map. Unloaded chunks are loaded into a temporary `WorldChunk`, exported, then dropped from the map.

For each chunk:

1. Build `ChunkFileData`:
   - `mapId` from the parameter.
   - `terrain` — direct copy of the chunk's `TerrainChunkData`.
   - `lights` — convert each `EditorLight` to `ChunkLightData` (type, position, direction, color, intensity, range, inner/outer angles, `castShadows`).
   - `objects` — convert each `ChunkObject` to `ChunkObjectData`:
     - `modelPath = modelPathRemap[obj.modelPath]` (falls back to original if not remapped).
     - `position` direct copy.
     - `rotation = glm::eulerAngles(obj.rotation)` — quaternion → euler radians.
     - `scale = (obj.scale, obj.scale, obj.scale)` — uniform scalar → vec3.
     - `flags = obj.castsShadow ? 1 : 0`.
     - `materialId` direct copy.
2. `WriteChunkFile(runtimeChunksDir + "/chunk_{cx}_{cz}.chunk", fileData, cx, cz)` via the shared `ChunkFileWriter`.
3. `result.chunksExported++`.

### 6. Finalize

`result.success = result.errors.empty()`.

## Runtime side

The Client loads the exported data, never the raw editor files:

- **Chunks** — `ClientTerrainSystem::LoadZone(mapId, "Data/maps/{mapId:03}")` → `LoadChunkFile` from the shared library.
- **Models** — `GameRenderer::LoadRuntimeModel(path)` → `ReadOmdl` → upload merged VBO/EBO → per-mesh `glDrawElementsBaseVertex`. See [mmogame-client.md](mmogame-client.md) for the full loading flow.

## Constraints and known gaps

The current pipeline is intentionally minimal. Future work (rough priority):

1. **Texture compression** — currently raw PNGs. BC1/BC3/BC7 would shrink VRAM and bandwidth substantially.
2. **Streaming-aware bundling** — split `.chunk` into terrain + objects sections that can stream independently.
3. **LOD chains** — bake multiple LOD meshes into `.omdl` (header would gain a per-LOD index table).
4. **Pack files / archives** — group small files into a single archive (one `seek` per chunk vs many).
5. **Hierarchical model references** — share submeshes across `.omdl` files.

The `Data/` layout is currently flat (`Data/models/`, `Data/textures/`). A name-collision fix (hash suffix on duplicate stems) is recorded as a TODO but not implemented — duplicates currently surface as `errors` in the `ExportResult`.

For format specs:
- `.chunk` — see [terrain-and-formats.md#chunk-file-format-chunkformath-chunkioh](terrain-and-formats.md#chunk-file-format-chunkformath-chunkioh).
- `.omdl` — see [terrain-and-formats.md#omdl-custom-model-format](terrain-and-formats.md#omdl-custom-model-format).
