#pragma once

#include <Core/Layer.h>
#include <Graphics/SceneRenderer.h>
#include "Panels/PanelManager.h"
#include "World/EditorWorld.h"
#include "Map/EditorMapRegistry.h"
#include "Map/MapBrowserDialog.h"
#include <memory>

#ifdef HAS_DATABASE
#include <Database/Database.h>
#endif

namespace MMO {

class ViewportPanel;
class InspectorPanel;
class StatisticsPanel;
class LightingPanel;
class MaterialEditorPanel;

class Editor3DLayer : public Onyx::Layer {
public:
    Editor3DLayer();
    ~Editor3DLayer() override = default;

    void OnAttach() override;
    void OnUpdate() override;
    void OnImGui() override;

    EditorWorld& GetWorld() { return m_World; }

private:
    void SetupDockspace();
    void RenderMenuBar();
    void HandleGlobalShortcuts();
    void LoadMap(uint32_t mapId);
    void UnloadMap();

#ifdef HAS_DATABASE
    void RenderExportDialog();
    void ExportMapToDatabase();
#endif

    EditorWorld m_World;
    PanelManager m_PanelManager;
    ViewportPanel* m_ViewportPanel = nullptr;
    std::unique_ptr<Onyx::SceneRenderer> m_SceneRenderer;

    // Map management
    Editor3D::EditorMapRegistry m_MapRegistry;
    Editor3D::MapBrowserDialog m_MapBrowser;
    uint32_t m_CurrentMapId = 0;
    bool m_MapLoaded = false;
    bool m_ShowMapBrowser = true;

#ifdef HAS_DATABASE
    // Database export
    Database m_Database;
    bool m_ShowExportDialog = false;
    char m_DbHost[128] = "localhost";
    char m_DbUser[64] = "root";
    char m_DbPass[64] = "root";
    char m_DbName[64] = "mmogame";
    std::vector<std::string> m_ExportLog;
#endif
};

} // namespace MMO
