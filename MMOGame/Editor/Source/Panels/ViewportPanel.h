#pragma once

#include "EditorPanel.h"
#include "EditorContext.h"
#include "../TileMapRenderer.h"
#include <Graphics/Shader.h>
#include <Graphics/Buffers.h>
#include <memory>

namespace MMO {

// ============================================================
// VIEWPORT PANEL
// Main scene viewport with tile map rendering and input handling
// ============================================================

class ViewportPanel : public EditorPanel {
public:
    ViewportPanel();
    ~ViewportPanel() override = default;

    void OnImGuiRender(bool& isOpen) override;

    // Initialize renderers and shaders
    void Init();

    // Cache invalidation (forwarded to TileMapRenderer)
    void InvalidateTileCache();
    void InvalidateChunkAt(int32_t tileX, int32_t tileY);
    void InvalidateYSortCache();

private:
    void HandleInput();
    void RenderScene();
    void RenderGrid();
    void RenderTileCursor();
    void RenderOverlayInfo();

    // Coordinate conversions
    void ScreenToWorld(float screenX, float screenY, float& worldX, float& worldY);
    void WorldToScreen(float worldX, float worldY, float& screenX, float& screenY);
    void WorldToTile(float worldX, float worldY, int32_t& tileX, int32_t& tileY);

    // Tile operations
    void PaintTile(int32_t tileX, int32_t tileY);
    void EraseTile(int32_t tileX, int32_t tileY);
    void FillTiles(int32_t startX, int32_t startY);

private:
    // Tile map renderer
    TileMapRenderer m_TileMapRenderer;

    // Grid rendering
    std::unique_ptr<Onyx::Shader> m_GridShader;
    std::unique_ptr<Onyx::VertexArray> m_GridVAO;
    std::unique_ptr<Onyx::VertexBuffer> m_GridVBO;

    // Panning state
    bool m_IsPanning = false;
    float m_PanStartX = 0.0f;
    float m_PanStartY = 0.0f;
    float m_CameraStartX = 0.0f;
    float m_CameraStartY = 0.0f;
    float m_PanWorldStartX = 0.0f;
    float m_PanWorldStartY = 0.0f;
};

} // namespace MMO
