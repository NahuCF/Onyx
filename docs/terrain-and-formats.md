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

GPU-ready model binary. Vertices live on disk in the exact 28-byte `MeshVertex` layout the GPU consumes — no parsing, no decoding step. The runtime memory-maps the file and points `glBufferData` straight at the mapping (zero-copy load).

```
OMDL_MAGIC   = 0x4F4D444C   // "OMDL"
OMDL_VERSION = 2            // v2: quantized 28 B vertex + flags
```

### Why this format

Three optimizations stacked into one format:

1. **Welded vertices.** Editor3D import runs Assimp with `aiProcess_JoinIdenticalVertices`. On the test models this dropped Celtic_Idol from 17,088 → 3,269 unique verts (5.2×) and testJunto2 from 1.97M → 815k (2.4×). Halves GPU vertex shader invocations as well as shrinking files.
2. **Quantized vertex layout.** 28 bytes per vertex instead of 56:
   | Field | Type | Bytes | Notes |
   |---|---|---:|---|
   | position | `float[3]` | 12 | world coords need full precision |
   | octNormal | `int16_t[2]` snorm | 4 | oct-encoded unit normal (Cigolle 2014) |
   | uvHalf | `uint16_t[2]` | 4 | half-float UV |
   | octTangent | `int16_t[2]` snorm | 4 | oct-encoded unit tangent |
   | bitangentSign | `int16_t[2]` snorm | 4 | `.x` carries sign of bitangent; `.y` reserved |

   The vertex shader decodes oct → `vec3` and reconstructs bitangent as `cross(N, T) * sign(.x)`, so the bitangent never travels over the bus.
3. **u16 indices when small.** If `totalVertices < 65536`, `OMDL_FLAG_U16_INDICES` is set and indices are `uint16_t` (half the bytes). Otherwise `uint32_t`.

### `OmdlFormat.h` structs

```cpp
constexpr uint32_t OMDL_VERTEX_BYTES      = 28;
constexpr uint32_t OMDL_FLAG_U16_INDICES  = 1u << 0;

struct OmdlHeader {
    uint32_t magic, version;
    uint32_t flags;                      // OMDL_FLAG_U16_INDICES, …
    uint32_t meshCount, totalVertices, totalIndices;
    float    boundsMin[3], boundsMax[3];
};

struct OmdlMeshInfo {
    uint32_t indexCount, firstIndex;
    int32_t  baseVertex;
    float    boundsMin[3], boundsMax[3];
    std::string albedoPath;   // length-prefixed (u16 len), relative to Data/
    std::string normalPath;
};

// Writer-side (used by editor exporter): owns blobs in std::vector.
struct OmdlData {
    OmdlHeader header;
    std::vector<OmdlMeshInfo> meshes;
    std::vector<uint8_t> vertexBlob;     // totalVertices * 28 bytes
    std::vector<uint8_t> indexBlob;      // totalIndices * (u16 ? 2 : 4) bytes
};

// Reader-side (used by client): zero-copy view into a memory-mapped file.
// Pointers valid as long as OmdlMapped lives — the mapping is unmapped
// in its destructor. Move-only.
struct OmdlMapped {
    OmdlHeader header;
    std::vector<OmdlMeshInfo> meshes;
    const void* vertexData;  size_t vertexBytes;
    const void* indexData;   size_t indexBytes;
    std::unique_ptr<OmdlMapping> mapping;  // RAII unmap
};
```

### File layout on disk

```
OmdlHeader               (fixed, includes flags field)
OmdlMeshInfo[meshCount]  (variable — strings are u16-len-prefixed)
MeshVertex[totalVertices]    raw bytes, 28 each
uint16_t[totalIndices]   if OMDL_FLAG_U16_INDICES, else uint32_t[totalIndices]
```

### API

- `bool WriteOmdl(const std::string& path, const OmdlData& data)` — [Model/OmdlWriter.h](../MMOGame/Shared/Source/Model/OmdlWriter.h) — used by the editor exporter.
- `bool ReadOmdl(const std::string& path, OmdlMapped& out)` — [Model/OmdlReader.h](../MMOGame/Shared/Source/Model/OmdlReader.h) — used by the client. mmaps the file (Win32 `CreateFileMapping`/`MapViewOfFile` or POSIX `mmap`), validates the header, walks the mesh-info section, and fills the blob view pointers — no copies.

### Render side

The Client uses `glDrawElementsBaseVertex()` per `OmdlMeshInfo` to render each mesh out of the merged VBO/EBO. The index type comes from the header flags: `GL_UNSIGNED_SHORT` for u16, `GL_UNSIGNED_INT` for u32. See [mmogame-client.md](mmogame-client.md) for runtime model loading details and [export-pipeline.md](export-pipeline.md) for the producer side.

### Performance

Best-of-5 Release runs vs Assimp+walk on the same file ([MMOGame/Benchmarks/OmdlVsFbx.cpp](../MMOGame/Benchmarks/OmdlVsFbx.cpp)):

| Model | FBX (Assimp) | OMDL v2 | Speedup | File: FBX → OMDL |
|---|---:|---:|---:|---|
| Celtic_Idol (3,269 verts) | 7.93 ms | **0.080 ms** | **99×** | 4.62 MB → 126 KB |
| testJunto2 (815,558 verts) | 8,576 ms | **9.51 ms** | **902×** | 22.3 MB → 30.7 MB |

### Migration

OMDL v2 is a clean break — the reader rejects v1 files with a clear error. Re-export from the editor (`Save → Export Runtime Data` or equivalent) to upgrade existing `Data/models/*.omdl`.
