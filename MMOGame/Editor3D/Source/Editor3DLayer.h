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

    EditorWorld m_World;
    PanelManager m_PanelManager;
    ViewportPanel* m_ViewportPanel = nullptr;
};

} // namespace MMO
