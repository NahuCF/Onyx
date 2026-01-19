#pragma once

#include <cstdint>

namespace MMO {

class ViewportPanel;

class StatisticsPanel {
public:
    StatisticsPanel() = default;
    ~StatisticsPanel() = default;

    void Init(ViewportPanel* viewport);
    void OnImGuiRender();

    bool IsOpen() const { return m_IsOpen; }
    void SetOpen(bool open) { m_IsOpen = open; }

private:
    ViewportPanel* m_Viewport = nullptr;
    bool m_IsOpen = false;
};

} // namespace MMO
