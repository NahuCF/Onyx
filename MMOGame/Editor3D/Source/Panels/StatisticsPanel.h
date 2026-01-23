#pragma once

#include "EditorPanel.h"
#include <cstdint>

namespace MMO {

class ViewportPanel;

class StatisticsPanel : public EditorPanel {
public:
    StatisticsPanel() { m_Name = "Statistics"; m_IsOpen = false; }
    ~StatisticsPanel() override = default;

    void SetViewport(ViewportPanel* viewport) { m_Viewport = viewport; }
    void OnImGuiRender() override;

private:
    ViewportPanel* m_Viewport = nullptr;
};

} // namespace MMO
