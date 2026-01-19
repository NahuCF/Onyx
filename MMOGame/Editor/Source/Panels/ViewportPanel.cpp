#include "ViewportPanel.h"
#include "../Tileset.h"
#include <Graphics/RenderCommand.h>
#include <Graphics/VertexLayout.h>
#include <imgui.h>
#include <cmath>
#include <algorithm>

namespace MMO {

ViewportPanel::ViewportPanel() {
    m_ID = "viewport";
    m_Name = "Viewport";
}

void ViewportPanel::Init() {
    auto& ctx = EditorContext::Get();

    // Initialize tile map renderer
    m_TileMapRenderer.Init();
    if (ctx.tileMap) {
        m_TileMapRenderer.SetTileMap(ctx.tileMap);
    }

    // Initialize grid shader
    m_GridShader = std::make_unique<Onyx::Shader>(
        "MMOGame/assets/shaders/grid.vert",
        "MMOGame/assets/shaders/grid.frag"
    );

    // Create fullscreen quad VAO/VBO for grid
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    m_GridVBO = std::make_unique<Onyx::VertexBuffer>(quadVertices, sizeof(quadVertices));
    m_GridVAO = std::make_unique<Onyx::VertexArray>();

    Onyx::VertexLayout layout;
    layout.PushFloat(2);  // a_Position

    m_GridVAO->SetVertexBuffer(m_GridVBO.get());
    m_GridVAO->SetLayout(layout);
}

void ViewportPanel::InvalidateTileCache() {
    m_TileMapRenderer.InvalidateAll();
}

void ViewportPanel::InvalidateChunkAt(int32_t tileX, int32_t tileY) {
    m_TileMapRenderer.InvalidateChunkAt(tileX, tileY);
}

void ViewportPanel::InvalidateYSortCache() {
    m_TileMapRenderer.InvalidateYSortCache();
}

void ViewportPanel::OnImGuiRender(bool& isOpen) {
    auto& ctx = EditorContext::Get();

    ImGuiWindowFlags vpFlags = ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", &isOpen, vpFlags);

    ctx.viewportFocused = ImGui::IsWindowFocused();
    ctx.viewportHovered = ImGui::IsWindowHovered();

    ImVec2 viewportPos = ImGui::GetCursorScreenPos();
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    ctx.viewportX = viewportPos.x;
    ctx.viewportY = viewportPos.y;

    int newWidth = std::max(1, static_cast<int>(viewportSize.x));
    int newHeight = std::max(1, static_cast<int>(viewportSize.y));
    if (newWidth != static_cast<int>(ctx.viewportWidth) || newHeight != static_cast<int>(ctx.viewportHeight)) {
        ctx.viewportWidth = static_cast<float>(newWidth);
        ctx.viewportHeight = static_cast<float>(newHeight);
        if (ctx.framebuffer) {
            ctx.framebuffer->Resize(newWidth, newHeight);
        }
    }

    if (ctx.viewportHovered) {
        HandleInput();
    }

    if (ctx.mapLoaded) {
        RenderScene();
    }

    // Display framebuffer
    if (ctx.framebuffer) {
        ImGui::Image(
            static_cast<ImTextureID>(ctx.framebuffer->GetColorBufferID()),
            ImVec2(ctx.viewportWidth, ctx.viewportHeight),
            ImVec2(0, 1), ImVec2(1, 0)
        );
    }

    if (ctx.mapLoaded) {
        RenderTileCursor();
    }

    RenderOverlayInfo();

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::HandleInput() {
    auto& ctx = EditorContext::Get();
    ImGuiIO& io = ImGui::GetIO();

    ctx.mouseX = io.MousePos.x;
    ctx.mouseY = io.MousePos.y;

    // Convert screen position to world position
    float worldX, worldY;
    ScreenToWorld(ctx.mouseX, ctx.mouseY, worldX, worldY);

    // Convert to tile coordinates
    WorldToTile(worldX, worldY, ctx.hoveredTileX, ctx.hoveredTileY);

    // Scroll wheel: Ctrl+Wheel adjusts fill radius, otherwise zoom
    if (io.MouseWheel != 0.0f) {
        if (io.KeyCtrl && ctx.currentTool == EditorTool::FILL) {
            // Adjust fill radius
            int32_t delta = io.MouseWheel > 0 ? 1 : -1;
            ctx.fillRadius = std::clamp(ctx.fillRadius + delta, 1, 10);
        } else {
            // Zoom
            float zoomDelta = io.MouseWheel * 0.1f;
            ctx.cameraZoom = std::clamp(ctx.cameraZoom + zoomDelta, 0.25f, 4.0f);
        }
    }

    // Pan with middle mouse button
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        m_IsPanning = true;
        m_PanStartX = ctx.mouseX;
        m_PanStartY = ctx.mouseY;
        m_CameraStartX = ctx.cameraX;
        m_CameraStartY = ctx.cameraY;
        // Store the world position under the mouse at start of pan
        ScreenToWorld(ctx.mouseX, ctx.mouseY, m_PanWorldStartX, m_PanWorldStartY);
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
        m_IsPanning = false;
    }
    if (m_IsPanning && ctx.tileMap) {
        // Get current world position under mouse (with original camera)
        float tempCamX = ctx.cameraX;
        float tempCamY = ctx.cameraY;
        ctx.cameraX = m_CameraStartX;
        ctx.cameraY = m_CameraStartY;

        float currentWorldX, currentWorldY;
        ScreenToWorld(ctx.mouseX, ctx.mouseY, currentWorldX, currentWorldY);

        // Move camera so the point under mouse stays fixed
        ctx.cameraX = m_CameraStartX - (currentWorldX - m_PanWorldStartX);
        ctx.cameraY = m_CameraStartY - (currentWorldY - m_PanWorldStartY);
    }

    // Paint with left mouse button (only if tileset loaded for brush/fill)
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !m_IsPanning) {
        bool hasTileset = TilesetManager::Instance().GetTilesetCount() > 0;

        if (ctx.currentTool == EditorTool::BRUSH && hasTileset) {
            PaintTile(ctx.hoveredTileX, ctx.hoveredTileY);
        } else if (ctx.currentTool == EditorTool::ERASER) {
            EraseTile(ctx.hoveredTileX, ctx.hoveredTileY);
        } else if (ctx.currentTool == EditorTool::FILL && hasTileset) {
            FillTiles(ctx.hoveredTileX, ctx.hoveredTileY);
        }
    }

