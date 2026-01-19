#pragma once

#include "EditorPanel.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace MMO {

// ============================================================
// PANEL DATA
// Stores panel instance and metadata
// ============================================================

struct PanelData {
    std::string id;
    std::string name;
    std::unique_ptr<EditorPanel> panel;
    bool isOpen = true;
};

// ============================================================
// PANEL MANAGER
// Manages registration and rendering of all editor panels
// ============================================================

class PanelManager {
public:
    PanelManager() = default;
    ~PanelManager() = default;

    // Register a new panel
    // Returns raw pointer for optional configuration after registration
    template<typename T, typename... Args>
    T* AddPanel(const char* id, const char* name, bool openByDefault, Args&&... args) {
        auto panel = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = panel.get();

        PanelData data;
        data.id = id;
        data.name = name;
        data.panel = std::move(panel);
        data.isOpen = openByDefault;

        m_Panels[id] = std::move(data);
        return ptr;
    }

    // Get a panel by ID (returns nullptr if not found)
    template<typename T>
    T* GetPanel(const char* id) {
        auto it = m_Panels.find(id);
        if (it != m_Panels.end()) {
            return static_cast<T*>(it->second.panel.get());
        }
        return nullptr;
    }

    // Render all open panels
    void OnImGuiRender();

    // Broadcast events to all panels
    void OnEvent();

    // Access panel data (for menu bar iteration)
    std::unordered_map<std::string, PanelData>& GetPanels() { return m_Panels; }
    const std::unordered_map<std::string, PanelData>& GetPanels() const { return m_Panels; }

    // Toggle panel visibility
    void SetPanelOpen(const char* id, bool open);
    bool IsPanelOpen(const char* id) const;

private:
    std::unordered_map<std::string, PanelData> m_Panels;
};

} // namespace MMO
