#include "Editor3DLayer.h"
#include "Panels/ViewportPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/AssetBrowserPanel.h"
#include "Panels/StatisticsPanel.h"
#include "Panels/LightingPanel.h"
#include "Panels/TerrainPanel.h"
#include "Panels/MaterialEditorPanel.h"
#include "Terrain/MaterialSerializer.h"
#include <filesystem>
#include "Commands/EditorCommand.h"
#include <Core/Application.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace MMO {

Editor3DLayer::Editor3DLayer() = default;

void Editor3DLayer::OnAttach() {
    auto& window = *Onyx::Application::GetInstance().GetWindow();
    window.SetVSync(false);

    // Load map registry
    m_MapRegistry.Load("maps");
    m_MapBrowser.SetRegistry(&m_MapRegistry);
    m_ShowMapBrowser = true;
    m_MapLoaded = false;

    // Create SceneRenderer
    m_SceneRenderer = std::make_unique<Onyx::SceneRenderer>();
    
    m_SceneRenderer->Init(&Onyx::Application::GetInstance().GetAssetManager());
    // Setup panels (created once, rendered only when map is loaded)
    m_ViewportPanel = m_PanelManager.AddPanel<ViewportPanel>("Viewport", true);
    m_ViewportPanel->SetSceneRenderer(m_SceneRenderer.get());

    m_PanelManager.AddPanel<HierarchyPanel>("Hierarchy", true);

    auto* inspector = m_PanelManager.AddPanel<InspectorPanel>("Inspector", true);
    inspector->SetViewport(m_ViewportPanel);

    m_PanelManager.AddPanel<AssetBrowserPanel>("Asset Browser", true);

    auto* stats = m_PanelManager.AddPanel<StatisticsPanel>("Statistics", false);
    stats->SetViewport(m_ViewportPanel);

    auto* lighting = m_PanelManager.AddPanel<LightingPanel>("Lighting", true);
    lighting->SetViewport(m_ViewportPanel);

    auto* terrain = m_PanelManager.AddPanel<TerrainPanel>("Terrain", true);
    terrain->SetViewport(m_ViewportPanel);

    auto* materialEditor = m_PanelManager.AddPanel<MaterialEditorPanel>("Material Editor", false);
    materialEditor->SetMaterialLibrary(&m_ViewportPanel->GetTerrainMaterialLibrary());

    m_PanelManager.SetWorld(&m_World);
    m_PanelManager.OnInit();

    terrain->SetMaterialLibrary(&m_ViewportPanel->GetTerrainMaterialLibrary());

    auto* assetBrowser = m_PanelManager.GetPanel<AssetBrowserPanel>("Asset Browser");
    if (assetBrowser) {
        assetBrowser->SetMaterialOpenCallback([materialEditor, this](const std::string& filePath) {
            std::filesystem::path p(filePath);
            std::string matId = p.stem().string();

            auto& lib = m_ViewportPanel->GetTerrainMaterialLibrary();
            Onyx::Material mat;
            if (Editor3D::LoadMaterial(filePath, mat)) {
                if (mat.id.empty()) mat.id = matId;
                lib.UpdateMaterial(mat.id, mat);
                materialEditor->OpenMaterial(mat.id);
            }
        });

        assetBrowser->SetMaterialCreateCallback([this, materialEditor](const std::string& currentDir) {
            auto& lib = m_ViewportPanel->GetTerrainMaterialLibrary();
            std::string newId = lib.CreateMaterial("New Material", currentDir);
            materialEditor->OpenMaterial(newId);
        });
    }
}

void Editor3DLayer::OnUpdate() {
    if (!m_MapLoaded) return;
    HandleGlobalShortcuts();
}

void Editor3DLayer::OnImGui() {
    SetupDockspace();
}

