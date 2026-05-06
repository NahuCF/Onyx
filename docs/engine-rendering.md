# Onyx Engine — Rendering

Multi-draw-indirect (MDI) batching for static and skinned meshes, 4-cascade shadow maps, screen-space AO. The rendering code lives in `Onyx/Source/Graphics/`.

For deeper specs see also `docs/SceneRenderer_AssetManager_Spec.md`, `docs/rendering-pipeline.md`, `docs/frustum-culling-math.md`.

## SceneRenderer

`Onyx/Source/Graphics/SceneRenderer.h`. Per-frame lifecycle:

1. `Begin(view, projection, cameraPos)` — clears batches/lights, computes camera `Frustum`.
2. `SubmitStatic(Model*, meshIdx, worldXform, albedoPath, normalPath)` / `SubmitSkinned(AnimatedModel*, worldXform, boneMatrices[], …)` — accumulate.
3. `RenderShadows(callback)` — 4 cascades; the callback can render extra geometry (e.g., terrain) into the shadow maps.
4. `RenderBatches()` — color pass with per-mesh frustum culling.

Lights are configured between `Begin()` and `RenderBatches()`:

- `SetDirectionalLight(...)`, `SetAmbient(...)`, `SetShadowsEnabled(bool)`, `SetShadowBias`, `SetShadowDistance`, `SetSplitLambda`, `ShowCascades(bool)`.
- `AddPointLight(PointLightData)` — max **32**.
- `AddSpotLight(SpotLightData)` — max **8**.
- `UploadShadowUniforms(shader, slot=2)` — wires the shadow texture array to the given slot. **Terrain must pass slot 6**, models default to slot 2.
- `UploadLightUniforms(shader)` — bind directional + point/spot to a non-batched shader.

### Batching

- Batch key = `uint64_t` hash of `(Model* ptr, albedoPath, normalPath)`.
- `SubmitStatic()` lazily calls `model->BuildMergedBuffers()` to merge all meshes into one VBO/EBO.
- Static draws → `StaticDrawData { mat4 model }` SSBO binding 0.
- Skinned draws → `SkinnedDrawData { mat4 model; int32_t boneOffset, boneCount }` SSBO 0; `mat4[] bones` SSBO 1.
- `DrawIndirectCommand` is the standard GL MDI struct.

### Stats

`SceneRenderer::GetStats() → RenderStats` exposes `meshesSubmitted` and `meshesCulled` for diagnostics.

## CascadedShadowMap

`Onyx/Source/Graphics/CascadedShadowMap.h`:

- `NUM_SHADOW_CASCADES = 4`, default 2048×2048 per layer.
- One `GL_TEXTURE_2D_ARRAY` with 4 layers + a single FBO.
- `Update(view, proj, lightDir, near, far, splitLambda=0.75)` computes the 4 ortho matrices.
- `splitLambda` blends logarithmic (0) vs uniform (1) cascade distribution.
- 3×3 PCF via `sampler2DArrayShadow`.
- Polygon offset 4.0 / 4.0 during the shadow pass.

## Frustum

`Onyx/Source/Graphics/Frustum.h` — single shared implementation for all culling (camera, shadow cascades, terrain chunks). Gribb/Hartmann plane extraction + p-vertex AABB test (`IsBoxVisible`).

`MeshBounds { vec3 worldMin, worldMax }` is computed via Arvo's method (`TransformAABB`).

## Model and AnimatedModel

### `Onyx::Model` — static

Vertex layout (`MeshVertex`, **28 bytes** — quantized v2 layout, also written verbatim into `.omdl` files):
| Loc | Field | C++ type | GL type | Notes |
|---:|---|---|---|---|
| 0 | position | `float[3]` | `vec3` | full precision; offset 0 |
| 1 | octNormal | `int16_t[2]` snorm | `vec2` (snorm) | oct-encoded unit normal (Cigolle 2014) |
| 2 | uvHalf | `uint16_t[2]` half | `vec2` | half-float UV |
| 3 | octTangent | `int16_t[2]` snorm | `vec2` (snorm) | oct-encoded unit tangent |
| 4 | bitangentSign | `int16_t[2]` snorm | `vec2` (snorm) | `.x` carries sign of bitangent; `.y` reserved |

Vertex shaders decode oct → `vec3` and reconstruct bitangent as `cross(N, T) * sign(bitangentSign.x)`. The bitangent never travels over the bus.

