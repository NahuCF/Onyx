#pragma once

#include "EditorPanel.h"

namespace MMO {

class ViewportPanel;

class LightingPanel : public EditorPanel {
public:
    LightingPanel() { m_Name = "Lighting"; }
    ~LightingPanel() override = default;

    void SetViewport(ViewportPanel* viewport) { m_Viewport = viewport; }
    void OnImGuiRender() override;

private:
    ViewportPanel* m_Viewport = nullptr;
};

} // namespace MMO
