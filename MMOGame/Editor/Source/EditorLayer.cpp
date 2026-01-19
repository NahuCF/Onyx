#include "EditorLayer.h"
#include "Panels/EditorContext.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/PropertiesPanel.h"
#include "Panels/TilesetPanel.h"
#include "Panels/AssetBrowserPanel.h"
#include "Panels/StatisticsPanel.h"
#include "Tileset.h"
#include <Core/Application.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <algorithm>

namespace MMO {

EditorLayer::EditorLayer() = default;

void EditorLayer::OnAttach() {
    // Initialize file dialog system
    FileDialog::Init();

    // Initialize Renderer2D
    auto& window = *Onyx::Application::GetInstance().GetWindow();
    window.SetVSync(false);
    m_Renderer = std::make_unique<Onyx::Renderer2D>(window, nullptr);

    // Initialize viewport framebuffer
    m_Framebuffer = std::make_unique<Onyx::Framebuffer>();
    m_Framebuffer->Resize(800, 600);

    // Create a default empty map
    MapMetadata meta;
    meta.name = "Untitled Map";
    meta.id = 1;
    m_TileMap.Create(meta);

    // Setup EditorContext with references
    auto& ctx = EditorContext::Get();
    ctx.tileMap = &m_TileMap;
    ctx.renderer = m_Renderer.get();
    ctx.framebuffer = m_Framebuffer.get();
    ctx.mapLoaded = true;
    ctx.selectedLayerId = m_TileMap.GetMetadata().layers.empty() ? 0 : m_TileMap.GetMetadata().layers[0].id;

    // Setup callbacks
    ctx.onCacheInvalidated = [this]() { OnCacheInvalidated(); };
    ctx.onTilePainted = [this](int32_t x, int32_t y) { OnTilePainted(x, y); };

    // Register panels
    m_PanelManager.AddPanel<HierarchyPanel>("hierarchy", "Hierarchy", true);
    m_PanelManager.AddPanel<PropertiesPanel>("properties", "Properties", true);
    m_PanelManager.AddPanel<TilesetPanel>("tileset", "Tileset", true);
    m_PanelManager.AddPanel<AssetBrowserPanel>("asset_browser", "Asset Browser", true);
    m_PanelManager.AddPanel<StatisticsPanel>("statistics", "Statistics", false);

    // Viewport panel needs special initialization
    m_ViewportPanel = m_PanelManager.AddPanel<ViewportPanel>("viewport", "Viewport", true);
    m_ViewportPanel->Init();
}

void EditorLayer::OnUpdate() {
    auto& ctx = EditorContext::Get();

    // Log FPS every 2 seconds
    m_FpsLogTimer += 1.0f / 60.0f;
    if (m_FpsLogTimer >= 2.0f) {
        int fps = Onyx::Application::GetInstance().GetWindow()->GetFramerate();
        std::cout << "FPS: " << fps << std::endl;
        m_FpsLogTimer = 0.0f;
    }

    // Update loaded chunks based on camera position
    if (ctx.mapLoaded) {
        m_TileMap.UpdateLoadedChunks(ctx.cameraX, ctx.cameraY, 3);
    }
}

void EditorLayer::OnImGui() {
    SetupDockspace();

    // Render file browser if open
    auto& browser = FileDialog::GetBrowser();
    if (browser.Render()) {
        std::string selectedPath = browser.GetSelectedPath();

        switch (m_FileBrowserPurpose) {
            case FileBrowserPurpose::LOAD_TILESET:
                m_PendingTilesetPath = selectedPath;
                m_ShowTileSizeDialog = true;
                break;

            case FileBrowserPurpose::OPEN_MAP: {
                auto& ctx = EditorContext::Get();
                if (m_TileMap.Load(selectedPath)) {
                    m_CurrentMapPath = selectedPath;
                    ctx.mapLoaded = true;
                    ctx.currentMapPath = selectedPath;

                    // Restore camera position from metadata
                    const auto& meta = m_TileMap.GetMetadata();
                    ctx.cameraX = meta.cameraX;
                    ctx.cameraY = meta.cameraY;
                    ctx.cameraZoom = meta.cameraZoom;

                    // Auto-load tilesets
                    LoadTilesetsFromMetadata();
                    OnCacheInvalidated();

                    std::cout << "Loaded map from: " << m_CurrentMapPath << std::endl;
                }
                break;
            }

            case FileBrowserPurpose::SAVE_MAP:
                m_CurrentMapPath = selectedPath;
                EditorContext::Get().currentMapPath = selectedPath;
                PrepareMapMetadataForSave();
                m_TileMap.Save(m_CurrentMapPath);
                std::cout << "Saved map to: " << m_CurrentMapPath << std::endl;
                break;

            default:
                break;
        }

        m_FileBrowserPurpose = FileBrowserPurpose::NONE;
    }

    RenderTileSizeDialog();
}

void EditorLayer::RenderTileSizeDialog() {
    if (m_ShowTileSizeDialog) {
        ImGui::OpenPopup("Tile Size");
    }

    if (ImGui::BeginPopupModal("Tile Size", &m_ShowTileSizeDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter tile dimensions for the tileset:");
        ImGui::Separator();

        ImGui::InputInt("Tile Width", &m_TileSizeInput[0]);
        ImGui::InputInt("Tile Height", &m_TileSizeInput[1]);

        m_TileSizeInput[0] = std::max(8, std::min(256, m_TileSizeInput[0]));
        m_TileSizeInput[1] = std::max(8, std::min(256, m_TileSizeInput[1]));

        ImGui::Separator();

        if (ImGui::Button("Load", ImVec2(120, 0))) {
            uint16_t index = TilesetManager::Instance().LoadTileset(
                m_PendingTilesetPath, m_TileSizeInput[0], m_TileSizeInput[1]
            );
            if (index > 0) {
                EditorContext::Get().selectedTilesetIndex = index;
                std::cout << "Loaded tileset: " << m_PendingTilesetPath << std::endl;
                OnCacheInvalidated();
            }
            m_ShowTileSizeDialog = false;
            m_PendingTilesetPath.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_ShowTileSizeDialog = false;
            m_PendingTilesetPath.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void EditorLayer::SetupDockspace() {
    auto& ctx = EditorContext::Get();

    // Setup dockspace window flags
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        RenderMenuBar();
        ImGui::EndMenuBar();
    }

    // Create dockspace
    ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Setup default layout on first run
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

        ImGuiID dockLeft, dockRight, dockBottom, dockCenter;

        // Split left panel (Hierarchy)
        ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.15f, &dockLeft, &dockspaceId);

        // Split right panel (Properties)
        ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.2f, &dockRight, &dockspaceId);

        // Split bottom panel (Asset Browser + Tileset)
        ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.25f, &dockBottom, &dockCenter);

        // Dock windows
        ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Properties", dockRight);
        ImGui::DockBuilderDockWindow("Asset Browser", dockBottom);
        ImGui::DockBuilderDockWindow("Tileset", dockBottom);
        ImGui::DockBuilderDockWindow("Viewport", dockCenter);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::End();

    // Render panels (they will dock themselves)
    if (auto* panel = m_PanelManager.GetPanel<HierarchyPanel>("hierarchy")) {
        bool open = m_PanelManager.IsPanelOpen("hierarchy");
        panel->OnImGuiRender(open);
    }

    if (auto* panel = m_PanelManager.GetPanel<PropertiesPanel>("properties")) {
        bool open = m_PanelManager.IsPanelOpen("properties");
        panel->OnImGuiRender(open);
    }

    if (auto* panel = m_PanelManager.GetPanel<AssetBrowserPanel>("asset_browser")) {
        bool open = m_PanelManager.IsPanelOpen("asset_browser");
        panel->OnImGuiRender(open);
    }

    if (auto* panel = m_PanelManager.GetPanel<TilesetPanel>("tileset")) {
        bool open = m_PanelManager.IsPanelOpen("tileset");
        panel->OnImGuiRender(open);
    }

    if (m_ViewportPanel) {
        bool open = m_PanelManager.IsPanelOpen("viewport");
        m_ViewportPanel->OnImGuiRender(open);
    }

    // Statistics panel (floating)
    if (auto* panel = m_PanelManager.GetPanel<StatisticsPanel>("statistics")) {
        bool open = m_PanelManager.IsPanelOpen("statistics");
        if (open) {
            panel->OnImGuiRender(open);
            m_PanelManager.SetPanelOpen("statistics", open);
        }
    }
}

void EditorLayer::RenderMenuBar() {
    auto& ctx = EditorContext::Get();

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Map", "Ctrl+N")) {
            NewMap();
        }
        if (ImGui::MenuItem("Open Map...", "Ctrl+O")) {
            OpenMap();
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            SaveMap();
        }
        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
            SaveMapAs();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) {
            // TODO: Proper exit handling
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
            // TODO: Undo
        }
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
            // TODO: Redo
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Show Grid", nullptr, &ctx.showGrid);
        ImGui::MenuItem("Show Collision", nullptr, &ctx.showCollision);
        ImGui::Separator();
        bool statsOpen = m_PanelManager.IsPanelOpen("statistics");
        if (ImGui::MenuItem("Statistics", nullptr, &statsOpen)) {
            m_PanelManager.SetPanelOpen("statistics", statsOpen);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Camera")) {
            ctx.cameraX = 0.0f;
            ctx.cameraY = 0.0f;
            ctx.cameraZoom = 1.0f;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Tileset")) {
        if (ImGui::MenuItem("Load Tileset...")) {
            LoadTileset();
        }
        ImGui::EndMenu();
    }

    // Map selector dropdown
    ImGui::Separator();
    ImGui::Text("Map:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    if (ImGui::BeginCombo("##MapSelector", m_TileMap.GetMetadata().name.c_str())) {
        if (ImGui::Selectable(m_TileMap.GetMetadata().name.c_str(), true)) {
            // Already selected
        }
        ImGui::EndCombo();
    }

    // Unsaved indicator
    if (m_TileMap.HasUnsavedChanges()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "*");
    }
}