Encoding helpers in [Mesh.h](../Onyx/Source/Graphics/Mesh.h): `OctEncodeNormal`, `FloatToHalf`, `MakeMeshVertex`. `Model::ParseFromFile` calls `MakeMeshVertex` after applying the world transform.

`VertexLayout` ([VertexLayout.h](../Onyx/Source/Graphics/VertexLayout.h)) gained `SNorm16`, `SNorm16x2`, `Half2` types. `VertexArray::SetLayout` ([Buffers.cpp](../Onyx/Source/Graphics/Buffers.cpp)) dispatches them to `glVertexAttribPointer` with `GL_SHORT`+`GL_TRUE` (snorm) or `GL_HALF_FLOAT`+`GL_FALSE` respectively.

`Model::ParseFromFile` (and `LoadModel`) pass `aiProcess_JoinIdenticalVertices` to Assimp — the welding step typically drops vertex count by 2–5×.

Constructors:
- `Model(path, loadTextures=true)` — synchronous Assimp parse + GPU upload.
- `Model(CpuMeshData[], directory)` — GPU upload from pre-parsed data.
- `Model(directory, MeshBoundsInfo[])` — staged-upload constructor (bounds-only).

API:
- `BuildMergedBuffers()` / `SetMergedBuffers(MergedBuffers&&)`
- `static std::vector<CpuMeshData> ParseFromFile(path, outDir, loadTexturePaths=true)` — CPU-only parse for background threads.

### `Onyx::AnimatedModel` — skinned

`SkinnedVertex` is **independent** of `MeshVertex`'s quantized layout — it stays full-precision floats (80 bytes total) because the skinned shaders consume `vec3` normal/tangent/bitangent directly:
- 0 = position (vec3, float)
- 1 = normal (vec3, float)
- 2 = texCoords (vec2, float)
- 3 = tangent (vec3, float)
- 4 = bitangent (vec3, float)
- 5 = boneIds (ivec4)
- 6 = boneWeights (vec4)

Quantizing skinned verts is future work and would require updating `skinned*.vert` in lockstep. Static `MeshVertex` and `SkinnedVertex` no longer share a layout — `SkinnedVertex::GetLayout()` builds its own.

Skinned shaders fall back to identity if `totalWeight < 0.01`.

Async pipeline (split CPU/GPU):
1. `static unique_ptr<AnimatedModel> ParseFromFile(path)` — Assimp on background thread.
2. `PreloadTextureData()` — `stbi_load` on background thread, stores `PreloadedImage` blobs.
3. `UploadGPUResources()` / `UploadTextures()` — main thread, creates GL handles.
4. `BuildMergedBuffers()` / `SetMergedBuffers(...)` — final GPU layout.

Synchronous fallback: `bool Load(path)` does everything on the calling thread.

`SkinnedVertex::GetLayout()` returns the canonical attribute layout.

### `Onyx::Animator`

Per-object, cached by GUID in `m_AnimatorCache`. Supports playback control, blending (`BlendTo`), pause/resume, speed. `SetModel()` initializes `MAX_BONES` identity matrices. `Update(dt)` advances time. `GetBoneMatrices()` returns final transforms.

## AssetManager

`Onyx/Source/Graphics/AssetManager.h`. Caches models, animated models, textures, materials by path/ID.

| Method | Purpose |
|---|---|
| `LoadModel(path, loadTextures=false)` | Sync parse + GPU upload, returns `ModelHandle` |
| `LoadAnimatedModel(path)` | Sync skinned load, returns `AnimatedModelHandle` |
| `LoadTexture(path)` / `ResolveTexture(path)` | Texture load/resolve |
| `CreateMaterial(id, name)` / `GetMaterial(id)` / `HasMaterial(id)` / `RemoveMaterial(id)` | Material registry |
| `GetDefaultAlbedo()` / `GetDefaultNormal()` | White / flat-blue fallbacks |
| `Reload(ModelHandle)` / `Reload(AnimatedModelHandle)` | Re-parse from disk (O(1) via `m_IdToPath` reverse map) |
| `RequestModelAsync(path, checkAnimated=true)` | Background parse |
| `GetModelStatus(path)` → `ModelLoadStatus` | Poll async state |
| `ProcessGPUUploads(maxPerFrame=2)` | Drains GPU upload queue. Models > 4 MB use staged multi-frame upload (`BeginStagedUpload` / `ContinueStagedUpload` / `FinalizeStagedUpload`) |

`Material { id, name, albedoPath, normalPath, rmaPath, tilingScale, normalStrength, filePath }` — the canonical material struct. Editor3D's `TerrainMaterialLibrary` delegates storage here.

