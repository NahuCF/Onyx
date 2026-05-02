# Terrain and File Formats

Shared terrain library used by both Editor3D and the game Client. CPU-only data structures, chunk I/O, and mesh generation — no GL dependencies. Lives in `MMOGame/Shared/Source/Terrain/` and `MMOGame/Shared/Source/Model/`.

## Terrain data (`TerrainData.h`)

Constants:
- `TERRAIN_CHUNK_SIZE = 64.0f` (world units per chunk)
- `TERRAIN_CHUNK_RESOLUTION = 65` (vertices per edge → 65×65 heightmap)
- `TERRAIN_CHUNK_HEIGHTMAP_SIZE = 4225` (65×65)
- `TERRAIN_SPLATMAP_RESOLUTION = 64`
- `TERRAIN_MAX_LAYERS = 8`

`TerrainChunkData` struct:
- `heightmap` — 65×65 floats
- `splatmap` — 8-channel per 64×64 texel
- `holeMask` — `uint64_t` (8×8 grid)
- `materialIds[8]` — strings mapping splatmap channels to materials
- `bounds` — AABB

Helpers:
- `GetTerrainHeight(data, lx, lz)` — bilinear interpolation within a chunk.
- `WorldToChunkCoord(worldX, worldZ, outChunkX, outChunkZ, outLocalX, outLocalZ)`.
- `SplitSplatmapToRGBA(splatmap8, rgba0, rgba1)` — deinterleave into two RGBA textures.

Hole semantics: `holeMask & (1ULL << (z*8+x))` is non-zero when the cell is a hole.

## Chunk file format (`ChunkFormat.h`, `ChunkIO.h`)

Section-based binary container (`.chunk` files).

```
CHNK_MAGIC = 0x43484E4B            // "CHNK"
CHUNK_FORMAT_VERSION = 3           // container version
TERRAIN_FORMAT_VERSION = 4         // standalone TERR section
```

Section tags:
| Tag | Hex | Section |
|---|---|---|
| `TERR_TAG` | `0x54455252` | Terrain |
| `LGHT_TAG` | `0x4C474854` | Lights |
| `OBJS_TAG` | `0x4F424A53` | Objects |
| `SNDS_TAG` | `0x534E4453` | Sounds |

`SectionWriter` (`ChunkIO.h`) is an RAII helper that writes a 4-byte tag + 4-byte size placeholder, lets the caller write the body, then patches the size on destruction. `WriteString` / `ReadString` provide length-prefixed strings.

### TERR section layout

`heightmap[65*65]` floats → `splatmap[8 * 64 * 64]` bytes → `uint64_t holeMask` → `bounds` (vec3 min, vec3 max) → 8 length-prefixed material ID strings.

### Light data (`ChunkLightData`)

```cpp
struct ChunkLightData {
    uint8_t type;            // 0 = Point, 1 = Spot
    float position[3];
    float direction[3];
    float color[3];
    float intensity, range;
    float innerAngle, outerAngle;  // radians
    bool castShadows;
};
```

### Object data (`ChunkObjectData`)

```cpp
struct ChunkObjectData {
    std::string modelPath;       // length-prefixed
    float position[3];
    float rotation[3];           // euler radians
    float scale[3];              // editor exports uniform scale as (s,s,s)
    uint32_t flags;              // bit 0 = castsShadow
    std::string materialId;      // length-prefixed
};
```

## Chunk reader (`ChunkFileReader.h/.cpp`)

```cpp
struct ChunkFileData {
    uint32_t mapId;
    TerrainChunkData terrain;
    std::vector<ChunkLightData> lights;
    std::vector<ChunkObjectData> objects;
};

bool LoadChunkFile(const std::string& path, ChunkFileData& out);
void ReadTerrainSection(std::istream& file, TerrainChunkData& out, int32_t chunkX, int32_t chunkZ);
```

`LoadChunkFile` opens the `.chunk` file, validates the CHNK header, then walks sections by tag and dispatches to the appropriate reader.

## Chunk writer (`ChunkFileWriter.h/.cpp`)

```cpp
bool WriteChunkFile(const std::string& path,
                    const ChunkFileData& data,
                    int32_t chunkX, int32_t chunkZ);
```

