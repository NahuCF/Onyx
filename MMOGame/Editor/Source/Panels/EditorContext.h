#pragma once

#include "../EditorDefines.h"
#include "../TileMap.h"
#include <Graphics/Camera.h>
#include <Graphics/Renderer2D.h>
#include <Graphics/Framebuffer.h>
#include <memory>
#include <functional>

namespace MMO {

// ============================================================
// EDITOR CONTEXT
// Shared state accessible by all panels
// ============================================================

class EditorContext {
public:
    static EditorContext& Get() {
        static EditorContext instance;
        return instance;
    }

    // Prevent copying
    EditorContext(const EditorContext&) = delete;
    EditorContext& operator=(const EditorContext&) = delete;

    // ==================== TILE SELECTION ====================
    TileId selectedTile = EMPTY_TILE;
    uint16_t selectedTilesetIndex = 0;
    uint16_t selectedLayerId = 0;
    EditorTool currentTool = EditorTool::BRUSH;
    int32_t fillRadius = 3;  // Fill tool radius (Ctrl+Wheel to adjust)

    bool HasTileSelected() const { return selectedTile != EMPTY_TILE; }

    // ==================== CAMERA STATE ====================
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float cameraZoom = 1.0f;

    // ==================== VIEWPORT STATE ====================
    bool viewportHovered = false;
    bool viewportFocused = false;
    float viewportX = 0.0f;
    float viewportY = 0.0f;
    float viewportWidth = 800.0f;
    float viewportHeight = 600.0f;

    // ==================== MOUSE STATE ====================
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    int32_t hoveredTileX = 0;
    int32_t hoveredTileY = 0;

    // ==================== REFERENCES (set by EditorLayer) ====================
    TileMap* tileMap = nullptr;
    Onyx::Camera* camera = nullptr;
    Onyx::Renderer2D* renderer = nullptr;
    Onyx::Framebuffer* framebuffer = nullptr;

    // ==================== MAP STATE ====================
    std::string currentMapPath;
    bool mapLoaded = false;
    bool mapDirty = false;  // Has unsaved changes

    // ==================== DISPLAY OPTIONS ====================
    bool showGrid = true;
    bool showCollision = false;

    // ==================== CALLBACKS ====================
    // Called when tile selection changes
    std::function<void()> onSelectionChanged;
    // Called when a tile is painted (for cache invalidation)
    std::function<void(int32_t tileX, int32_t tileY)> onTilePainted;
    // Called when map needs full cache rebuild
    std::function<void()> onCacheInvalidated;

private:
    EditorContext() = default;
};

} // namespace MMO