    // Keyboard shortcuts
    if (ctx.viewportFocused && ctx.tileMap) {
        // Layer shortcuts (1-9 for first 9 layers)
        const auto& layers = ctx.tileMap->GetMetadata().layers;
        for (size_t i = 0; i < layers.size() && i < 9; ++i) {
            if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i))) {
                ctx.selectedLayerId = layers[i].id;
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_B)) ctx.currentTool = EditorTool::BRUSH;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) ctx.currentTool = EditorTool::ERASER;
        if (ImGui::IsKeyPressed(ImGuiKey_G)) ctx.currentTool = EditorTool::FILL;
    }
}

void ViewportPanel::RenderScene() {
    auto& ctx = EditorContext::Get();
    if (!ctx.framebuffer || !ctx.tileMap) return;

    ctx.framebuffer->Bind();
    ctx.framebuffer->Clear(40.0f/255.0f, 44.0f/255.0f, 52.0f/255.0f, 1.0f);
    ctx.framebuffer->DisableDepthTest();
    ctx.framebuffer->EnableBlending();

    // Ensure tile map renderer has correct tile map
    m_TileMapRenderer.SetTileMap(ctx.tileMap);

    // Render tiles
    m_TileMapRenderer.Render(ctx.cameraX, ctx.cameraY, ctx.cameraZoom,
                             ctx.viewportWidth, ctx.viewportHeight);

    if (ctx.showGrid) {
        RenderGrid();
    }

    ctx.framebuffer->UnBind();

    // Resolve multisampled framebuffer to display texture
    ctx.framebuffer->Resolve();
}