## Texture slot conventions

| Consumer | Slot | Uniform | Type |
|---|---|---|---|
| Model shaders | 0 | `u_AlbedoMap` | `sampler2D` |
| Model shaders | 1 | `u_NormalMap` | `sampler2D` |
| Model shaders | 2 | `u_ShadowMap` | `sampler2DArrayShadow` |
| Terrain shader | 0 | `u_Heightmap` | `sampler2D` |
| Terrain shader | 1 | `u_Splatmap0` | `sampler2D` |
| Terrain shader | 2 | `u_Splatmap1` | `sampler2D` |
| Terrain shader | 3 | `u_DiffuseArray` | `sampler2DArray` |
| Terrain shader | 4 | `u_NormalArray` | `sampler2DArray` |
| Terrain shader | 5 | `u_RMAArray` | `sampler2DArray` |
| Terrain shader | 6 | `u_ShadowMap` | `sampler2DArrayShadow` |

## Shader inventory

### Engine shaders (`Onyx/Assets/Shaders/`)

| File | Purpose | GLSL |
|---|---|---|
| `model_batched.vert` | MDI static (TBN, batched draw data) | 4.60 |
| `skinned_batched.vert` | MDI skinned (bones via SSBO 0/1) | 4.60 |
| `model.frag` | Blinn-Phong + 4-cascade shadows + 32 point + 8 spot lights | 3.30 |
| `shadow_depth_batched.vert` | MDI shadow pass (static) | 4.60 |
| `shadow_depth_skinned_batched.vert` | MDI shadow pass (skinned) | 4.60 |
| `shadow_depth.frag` | Depth-only (empty) | 3.30 |
| `PostProcess/ssao.vert` + `ssao.frag` | SSAO compute (64-kernel, depth-reconstructed normals) | 3.30 |
| `PostProcess/ssao_blur.frag` | Bilateral blur of AO buffer | 3.30 |
| `PostProcess/ssao_composite.frag` | Composite AO over color | 3.30 |

Batched vertex shaders need GLSL 4.60 for MDI; fragments and post-process use 3.30.

### Editor3D shaders (`MMOGame/Editor3D/assets/shaders/`)

`basic3d.vert/.frag`, `billboard.vert/.frag`, `gizmo.vert/.frag`, `infinite_grid.vert/.frag` (+ `infinite_grid_test.frag`), `model.vert/.frag` (non-batched, 32 point + 8 spot lights), `model_batched.vert`, `picking.vert/.frag`, `picking_billboard.vert/.frag`, `shadow_depth.vert/.frag`, `shadow_depth_batched.vert`, `skinned.vert` (uniform bones), `terrain.vert/.frag` (PBR + Blinn-Phong toggle, 8-layer splatmap, Sobel normals, 32 point + 8 spot lights).

### Client shaders (`MMOGame/Client/assets/shaders/`)

`terrain.vert/.frag`, `entity.vert/.frag` (colored cubes), `model.vert/.frag` (MeshVertex layout, albedo + directional light, used for `.omdl` static objects).

## GPU buffers (`Onyx/Source/Graphics/Buffers.h`)

| Class | Purpose |
|---|---|
| `VertexBuffer(data, sizeBytes)` | Standard VBO; `SetData` for dynamic updates |
| `IndexBuffer(sizeBytes)` / `IndexBuffer(data, sizeBytes)` | EBO; constructor takes byte size |
| `ShaderStorageBuffer` | `GL_SHADER_STORAGE_BUFFER`, grow-or-reuse via `Upload(data, sizeBytes, bindingPoint)`; `Allocate`, `ClearUint` |
| `DrawCommandBuffer` | `GL_DRAW_INDIRECT_BUFFER`, same grow-or-reuse pattern |

## Design notes

1. **Terrain does not go through SceneRenderer.** It has its own shader, but shares light/shadow state via `UploadLightUniforms()` and `UploadShadowUniforms(shader, 6)`.
2. **Frustum** is a single shared implementation (camera, shadow cascades, terrain chunks).
3. Non-batched shaders exist only for selection wireframe and spawn-point rendering in `RenderWorldObjects()` (Editor3D viewport).
4. **MSAA** is 4× by default, resolved after the main pass. Picking framebuffer is always 1-sample.
5. SSAO lives under `Onyx/Source/Graphics/PostProcess/` (`SSAOEffect.h/.cpp`, `PostProcessStack.h/.cpp`).