void EditorLayer::OnCacheInvalidated() {
    if (m_ViewportPanel) {
        m_ViewportPanel->InvalidateTileCache();
    }
}

void EditorLayer::OnTilePainted(int32_t tileX, int32_t tileY) {
    if (m_ViewportPanel) {
        m_ViewportPanel->InvalidateChunkAt(tileX, tileY);
    }
}

void EditorLayer::NewMap() {
    if (m_TileMap.HasUnsavedChanges()) {
        // TODO: Ask to save
    }

    auto& ctx = EditorContext::Get();

    MapMetadata meta;
    meta.name = "New Map";
    meta.id = 1;
    // Uses DEFAULT_TILE_WIDTH/HEIGHT from EditorDefines.h
    m_TileMap.Create(meta);
    m_CurrentMapPath.clear();
    ctx.mapLoaded = true;
    ctx.currentMapPath.clear();
    ctx.cameraX = 0.0f;
    ctx.cameraY = 0.0f;
    ctx.selectedLayerId = m_TileMap.GetMetadata().layers.empty() ? 0 : m_TileMap.GetMetadata().layers[0].id;
    OnCacheInvalidated();
}

void EditorLayer::OpenMap() {
    if (m_TileMap.HasUnsavedChanges()) {
        // TODO: Ask to save first
    }

    m_FileBrowserPurpose = FileBrowserPurpose::OPEN_MAP;
    FileDialog::GetBrowser().Open(ImGuiFileBrowser::Mode::OPEN_FOLDER, "Open Map Folder");
}