void Editor3DLayer::SetupDockspace() {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar()) {
        RenderMenuBar();
        ImGui::EndMenuBar();
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        static bool firstTime = true;
        if (firstTime) {
            firstTime = false;

            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

            ImGuiID dockLeft, dockRight, dockBottom;
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, &dockLeft, &dockspaceId);
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.25f, &dockRight, &dockspaceId);
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.25f, &dockBottom, &dockspaceId);

            ImGuiID dockRightTop, dockRightBottom;
            ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Down, 0.4f, &dockRightBottom, &dockRightTop);

            ImGuiID dockLeftTop, dockLeftBottom;
            ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.4f, &dockLeftBottom, &dockLeftTop);

            ImGui::DockBuilderDockWindow("Hierarchy", dockLeftTop);
            ImGui::DockBuilderDockWindow("Lighting", dockLeftBottom);
            ImGui::DockBuilderDockWindow("Inspector", dockRightTop);
            ImGui::DockBuilderDockWindow("Terrain", dockRightBottom);
            ImGui::DockBuilderDockWindow("Asset Browser", dockBottom);
            ImGui::DockBuilderDockWindow("Viewport", dockspaceId);

            ImGui::DockBuilderFinish(dockspaceId);
        }
    }

    ImGui::End();

    // Show map browser if no map is loaded
    if (m_ShowMapBrowser) {
        if (m_MapBrowser.Render()) {
            uint32_t selectedId = m_MapBrowser.GetSelectedMapId();
            if (selectedId > 0) {
                LoadMap(selectedId);
                m_ShowMapBrowser = false;
            }
        }
    }

    // Only render panels when a map is loaded
    if (m_MapLoaded) {
        m_PanelManager.OnImGuiRender();
    }
}

