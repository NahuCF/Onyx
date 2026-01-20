#include "Editor3DLayer.h"
#include "Commands/EditorCommand.h"
#include <Core/Application.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace MMO {

Editor3DLayer::Editor3DLayer() = default;

void Editor3DLayer::OnAttach() {
    auto& window = *Onyx::Application::GetInstance().GetWindow();
    window.SetVSync(false);

    // Initialize world with a new map
    m_World.NewMap("Untitled Map");

    // Create ground plane for shadow testing
    auto* groundPlane = m_World.CreateStaticObject("Ground Plane");
    groundPlane->SetPosition(glm::vec3(0.0f, -0.5f, 0.0f));
    groundPlane->SetScale(50.0f);  // Large ground plane
    groundPlane->SetModelPath("#plane");

    // Create some test objects
    auto* obj1 = m_World.CreateStaticObject("Test Cube 1");
    obj1->SetPosition(glm::vec3(0.0f, 0.5f, 0.0f));

    auto* obj2 = m_World.CreateStaticObject("Test Cube 2");
    obj2->SetPosition(glm::vec3(5.0f, 0.5f, 0.0f));

    auto* obj3 = m_World.CreateStaticObject("Test Cube 3");
    obj3->SetPosition(glm::vec3(-5.0f, 0.5f, 0.0f));

    auto* spawn = m_World.CreateSpawnPoint("Goblin Spawn");
    spawn->SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    spawn->SetCreatureTemplateId(1);

    auto* light = m_World.CreateLight(LightType::POINT, "Main Light");
    light->SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
    light->SetColor(glm::vec3(1.0f, 0.9f, 0.8f));
    light->SetRadius(50.0f);

    // Create panels
    m_ViewportPanel = std::make_unique<ViewportPanel>();
    m_ViewportPanel->Init(&m_World);

    m_HierarchyPanel = std::make_unique<HierarchyPanel>();
    m_HierarchyPanel->Init(&m_World);

    m_InspectorPanel = std::make_unique<InspectorPanel>();
    m_InspectorPanel->Init(&m_World, m_ViewportPanel.get());

    m_AssetBrowserPanel = std::make_unique<AssetBrowserPanel>();
    m_AssetBrowserPanel->Init(&m_World);

    m_StatisticsPanel = std::make_unique<StatisticsPanel>();
    m_StatisticsPanel->Init(m_ViewportPanel.get());

    m_LightingPanel = std::make_unique<LightingPanel>();
    m_LightingPanel->Init(m_ViewportPanel.get());
}

void Editor3DLayer::OnUpdate() {
    HandleGlobalShortcuts();
}

void Editor3DLayer::OnImGui() {
    SetupDockspace();
}

void Editor3DLayer::SetupDockspace() {
    // Enable fullscreen dockspace
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

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        RenderMenuBar();
        ImGui::EndMenuBar();
    }

    // Create dockspace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // Setup default layout on first run
        static bool firstTime = true;
        if (firstTime) {
            firstTime = false;

            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

            // Split the dockspace
            ImGuiID dockLeft, dockRight, dockBottom;
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, &dockLeft, &dockspaceId);
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.25f, &dockRight, &dockspaceId);
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.25f, &dockBottom, &dockspaceId);

            // Dock windows
            ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
            ImGui::DockBuilderDockWindow("Inspector", dockRight);
            ImGui::DockBuilderDockWindow("Asset Browser", dockBottom);
            ImGui::DockBuilderDockWindow("Viewport", dockspaceId);

            ImGui::DockBuilderFinish(dockspaceId);
        }
    }

    ImGui::End();  // DockSpace

    // Render panels (they will dock into the dockspace)
    m_HierarchyPanel->OnImGuiRender();
    m_ViewportPanel->OnImGuiRender();
    m_InspectorPanel->OnImGuiRender();
    m_AssetBrowserPanel->OnImGuiRender();
    m_StatisticsPanel->OnImGuiRender();
    m_LightingPanel->OnImGuiRender();
}

void Editor3DLayer::RenderMenuBar() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
            m_World.NewMap("Untitled Map");
        }
        if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
            // TODO: File dialog
        }
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
            // TODO: Save
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
        // Panels
        bool statsOpen = m_StatisticsPanel->IsOpen();
        if (ImGui::MenuItem("Statistics", nullptr, &statsOpen)) {
            m_StatisticsPanel->SetOpen(statsOpen);
        }

        bool lightingOpen = m_LightingPanel->IsOpen();
        if (ImGui::MenuItem("Lighting", nullptr, &lightingOpen)) {
            m_LightingPanel->SetOpen(lightingOpen);
        }

        ImGui::Separator();

        // Object visibility
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
}

void Editor3DLayer::HandleGlobalShortcuts() {
    ImGuiIO& io = ImGui::GetIO();

    // Don't handle shortcuts if typing in a text field
    if (io.WantTextInput) return;

    // Delete
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        m_World.DeleteSelected();
    }

    // Escape - deselect
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_World.DeselectAll();
    }

    // Ctrl+A - select all
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
        m_World.SelectAll();
    }

    // Ctrl+Z - undo
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
        CommandHistory::Get().Undo();
    }

    // Ctrl+Y - redo
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
        CommandHistory::Get().Redo();
    }
}

} // namespace MMO
