#include "StatisticsPanel.h"
#include "ViewportPanel.h"
#include <Core/Application.h>
#include <imgui.h>

namespace MMO {

void StatisticsPanel::OnImGuiRender() {
    ImGui::Begin("Statistics", &m_IsOpen);

    if (ImGui::BeginTabBar("StatsTabs")) {
        // General tab - FPS and rendering stats
        if (ImGui::BeginTabItem("General")) {
            ImGui::Text("Performance");
            ImGui::Separator();

            float fps = ImGui::GetIO().Framerate;
            float frameTime = 1000.0f / fps;

            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Frame Time: %.2f ms", frameTime);

            ImGui::Spacing();
            ImGui::Text("Rendering");
            ImGui::Separator();

            if (m_Viewport) {
                ImGui::Text("Triangles: %u", m_Viewport->GetTriangleCount());
                ImGui::Text("Draw Calls: %u", m_Viewport->GetDrawCalls());
            } else {
                ImGui::TextDisabled("No viewport available");
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace MMO
