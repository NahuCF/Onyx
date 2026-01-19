#include "PanelManager.h"

namespace MMO {

void PanelManager::OnImGuiRender() {
    for (auto& [id, data] : m_Panels) {
        if (data.isOpen) {
            data.panel->OnImGuiRender(data.isOpen);
        }
    }
}

void PanelManager::OnEvent() {
    for (auto& [id, data] : m_Panels) {
        data.panel->OnEvent();
    }
}

void PanelManager::SetPanelOpen(const char* id, bool open) {
    auto it = m_Panels.find(id);
    if (it != m_Panels.end()) {
        it->second.isOpen = open;
    }
}

bool PanelManager::IsPanelOpen(const char* id) const {
    auto it = m_Panels.find(id);
    if (it != m_Panels.end()) {
        return it->second.isOpen;
    }
    return false;
}

} // namespace MMO