void ViewportPanel::RenderGrid() {
    auto& ctx = EditorContext::Get();
    if (!m_GridShader || !m_GridVAO || !ctx.tileMap) return;

    const auto& metadata = ctx.tileMap->GetMetadata();

    m_GridShader->Bind();

    m_GridShader->SetVec2("u_ViewportSize", ctx.viewportWidth, ctx.viewportHeight);
    m_GridShader->SetVec2("u_CameraPos", ctx.cameraX, ctx.cameraY);
    m_GridShader->SetFloat("u_Zoom", ctx.cameraZoom);
    m_GridShader->SetVec2("u_TileSize", static_cast<float>(metadata.tileWidth), static_cast<float>(metadata.tileHeight));
    m_GridShader->SetFloat("u_ChunkSize", static_cast<float>(CHUNK_SIZE));
    m_GridShader->SetVec4("u_GridColor", 0.6f, 0.6f, 0.6f, 0.5f);
    m_GridShader->SetVec4("u_ChunkColor", 0.8f, 0.8f, 0.4f, 0.5f);
    m_GridShader->SetFloat("u_LineWidth", 1.5f);

    Onyx::RenderCommand::DrawArrays(*m_GridVAO, 6);

    // Unbind VAO and shader to leave clean GL state
    m_GridVAO->UnBind();
    m_GridShader->UnBind();
}

