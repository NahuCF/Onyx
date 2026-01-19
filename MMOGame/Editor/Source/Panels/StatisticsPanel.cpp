#include "StatisticsPanel.h"
#include "EditorContext.h"
#include <Core/Application.h>
#include <imgui.h>

namespace MMO {

StatisticsPanel::StatisticsPanel() {
    m_ID = "statistics";
    m_Name = "Statistics";
}

void StatisticsPanel::OnImGuiRender(bool& isOpen) {
    if (!isOpen) return;

    ImGui::Begin("Statistics", &isOpen);

    if (ImGui::BeginTabBar("StatsTabs")) {
        // General tab - FPS and Renderer2D stats
        if (ImGui::BeginTabItem("General")) {
            auto& ctx = EditorContext::Get();

            int fps = Onyx::Application::GetInstance().GetWindow()->GetFramerate();
            ImGui::Text("FPS: %d", fps);

            ImGui::Separator();

            if (ctx.renderer) {
                const auto& stats = ctx.renderer->GetStats();
                ImGui::Text("Draw Calls: %d", stats.DrawCalls);
                ImGui::Text("Quads: %d", stats.QuadCount);
            } else {
                ImGui::TextDisabled("No renderer available");
            }

            ImGui::EndTabItem();
        }

        // Chunks tab - list all loaded chunks
        if (ImGui::BeginTabItem("Chunks")) {
            auto& ctx = EditorContext::Get();

            if (ctx.tileMap) {
                const auto& chunks = ctx.tileMap->GetLoadedChunks();
                ImGui::Text("Loaded: %zu", chunks.size());
                ImGui::Separator();

                for (const auto& [coord, chunk] : chunks) {
                    std::string label = "Chunk (" + std::to_string(coord.x) + ", " + std::to_string(coord.y) + ")";
                    if (chunk->IsDirty()) {
                        label += " *";
                    }
                    ImGui::Text("%s", label.c_str());
                }
            } else {
                ImGui::TextDisabled("No map loaded");
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace MMO
