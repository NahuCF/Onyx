#pragma once

#include <Core/Layer.h>
#include "Panels/ViewportPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/AssetBrowserPanel.h"
#include "Panels/StatisticsPanel.h"
#include "World/EditorWorld.h"
#include <memory>

namespace MMO {

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

    // Panels
    std::unique_ptr<ViewportPanel> m_ViewportPanel;
    std::unique_ptr<HierarchyPanel> m_HierarchyPanel;
    std::unique_ptr<InspectorPanel> m_InspectorPanel;
    std::unique_ptr<AssetBrowserPanel> m_AssetBrowserPanel;
    std::unique_ptr<StatisticsPanel> m_StatisticsPanel;
};

} // namespace MMO