void ViewportPanel::RenderTileCursor() {
    auto& ctx = EditorContext::Get();
    if (!ctx.viewportHovered || !ctx.tileMap) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const auto& metadata = ctx.tileMap->GetMetadata();

    float tileW = static_cast<float>(metadata.tileWidth) * ctx.cameraZoom;
    float tileH = static_cast<float>(metadata.tileHeight) * ctx.cameraZoom;
    float halfW = tileW / 2.0f;
    float halfH = tileH / 2.0f;

    ImU32 cursorColor;
    switch (ctx.currentTool) {
        case EditorTool::BRUSH:
        case EditorTool::FILL:
            cursorColor = IM_COL32(0, 255, 0, 150);
            break;
        case EditorTool::ERASER:
            cursorColor = IM_COL32(255, 0, 0, 150);
            break;
        default:
            cursorColor = IM_COL32(255, 255, 255, 150);
            break;
    }

    // Helper to draw a single tile outline
    auto drawTileOutline = [&](int32_t tileX, int32_t tileY) {
        float screenX, screenY;
        WorldToScreen(static_cast<float>(tileX) + 0.5f,
                      static_cast<float>(tileY) + 0.5f,
                      screenX, screenY);

        ImVec2 points[4] = {
            ImVec2(screenX - halfW, screenY),
            ImVec2(screenX, screenY - halfH),
            ImVec2(screenX + halfW, screenY),
            ImVec2(screenX, screenY + halfH)
        };
        drawList->AddPolyline(points, 4, cursorColor, ImDrawFlags_Closed, 2.0f);
    };

    // Draw cursor based on tool
    if (ctx.currentTool == EditorTool::FILL) {
        // Fill tool shows outer border of the area
        int32_t radius = ctx.fillRadius;
        int32_t cx = ctx.hoveredTileX;
        int32_t cy = ctx.hoveredTileY;

        // Get screen positions of the 4 corner tiles of the fill area
        float topX, topY, bottomX, bottomY, leftX, leftY, rightX, rightY;
        WorldToScreen(static_cast<float>(cx - radius) + 0.5f, static_cast<float>(cy - radius) + 0.5f, topX, topY);
        WorldToScreen(static_cast<float>(cx + radius) + 0.5f, static_cast<float>(cy + radius) + 0.5f, bottomX, bottomY);
        WorldToScreen(static_cast<float>(cx - radius) + 0.5f, static_cast<float>(cy + radius) + 0.5f, leftX, leftY);
        WorldToScreen(static_cast<float>(cx + radius) + 0.5f, static_cast<float>(cy - radius) + 0.5f, rightX, rightY);

        // Draw outer diamond border (using outer edges of corner tiles)
        ImVec2 outerPoints[4] = {
            ImVec2(topX, topY - halfH),      // Top vertex of top-left corner tile
            ImVec2(rightX + halfW, rightY),  // Right vertex of top-right corner tile
            ImVec2(bottomX, bottomY + halfH),// Bottom vertex of bottom-right corner tile
            ImVec2(leftX - halfW, leftY)     // Left vertex of bottom-left corner tile
        };
        drawList->AddPolyline(outerPoints, 4, cursorColor, ImDrawFlags_Closed, 2.0f);
    } else {
        // Other tools show single tile
        drawTileOutline(ctx.hoveredTileX, ctx.hoveredTileY);
    }

    // Show selected tile preview if brush tool
    if (ctx.currentTool == EditorTool::BRUSH && !ctx.selectedTile.IsEmpty()) {
        Tileset* tileset = TilesetManager::Instance().GetTileset(ctx.selectedTile.tilesetIndex);
        if (tileset && tileset->GetTexture()) {
            float screenX, screenY;
            WorldToScreen(static_cast<float>(ctx.hoveredTileX) + 0.5f,
                          static_cast<float>(ctx.hoveredTileY) + 0.5f,
                          screenX, screenY);

            float u, v;
            tileset->GetTileUV(ctx.selectedTile.tileIndex, u, v);

            ImTextureID texId = static_cast<ImTextureID>(tileset->GetTexture()->GetTextureID());

            float texWidth = static_cast<float>(tileset->GetTextureWidth());
            float texHeight = static_cast<float>(tileset->GetTextureHeight());
            float srcTileW = static_cast<float>(tileset->GetTileWidth());
            float srcTileH = static_cast<float>(tileset->GetTileHeight());

            ImVec2 uvMin(u / texWidth, v / texHeight);
            ImVec2 uvMax((u + srcTileW) / texWidth, (v + srcTileH) / texHeight);

            drawList->AddImage(
                texId,
                ImVec2(screenX - halfW, screenY - halfH),
                ImVec2(screenX + halfW, screenY + halfH),
                uvMin, uvMax,
                IM_COL32(255, 255, 255, 150)
            );
        }
    }
}

void ViewportPanel::RenderOverlayInfo() {
    auto& ctx = EditorContext::Get();

    ImGui::SetCursorPos(ImVec2(10, 30));
    ImGui::Text("Camera: (%.0f, %.0f) Zoom: %.2f", ctx.cameraX, ctx.cameraY, ctx.cameraZoom);
    ImGui::Text("Tile: (%d, %d)", ctx.hoveredTileX, ctx.hoveredTileY);

    if (ctx.tileMap) {
        const LayerDefinition* currentLayer = ctx.tileMap->GetMetadata().GetLayer(ctx.selectedLayerId);
        ImGui::Text("Layer: %s", currentLayer ? currentLayer->name.c_str() : "None");
    }
}

void ViewportPanel::ScreenToWorld(float screenX, float screenY, float& worldX, float& worldY) {
    auto& ctx = EditorContext::Get();
    if (!ctx.tileMap) {
        worldX = worldY = 0.0f;
        return;
    }

    float relX = (screenX - ctx.viewportX - ctx.viewportWidth / 2.0f) / ctx.cameraZoom;
    float relY = (screenY - ctx.viewportY - ctx.viewportHeight / 2.0f) / ctx.cameraZoom;

    const auto& meta = ctx.tileMap->GetMetadata();
    float halfTileW = static_cast<float>(meta.tileWidth) / 2.0f;
    float halfTileH = static_cast<float>(meta.tileHeight) / 2.0f;

    worldX = (relX / halfTileW + relY / halfTileH) / 2.0f + ctx.cameraX;
    worldY = (relY / halfTileH - relX / halfTileW) / 2.0f + ctx.cameraY;
}

