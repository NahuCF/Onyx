# Rendering Pipeline — From Geometry to Screen

> Tutorial-style explanation of Editor3D viewport rendering. Verified accurate as of 2026-02-22; the render-order summary in §2 omits the shadow pass (which runs *before* the main FBO bind). For the canonical engine API see [engine-rendering.md](engine-rendering.md).

This document explains how the Editor3D viewport renders a frame, how framebuffer attachments work, why MSAA resolve is necessary, and how the SSAO post-processing chain reads data from the main pass.

---

## 1. Framebuffer Setup

When the viewport creates its framebuffer with `m_Framebuffer->Create(width, height, 4)`, the GPU allocates:

```
m_FrameBufferID (FBO)
├── m_ColorBufferID   ← MSAA texture (4 samples per pixel, RGBA)
└── m_DepthBufferID   ← MSAA renderbuffer (depth+stencil)
```

These are empty GPU memory — blank canvases waiting to be drawn into.

**Why 4 samples?** MSAA (multisample anti-aliasing) stores 4 color/depth values per pixel at slightly different sub-pixel positions. When collapsed into one value, edges appear smooth instead of jagged.

---

## 2. Main Render Pass

```cpp
m_Framebuffer->Bind();   // redirect all rendering INTO this FBO
```

After `Bind()`, every draw call writes into the FBO's attachments instead of the screen. For every triangle the GPU rasterizes, it writes:
- **Color** (RGBA) → `m_ColorBufferID`
- **Depth** (float, 0.0–1.0) → `m_DepthBufferID`

Both happen simultaneously and automatically per fragment. You don't choose — the GPU always writes both when a fragment passes the depth test.

```cpp
RenderGrid();                          // grid lines → color + depth
RenderTerrain();                       // terrain triangles → color + depth
m_SceneRenderer->RenderBatches();      // all static/skinned models → color + depth
RenderWorldObjects();                  // cubes, outlines, etc. → color + depth
RenderGizmoIcons();
RenderGizmo();

m_Framebuffer->UnBind();
```

After this, the FBO contains a complete scene:
- **Color attachment**: what everything looks like (RGB per pixel, 4 MSAA samples each)
- **Depth attachment**: how far each pixel is from the camera (one float per pixel, 4 MSAA samples each)

---

## 3. MSAA Resolve — Why It's Necessary

```cpp
m_Framebuffer->Resolve();
```

### The problem

The color attachment is a `GL_TEXTURE_2D_MULTISAMPLE` with 4 samples per pixel. You **cannot**:
- Display it with `ImGui::Image()` — ImGui expects a regular `GL_TEXTURE_2D`
- Sample it in a shader with `texture(sampler2D, uv)` — that only works on regular textures
- Use it as input for any post-processing pass

A multisampled texture is a special GPU format. Each pixel stores 4 independent color values at sub-pixel offsets. A normal texture lookup expects 1 value per pixel.

### The solution: resolve

`Resolve()` performs a blit (GPU-to-GPU copy) that averages the 4 samples into 1:

```
m_ColorBufferID (GL_TEXTURE_2D_MULTISAMPLE, 4 samples per pixel)
    │
    │ glBlitFramebuffer(... GL_COLOR_BUFFER_BIT, GL_LINEAR)
    │ (averages 4 sub-pixel samples → 1 final color)
    │
    ▼
m_ResolveColorBufferID (GL_TEXTURE_2D, 1 sample per pixel)
```

Internally in `Framebuffer::Resolve()`:
```cpp
glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FrameBufferID);        // source: MSAA
glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveFrameBufferID); // dest: regular
glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
```

- `GL_READ_FRAMEBUFFER` — read from this FBO
- `GL_DRAW_FRAMEBUFFER` — write to this FBO
- `GL_LINEAR` — average the samples (this is what produces the anti-aliased result)

After resolve, `GetColorBufferID()` returns the single-sample texture that anything can read.

### What if MSAA is off?

