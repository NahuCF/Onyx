#include "StatisticsPanel.h"
#include "ViewportPanel.h"
#include "../World/EditorWorldSystem.h"
#include <Core/Application.h>
#include <imgui.h>

namespace MMO {

void StatisticsPanel::OnImGuiRender() {
    ImGui::Begin("Statistics", &m_IsOpen);

    if (ImGui::BeginTabBar("StatsTabs")) {
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
                auto& stats = m_Viewport->GetRenderStats();
                ImGui::Text("Triangles: %u", stats.triangles);
                ImGui::Text("Draw Calls: %u", stats.drawCalls);
                ImGui::Text("  Static Batched: %u", stats.batchedDrawCalls);
                ImGui::Text("  Batched Meshes: %u", stats.batchedMeshCount);
                ImGui::Text("  Skinned: %u (%u instances)", stats.skinnedDrawCalls, stats.skinnedInstances);

            } else {
                ImGui::TextDisabled("No viewport available");
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Render Passes")) {
            if (m_Viewport) {
                ImGui::Checkbox("Profile Pass Timing (reduces FPS)", &m_Viewport->GetProfilePassTiming());
                ImGui::Separator();

                ImGui::Text("Total Render: %6.2f ms", m_Viewport->GetTotalRenderTime());

                if (m_Viewport->GetProfilePassTiming()) {
                    ImGui::Spacing();
                    ImGui::Text("Shadow Pass:  %6.2f ms", m_Viewport->GetShadowPassTime());
                    ImGui::Text("Terrain:      %6.2f ms", m_Viewport->GetTerrainPassTime());
                    ImGui::Text("World Objects:%6.2f ms", m_Viewport->GetWorldObjectsPassTime());
                } else {
                    ImGui::Spacing();
                    ImGui::TextDisabled("Enable profiling to see per-pass timing");
                }

            } else {
                ImGui::TextDisabled("No viewport available");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Terrain")) {
            if (m_Viewport) {
                auto& stats = m_Viewport->GetWorldSystem().GetStats();
                auto& settings = m_Viewport->GetWorldSystem().GetSettings();

                ImGui::Text("Chunks");
                ImGui::Separator();
                ImGui::Text("Total:   %d", stats.totalChunks);
                ImGui::Text("Loaded:  %d", stats.loadedChunks);
                ImGui::Text("Visible: %d", stats.visibleChunks);
                ImGui::Text("Dirty:   %d", stats.dirtyChunks);

                ImGui::Spacing();
                ImGui::Text("Streaming");
                ImGui::Separator();
                ImGui::SliderFloat("Load Distance", &settings.loadDistance, 64.0f, 4096.0f, "%.0f");
                ImGui::SliderFloat("Unload Distance", &settings.unloadDistance, 128.0f, 8192.0f, "%.0f");
                if (settings.unloadDistance < settings.loadDistance + 64.0f) {
                    settings.unloadDistance = settings.loadDistance + 64.0f;
                }
                ImGui::SliderInt("Chunks/Frame", &settings.maxChunksPerFrame, 1, 16);
                ImGui::Checkbox("Frustum Culling", &settings.enableFrustumCulling);
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
