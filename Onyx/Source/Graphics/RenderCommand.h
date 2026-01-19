#pragma once

#include <cstdint>

namespace Onyx {

class VertexArray;

// Static facade for low-level rendering commands
// Abstracts OpenGL calls behind a clean API
// Note: Stats tracking is handled by higher-level renderers (Renderer2D, etc.)
class RenderCommand {
public:
    // Drawing
    static void DrawIndexed(const VertexArray& vao, uint32_t indexCount);
    static void DrawArrays(const VertexArray& vao, uint32_t vertexCount);

    // State
    static void Clear();
    static void SetClearColor(float r, float g, float b, float a);
    static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    // Blending
    static void EnableBlending();
    static void DisableBlending();

    // Depth testing
    static void EnableDepthTest();
    static void DisableDepthTest();

    // Polygon mode (wireframe)
    static void SetWireframeMode(bool enabled);

    // Line rendering
    static void SetLineWidth(float width);

    // Draw modes
    static void DrawLines(const VertexArray& vao, uint32_t vertexCount);

    // Reset state (unbind VAO, etc.)
    static void ResetState();
};

}