With `samples = 1`, the color attachment is already a regular `GL_TEXTURE_2D`. `Resolve()` still blits (it's a no-op copy), but the format is already sampleable. The resolve step is always safe to call.

---

## 4. Depth Blit — Making Depth Sampleable

```cpp
BlitDepthToTexture();
```

### The problem

The depth attachment (`m_DepthBufferID`) is a **renderbuffer**, not a texture. Renderbuffers are optimized for the GPU to write to during rendering, but shaders cannot read them. There is no way to bind a renderbuffer to a texture slot.

### The solution: blit depth into a RenderTarget

```
m_FrameBufferID's depth renderbuffer (MSAA, not sampleable by shaders)
    │
    │ RenderCommand::BlitDepth() — glBlitFramebuffer(... GL_DEPTH_BUFFER_BIT ...)
    │
    ▼
m_SSAODepthTarget (GL_TEXTURE_2D, GL_DEPTH_COMPONENT24, sampleable)
```

This copies the depth values into a regular depth texture that the SSAO shader can sample with `texture(u_DepthTexture, uv)`.

---

## 5. SSAO Post-Processing Chain

After resolve and depth blit, we have two sampleable textures from the main pass:

| Texture | Contains | Source |
|---------|----------|--------|
| `GetColorBufferID()` | Scene colors (RGB) | MSAA resolve |
| `m_SSAODepthTarget` | Per-pixel distance from camera | Depth blit |

The SSAO chain uses 4 render targets and 3 fullscreen quad passes:

### Pass 1: SSAO Occlusion
```
Input:  m_SSAODepthTarget (depth), m_SSAONoiseTexture (random noise)
Output: m_SSAOTarget (R8 — single channel, 0.0 = occluded, 1.0 = open)
```
The shader reconstructs 3D positions from depth, samples nearby points in a hemisphere, and checks how many are hidden behind geometry. Dark corners and crevices get low values.

### Pass 2: Blur
```
Input:  m_SSAOTarget (raw occlusion), m_SSAODepthTarget (depth for edge-aware blur)
Output: m_SSAOBlurTarget (R8 — smoothed occlusion)
```
Smooths the noisy SSAO result while preserving edges using depth-aware filtering.

### Pass 3: Composite
```
Input:  GetColorBufferID() (scene color), m_SSAOBlurTarget (blurred occlusion)
Output: m_SSAOCompositeTarget (RGBA8 — final image)
```
Multiplies scene color by the occlusion factor: `finalColor = sceneColor * occlusion`. Occluded areas get darkened.

### Data flow diagram

```
         ┌─────────────────────────────┐
         │       MAIN RENDER PASS      │
         │  (all geometry drawn here)  │
         └─────────┬───────────────────┘
                   │
          ┌────────┴────────┐
          ▼                 ▼
    color (MSAA)      depth (renderbuffer)
          │                 │
          │ Resolve()       │ BlitDepth()
          ▼                 ▼
    color texture      depth texture
          │                 │
          │        ┌────────┤
          │        ▼        │
          │   ┌─────────┐   │
          │   │  SSAO    │◄──┘  reads depth + noise
          │   │  Pass    │      writes occlusion
          │   └────┬────┘
          │        ▼
          │   ┌─────────┐
          │   │  Blur    │◄──── reads occlusion + depth
          │   │  Pass    │      writes smoothed occlusion
          │   └────┬────┘
          │        │
          ▼        ▼
     ┌──────────────────┐
     │   Composite      │  reads color + blurred occlusion
     │   Pass           │  writes final image
     └────────┬─────────┘
              ▼
        ImGui::Image()
```

---

## 6. Display

```cpp
uint32_t displayTexture = (m_EnableSSAO && m_SSAOCompositeTarget.GetTextureID())
    ? m_SSAOCompositeTarget.GetTextureID()
    : m_Framebuffer->GetColorBufferID();

ImGui::Image(static_cast<ImTextureID>(displayTexture), ...);
```

- **SSAO on**: display the composite result (color × occlusion)
- **SSAO off**: display the resolved color buffer directly

Either way, it's just a texture ID handed to ImGui.

---

## Key Concepts Summary

| Concept | What it is | Why it matters |
|---------|-----------|----------------|
| **FBO** | A container that holds attachments (textures/renderbuffers) | Lets you render to textures instead of the screen |
| **Color attachment** | Where fragment color is written during draw calls | Contains the visual result of rendering |
| **Depth attachment** | Where fragment depth is written during draw calls | Records distance per pixel; used for depth testing and post-effects |
| **MSAA** | Multiple samples per pixel for anti-aliasing | Smooth edges, but must be resolved before reading |
| **Resolve** | Averaging MSAA samples into a regular texture | Makes the color buffer readable by shaders and ImGui |
| **Depth blit** | Copying depth from renderbuffer to texture | Makes depth readable by shaders (renderbuffers aren't sampleable) |
| **Render target** | Lightweight FBO + texture for post-processing | One pass writes to it, next pass reads from it |
| **Fullscreen quad** | 2 triangles covering the screen | Runs a fragment shader on every pixel for post-processing |
