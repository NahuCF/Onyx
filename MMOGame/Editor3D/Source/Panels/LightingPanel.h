#pragma once

namespace MMO {

class ViewportPanel;

class LightingPanel {
public:
    LightingPanel() = default;
    ~LightingPanel() = default;

    void Init(ViewportPanel* viewport);
    void OnImGuiRender();

    bool IsOpen() const { return m_IsOpen; }
    void SetOpen(bool open) { m_IsOpen = open; }

private:
    ViewportPanel* m_Viewport = nullptr;
    bool m_IsOpen = true;
};

} // namespace MMO