Writes a runtime `.chunk` file. Uses the same `SectionWriter` RAII class and `WriteString` helpers as the reader, so the produced file matches the reader format exactly. Used by Editor3D's export pipeline (`EditorWorldSystem::ExportForRuntime`).

## Terrain mesh generator (`TerrainMeshGenerator.h/.cpp`)

CPU-only mesh generation, thread-safe (no GL calls).

```cpp
using HeightSamplerFn =
    std::function<float(int32_t chunkX, int32_t chunkZ, int localX, int localZ)>;

struct TerrainMeshOptions {
    int meshResolution = TERRAIN_CHUNK_RESOLUTION;  // 65
    bool sobelNormals = true;
    bool smoothNormals = false;
    bool diamondGrid = false;
    bool generatePaddedHeightmap = false;
    HeightSamplerFn heightSampler;  // null = clamp to edge
};

struct TerrainMeshData {
    std::vector<float> vertices;          // pos(3) + normal(3) + uv(2) per vertex
    std::vector<uint32_t> indices;
    uint32_t indexCount;
    std::vector<float> paddedHeightmap;   // 67×67 if generatePaddedHeightmap
    int paddedHeightmapResolution;
    std::vector<uint8_t> splatmapRGBA0;   // 64×64×4
    std::vector<uint8_t> splatmapRGBA1;   // 64×64×4
};

void GenerateTerrainMesh(const TerrainChunkData& data,
                         const TerrainMeshOptions& options,
                         TerrainMeshData& out);
```

Pass a `HeightSamplerFn` to stitch boundaries between adjacent chunks; without it, the generator clamps to chunk edges and produces visible seams.

## Map file structure (Editor3D output)

The Editor3D writes per-map directories during save:

```
maps/
├── maps.json                     # Top-level registry (see MapRegistry above)
└── 001/
    └── chunks/
        ├── chunk_0_0.chunk
        ├── chunk_0_-1.chunk
        └── ...
```

Runtime `.chunk` files written by `ExportForRuntime` use the same on-disk format, but live under `Data/maps/{mapId:03}/chunks/` and reference exported `.omdl` model paths.

## `.omdl` custom model format

GPU-ready model binary. Vertices in exact `MeshVertex` layout (56 bytes: pos3 + normal3 + uv2 + tangent3 + bitangent3). Client does `fread()` → `glBufferData()` — no Assimp at runtime.

```
OMDL_MAGIC = 0x4F4D444C   // "OMDL"
OMDL_VERSION = 1
```

`OmdlFormat.h` structs:

```cpp
struct OmdlHeader {
    uint32_t magic, version;
    uint32_t meshCount, totalVertices, totalIndices;
    float boundsMin[3], boundsMax[3];
};

struct OmdlMeshInfo {
    uint32_t indexCount, firstIndex, baseVertex;
    float boundsMin[3], boundsMax[3];
    std::string albedoPath;   // length-prefixed, relative to Data/
    std::string normalPath;   // length-prefixed, relative to Data/
};

struct OmdlData {
    OmdlHeader header;
    std::vector<OmdlMeshInfo> meshes;
    std::vector<uint8_t> vertexBlob;  // totalVertices * sizeof(MeshVertex) = 56 each
    std::vector<uint8_t> indexBlob;   // totalIndices * sizeof(uint32_t) = 4 each
};
```

File layout on disk:

```
OmdlHeader               (fixed)
OmdlMeshInfo[meshCount]  (variable — strings are length-prefixed)
MeshVertex[totalVertices]
uint32_t[totalIndices]
```

API:
- `bool WriteOmdl(const std::string& path, const OmdlData& data)` — `Model/OmdlWriter.h` — used by Editor3D export.
- `bool ReadOmdl(const std::string& path, OmdlData& out)` — `Model/OmdlReader.h` — used by the Client.

The Client uses `glDrawElementsBaseVertex()` per `OmdlMeshInfo` to render each mesh out of the merged VBO/EBO. See [mmogame-client.md](mmogame-client.md) for runtime model loading details and [export-pipeline.md](export-pipeline.md) for the producer side.