void Editor3DLayer::RenderMenuBar() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open Map", "Ctrl+O")) {
            m_MapBrowser.Reset();
            m_ShowMapBrowser = true;
        }
        if (ImGui::MenuItem("Save", "Ctrl+S", false, m_MapLoaded)) {
            if (m_ViewportPanel) {
                m_ViewportPanel->GetWorldSystem().SaveDirtyChunks();
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) {
            Onyx::Application::GetInstance().GetWindow()->CloseWindow();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        auto& history = CommandHistory::Get();

        std::string undoLabel = "Undo";
        if (history.CanUndo()) {
            undoLabel += " " + history.GetUndoDescription();
        }
        if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, history.CanUndo())) {
            history.Undo();
        }

        std::string redoLabel = "Redo";
        if (history.CanRedo()) {
            redoLabel += " " + history.GetRedoDescription();
        }
        if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, history.CanRedo())) {
            history.Redo();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete", "Delete", false, m_World.HasSelection())) {
            m_World.DeleteSelected();
        }
        if (ImGui::MenuItem("Select All", "Ctrl+A")) {
            m_World.SelectAll();
        }
        if (ImGui::MenuItem("Deselect All", "Escape")) {
            m_World.DeselectAll();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Create")) {
        if (ImGui::MenuItem("Static Object")) {
            auto* obj = m_World.CreateStaticObject();
            m_World.Select(obj);
        }
        if (ImGui::MenuItem("Spawn Point")) {
            auto* obj = m_World.CreateSpawnPoint();
            m_World.Select(obj);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Point Light")) {
            auto* obj = m_World.CreateLight(LightType::POINT);
            m_World.Select(obj);
        }
        if (ImGui::MenuItem("Spot Light")) {
            auto* obj = m_World.CreateLight(LightType::SPOT);
            m_World.Select(obj);
        }
        if (ImGui::MenuItem("Directional Light")) {
            auto* obj = m_World.CreateLight(LightType::DIRECTIONAL);
            m_World.Select(obj);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Particle Emitter")) {
            auto* obj = m_World.CreateParticleEmitter();
            m_World.Select(obj);
        }
        if (ImGui::MenuItem("Trigger Volume")) {
            auto* obj = m_World.CreateTriggerVolume();
            m_World.Select(obj);
        }
        if (ImGui::MenuItem("Instance Portal")) {
            auto* obj = m_World.CreateInstancePortal();
            m_World.Select(obj);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        ImGui::SeparatorText("Panels");
        m_PanelManager.RenderPanelToggles();

        ImGui::SeparatorText("Object Visibility");

        bool staticVisible = m_World.IsTypeVisible(WorldObjectType::STATIC_OBJECT);
        bool spawnVisible = m_World.IsTypeVisible(WorldObjectType::SPAWN_POINT);
        bool lightVisible = m_World.IsTypeVisible(WorldObjectType::LIGHT);
        bool particleVisible = m_World.IsTypeVisible(WorldObjectType::PARTICLE_EMITTER);
        bool triggerVisible = m_World.IsTypeVisible(WorldObjectType::TRIGGER_VOLUME);
        bool portalVisible = m_World.IsTypeVisible(WorldObjectType::INSTANCE_PORTAL);

        if (ImGui::MenuItem("Static Objects", nullptr, &staticVisible)) {
            m_World.SetTypeVisible(WorldObjectType::STATIC_OBJECT, staticVisible);
        }
        if (ImGui::MenuItem("Spawn Points", nullptr, &spawnVisible)) {
            m_World.SetTypeVisible(WorldObjectType::SPAWN_POINT, spawnVisible);
        }
        if (ImGui::MenuItem("Lights", nullptr, &lightVisible)) {
            m_World.SetTypeVisible(WorldObjectType::LIGHT, lightVisible);
        }
        if (ImGui::MenuItem("Particle Emitters", nullptr, &particleVisible)) {
            m_World.SetTypeVisible(WorldObjectType::PARTICLE_EMITTER, particleVisible);
        }
        if (ImGui::MenuItem("Trigger Volumes", nullptr, &triggerVisible)) {
            m_World.SetTypeVisible(WorldObjectType::TRIGGER_VOLUME, triggerVisible);
        }
        if (ImGui::MenuItem("Instance Portals", nullptr, &portalVisible)) {
            m_World.SetTypeVisible(WorldObjectType::INSTANCE_PORTAL, portalVisible);
        }
        ImGui::EndMenu();
    }

    // Show current map name in the menu bar
    if (m_MapLoaded) {
        const MapEntry* entry = m_MapRegistry.GetMapById(m_CurrentMapId);
        if (entry) {
            float availWidth = ImGui::GetContentRegionAvail().x;
            std::string mapInfo = entry->displayName + " (" + MapInstanceTypeName(entry->instanceType) + ")";
            float textWidth = ImGui::CalcTextSize(mapInfo.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - textWidth);
            ImGui::TextDisabled("%s", mapInfo.c_str());
        }
    }
}

void Editor3DLayer::HandleGlobalShortcuts() {
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput) return;

    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        m_World.DeleteSelected();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_World.DeselectAll();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
        m_World.SelectAll();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
        CommandHistory::Get().Undo();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
        CommandHistory::Get().Redo();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (m_MapLoaded && m_ViewportPanel) {
            m_ViewportPanel->GetWorldSystem().SaveDirtyChunks();
        }
    }
}

void Editor3DLayer::LoadMap(uint32_t mapId) {
    const MapEntry* entry = m_MapRegistry.GetMapById(mapId);
    if (!entry) return;

    // Unload previous map if any
    if (m_MapLoaded) {
        UnloadMap();
    }

    m_CurrentMapId = mapId;
    m_MapLoaded = true;

    m_World.NewMap(entry->displayName);

    // Initialize terrain system with map-scoped chunks directory
    std::string chunksDir = m_MapRegistry.GetChunksDirectory(mapId);
    m_ViewportPanel->GetWorldSystem().Init(chunksDir);
}

void Editor3DLayer::UnloadMap() {
    if (!m_MapLoaded) return;

    // Save dirty terrain before unloading
    if (m_ViewportPanel) {
        m_ViewportPanel->GetWorldSystem().SaveDirtyChunks();
        m_ViewportPanel->GetWorldSystem().Shutdown();
    }

    m_World.NewMap("Untitled Map");
    m_CurrentMapId = 0;
    m_MapLoaded = false;
}

} // namespace MMO