void EditorLayer::SaveMap() {
    std::cout << "[SaveMap] Starting..." << std::flush;
    if (m_CurrentMapPath.empty()) {
        std::cout << " path empty, calling SaveMapAs" << std::endl;
        SaveMapAs();
        return;
    }
    std::cout << " path=" << m_CurrentMapPath << std::endl;
    std::cout << "[SaveMap] PrepareMapMetadataForSave..." << std::flush;
    PrepareMapMetadataForSave();
    std::cout << " done" << std::endl;
    std::cout << "[SaveMap] Saving to disk..." << std::flush;
    m_TileMap.Save(m_CurrentMapPath);
    std::cout << " done" << std::endl;
}

void EditorLayer::SaveMapAs() {
    m_FileBrowserPurpose = FileBrowserPurpose::SAVE_MAP;
    FileDialog::GetBrowser().Open(ImGuiFileBrowser::Mode::OPEN_FOLDER, "Save Map To Folder");
}

void EditorLayer::PrepareMapMetadataForSave() {
    auto& ctx = EditorContext::Get();
    MapMetadata& meta = m_TileMap.GetMetadata();

    // Save camera position
    meta.cameraX = ctx.cameraX;
    meta.cameraY = ctx.cameraY;
    meta.cameraZoom = ctx.cameraZoom;

    // Save tileset info
    meta.tilesets.clear();
    const auto& tilesets = TilesetManager::Instance().GetTilesets();
    for (const auto& ts : tilesets) {
        if (ts) {
            TilesetInfo info;
            info.path = ts->GetImagePath();
            info.tileWidth = ts->GetTileWidth();
            info.tileHeight = ts->GetTileHeight();
            meta.tilesets.push_back(info);
        }
    }
}

void EditorLayer::LoadTilesetsFromMetadata() {
    const MapMetadata& meta = m_TileMap.GetMetadata();

    // Clear existing tilesets
    TilesetManager::Instance().Clear();

    // Load each tileset
    for (const auto& tsInfo : meta.tilesets) {
        uint16_t index = TilesetManager::Instance().LoadTileset(
            tsInfo.path, tsInfo.tileWidth, tsInfo.tileHeight);
        if (index > 0) {
            std::cout << "Auto-loaded tileset: " << tsInfo.path << std::endl;
        } else {
            std::cerr << "Failed to load tileset: " << tsInfo.path << std::endl;
        }
    }

    // Select first tileset if any were loaded
    if (TilesetManager::Instance().GetTilesetCount() > 0) {
        EditorContext::Get().selectedTilesetIndex = 1;
    }
}

void EditorLayer::LoadTileset() {
    std::vector<ImGuiFileBrowser::Filter> filters = {
        { "Image Files", "png,jpg,jpeg,gif,bmp" },
        { "PNG Files", "png" }
    };

    m_FileBrowserPurpose = FileBrowserPurpose::LOAD_TILESET;
    FileDialog::GetBrowser().Open(ImGuiFileBrowser::Mode::OPEN_FILE, "Load Tileset Image", filters);
}

} // namespace MMO
