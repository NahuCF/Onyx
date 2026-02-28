#include "StatisticsPanel.h"
#include "ViewportPanel.h"
#include "../World/EditorWorldSystem.h"
#include <imgui.h>

namespace MMO {

void StatisticsPanel::OnImGuiRender() {
    ImGui::Begin("Statistics", &m_IsOpen);

    float rawDeltaTime = ImGui::GetIO().DeltaTime * 1000.0f;
    float fps = ImGui::GetIO().Framerate;

    if (ImGui::BeginTabBar("StatsTabs")) {
        if (ImGui::BeginTabItem("General")) {
            ImGui::Text("Performance");
            ImGui::Separator();

            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Frame Time: %.2f ms", rawDeltaTime);

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
                ImGui::Spacing();
                ImGui::Text("Frustum Culling");
                ImGui::Separator();
                ImGui::Text("Meshes Submitted: %u", stats.meshesSubmitted);
                ImGui::Text("Meshes Culled: %u", stats.meshesCulled);
                ImGui::Text("Meshes Rendered: %u", stats.meshesSubmitted - stats.meshesCulled);
            } else {
                ImGui::TextDisabled("No viewport available");
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Render Passes")) {
            if (m_Viewport) {
                ImGui::Checkbox("Profile Pass Timing (reduces FPS)", &m_Viewport->GetProfilePassTiming());
                ImGui::Separator();

                if (m_Viewport->GetProfilePassTiming()) {
                    ImGui::Text("Total Render:    %6.2f ms", m_Viewport->GetTotalRenderTime());
                    ImGui::Text("Submit Models:   %6.2f ms", m_Viewport->GetSubmitModelsTime());
                    ImGui::Text("  Resolve Model: %6.2f ms", m_Viewport->GetResolveModelTime());
                    ImGui::Text("Shadow Pass:     %6.2f ms", m_Viewport->GetShadowPassTime());
                    ImGui::Text("Terrain:         %6.2f ms", m_Viewport->GetTerrainPassTime());
                    ImGui::Text("Batch Render:    %6.2f ms", m_Viewport->GetBatchRenderTime());
                    ImGui::Text("World Objects:   %6.2f ms", m_Viewport->GetWorldObjectRenderTime());
                } else {
                    ImGui::TextDisabled("Enable profiling for per-pass timing");
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
