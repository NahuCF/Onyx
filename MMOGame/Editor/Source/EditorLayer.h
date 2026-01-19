#pragma once

#include "EditorDefines.h"
#include "TileMap.h"
#include "FileDialog.h"
#include "Panels/PanelManager.h"
#include "Panels/ViewportPanel.h"
#include <Core/Layer.h>
#include <Graphics/Renderer2D.h>
#include <Graphics/Framebuffer.h>
#include <memory>
#include <string>

namespace MMO {

// ============================================================
// EDITOR LAYER
// Main editor interface - coordinates panels and file operations
// ============================================================

class EditorLayer : public Onyx::Layer {
public:
    EditorLayer();
    ~EditorLayer() override = default;

    void OnAttach() override;
    void OnUpdate() override;
    void OnImGui() override;

private:
    // UI setup
    void SetupDockspace();
    void RenderMenuBar();
    void RenderTileSizeDialog();

    // Map operations
    void NewMap();
    void OpenMap();
    void SaveMap();
    void SaveMapAs();
    void PrepareMapMetadataForSave();
    void LoadTilesetsFromMetadata();

    // Tileset operations
    void LoadTileset();

    // Callbacks for EditorContext
    void OnCacheInvalidated();
    void OnTilePainted(int32_t tileX, int32_t tileY);

private:
    // Core systems
    std::unique_ptr<Onyx::Renderer2D> m_Renderer;
    std::unique_ptr<Onyx::Framebuffer> m_Framebuffer;

    // Map data
    TileMap m_TileMap;
    std::string m_CurrentMapPath;

    // Panel system
    PanelManager m_PanelManager;
    ViewportPanel* m_ViewportPanel = nullptr;  // Cached for cache invalidation calls

    // Tileset loading dialog
    bool m_ShowTileSizeDialog = false;
    std::string m_PendingTilesetPath;
    int m_TileSizeInput[2] = { 32, 32 };

    // File browser state
    enum class FileBrowserPurpose { NONE, LOAD_TILESET, OPEN_MAP, SAVE_MAP };
    FileBrowserPurpose m_FileBrowserPurpose = FileBrowserPurpose::NONE;

    // FPS logging
    float m_FpsLogTimer = 0.0f;
};

} // namespace MMO