void ViewportPanel::WorldToScreen(float worldX, float worldY, float& screenX, float& screenY) {
    auto& ctx = EditorContext::Get();
    if (!ctx.tileMap) {
        screenX = screenY = 0.0f;
        return;
    }

    const auto& meta = ctx.tileMap->GetMetadata();
    float halfTileW = static_cast<float>(meta.tileWidth) / 2.0f;
    float halfTileH = static_cast<float>(meta.tileHeight) / 2.0f;

    float relX = worldX - ctx.cameraX;
    float relY = worldY - ctx.cameraY;

    float isoX = (relX - relY) * halfTileW;
    float isoY = (relX + relY) * halfTileH;

    screenX = ctx.viewportX + ctx.viewportWidth / 2.0f + isoX * ctx.cameraZoom;
    screenY = ctx.viewportY + ctx.viewportHeight / 2.0f + isoY * ctx.cameraZoom;
}

void ViewportPanel::WorldToTile(float worldX, float worldY, int32_t& tileX, int32_t& tileY) {
    tileX = static_cast<int32_t>(std::floor(worldX));
    tileY = static_cast<int32_t>(std::floor(worldY));
}

void ViewportPanel::PaintTile(int32_t tileX, int32_t tileY) {
    auto& ctx = EditorContext::Get();
    if (!ctx.tileMap || ctx.selectedTile.IsEmpty()) return;

    ctx.tileMap->SetTile(tileX, tileY, ctx.selectedLayerId, ctx.selectedTile);
    InvalidateChunkAt(tileX, tileY);

    if (ctx.onTilePainted) {
        ctx.onTilePainted(tileX, tileY);
    }
}

void ViewportPanel::EraseTile(int32_t tileX, int32_t tileY) {
    auto& ctx = EditorContext::Get();
    if (!ctx.tileMap) return;

    ctx.tileMap->SetTile(tileX, tileY, ctx.selectedLayerId, EMPTY_TILE);
    InvalidateChunkAt(tileX, tileY);

    if (ctx.onTilePainted) {
        ctx.onTilePainted(tileX, tileY);
    }
}

void ViewportPanel::FillTiles(int32_t startX, int32_t startY) {
    auto& ctx = EditorContext::Get();
    if (!ctx.tileMap || ctx.selectedTile.IsEmpty()) return;

    int32_t radius = ctx.fillRadius;

    for (int32_t dy = -radius; dy <= radius; ++dy) {
        for (int32_t dx = -radius; dx <= radius; ++dx) {
            ctx.tileMap->SetTile(startX + dx, startY + dy, ctx.selectedLayerId, ctx.selectedTile);
        }
    }

    // Invalidate all affected chunks
    int32_t minTileX = startX - radius;
    int32_t maxTileX = startX + radius;
    int32_t minTileY = startY - radius;
    int32_t maxTileY = startY + radius;

    int32_t minChunkX = static_cast<int32_t>(std::floor(static_cast<float>(minTileX) / CHUNK_SIZE));
    int32_t maxChunkX = static_cast<int32_t>(std::floor(static_cast<float>(maxTileX) / CHUNK_SIZE));
    int32_t minChunkY = static_cast<int32_t>(std::floor(static_cast<float>(minTileY) / CHUNK_SIZE));
    int32_t maxChunkY = static_cast<int32_t>(std::floor(static_cast<float>(maxTileY) / CHUNK_SIZE));

    for (int32_t chunkY = minChunkY; chunkY <= maxChunkY; ++chunkY) {
        for (int32_t chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
            m_TileMapRenderer.InvalidateChunkAt(chunkX * CHUNK_SIZE, chunkY * CHUNK_SIZE);
        }
    }
    m_TileMapRenderer.InvalidateYSortCache();
}

} // namespace MMO
