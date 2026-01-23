#pragma once

#include <Core/Layer.h>
#include "Panels/PanelManager.h"
#include "World/EditorWorld.h"
#include <memory>

namespace MMO {

class ViewportPanel;
class InspectorPanel;
class StatisticsPanel;
class LightingPanel;

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

    // World data
    EditorWorld m_World;

    // Panel manager
    PanelManager m_PanelManager;

    // Quick access to specific panels
    ViewportPanel* m_ViewportPanel = nullptr;
};

} // namespace MMO
