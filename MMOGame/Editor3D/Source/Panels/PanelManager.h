#pragma once

#include "EditorPanel.h"
#include <vector>
#include <memory>
#include <imgui.h>

namespace MMO {

class PanelManager {
public:
    template<typename T, typename... Args>
    T* AddPanel(const std::string& name, bool openByDefault, Args&&... args) {
        auto panel = std::make_unique<T>(std::forward<Args>(args)...);
        panel->m_Name = name;
        panel->m_IsOpen = openByDefault;
        T* ptr = panel.get();
        m_Panels.push_back(std::move(panel));
        return ptr;
    }

    void SetWorld(EditorWorld* world) {
        for (auto& panel : m_Panels) {
            panel->SetWorld(world);
        }
    }

    void OnInit() {
        for (auto& panel : m_Panels) {
            panel->OnInit();
        }
    }

    void OnImGuiRender() {
        for (auto& panel : m_Panels) {
            if (panel->IsOpen()) {
                panel->OnImGuiRender();
            }
        }
    }

    void RenderViewMenu() {
        if (ImGui::BeginMenu("View")) {
            ImGui::SeparatorText("Panels");
            RenderPanelToggles();
            ImGui::EndMenu();
        }
    }

    void RenderPanelToggles() {
        for (auto& panel : m_Panels) {
            bool open = panel->IsOpen();
            if (ImGui::MenuItem(panel->GetName().c_str(), nullptr, &open)) {
                panel->SetOpen(open);
            }
        }
    }

    template<typename T>
    T* GetPanel(const std::string& name) {
        for (auto& panel : m_Panels) {
            if (panel->GetName() == name) {
                return static_cast<T*>(panel.get());
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<EditorPanel>> m_Panels;
};

} // namespace MMO
