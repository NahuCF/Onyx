#pragma once

#include <Core/Layer.h>
#include "Panels/PanelManager.h"
#include "World/EditorWorld.h"
#include "Map/EditorMapRegistry.h"
#include "Map/MapBrowserDialog.h"
#include <memory>

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

    EditorWorld m_World;
    PanelManager m_PanelManager;
    ViewportPanel* m_ViewportPanel = nullptr;

    // Map management
    Editor3D::EditorMapRegistry m_MapRegistry;
    Editor3D::MapBrowserDialog m_MapBrowser;
    uint32_t m_CurrentMapId = 0;
    bool m_MapLoaded = false;
    bool m_ShowMapBrowser = true;
};

} // namespace MMO
