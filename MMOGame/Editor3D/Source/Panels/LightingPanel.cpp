#include "LightingPanel.h"
#include "ViewportPanel.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace MMO {

void LightingPanel::OnImGuiRender() {
    if (!m_Viewport) return;

    ImGui::Begin("Lighting", &m_IsOpen);

    // Directional Light Section
    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Light direction
        glm::vec3& lightDir = m_Viewport->GetLightDir();
        if (ImGui::DragFloat3("Direction", glm::value_ptr(lightDir), 0.01f, -1.0f, 1.0f)) {
            // Normalize if not zero
            float len = glm::length(lightDir);
            if (len > 0.001f) {
                lightDir = glm::normalize(lightDir);
            }
        }

        // Direction presets
        if (ImGui::Button("Noon")) {
            lightDir = glm::vec3(0.0f, -1.0f, 0.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Morning")) {
            lightDir = glm::normalize(glm::vec3(0.5f, -0.5f, 0.3f));
        }
        ImGui::SameLine();
        if (ImGui::Button("Evening")) {
            lightDir = glm::normalize(glm::vec3(-0.5f, -0.3f, -0.3f));
        }

        // Light color
        glm::vec3& lightColor = m_Viewport->GetLightColor();
        ImGui::ColorEdit3("Color", glm::value_ptr(lightColor));

        // Color presets
        if (ImGui::Button("White")) {
            lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Warm")) {
            lightColor = glm::vec3(1.0f, 0.95f, 0.8f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cool")) {
            lightColor = glm::vec3(0.8f, 0.9f, 1.0f);
        }

        // Ambient strength
        float& ambient = m_Viewport->GetAmbientStrength();
        ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);

        ImGui::Unindent();
    }

    // Shadows Section
    if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        // Enable shadows
        bool& enableShadows = m_Viewport->GetEnableShadows();
        ImGui::Checkbox("Enable Shadows", &enableShadows);

        if (enableShadows) {
            // Shadow bias
            float& bias = m_Viewport->GetShadowBias();
            ImGui::SliderFloat("Bias", &bias, 0.0001f, 0.01f, "%.4f");

            // Shadow distance
            float& distance = m_Viewport->GetShadowDistance();
            ImGui::SliderFloat("Distance", &distance, 10.0f, 500.0f);

            // Shadow map resolution
            static const char* resolutions[] = { "512", "1024", "2048", "4096" };
            static const uint32_t resValues[] = { 512, 1024, 2048, 4096 };

            uint32_t currentSize = m_Viewport->GetShadowMapSize();
            int currentIdx = 2; // Default to 2048
            for (int i = 0; i < 4; i++) {
                if (resValues[i] == currentSize) {
                    currentIdx = i;
                    break;
                }
            }

            if (ImGui::Combo("Resolution", &currentIdx, resolutions, 4)) {
                m_Viewport->SetShadowMapSize(resValues[currentIdx]);
            }
        }

        ImGui::Unindent();
    }

    // Visual Settings Section
    if (ImGui::CollapsingHeader("Visual Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        bool& showGrid = m_Viewport->GetShowGrid();
        ImGui::Checkbox("Show Grid", &showGrid);

        bool& wireframe = m_Viewport->GetShowWireframe();
        ImGui::Checkbox("Wireframe Mode", &wireframe);

        bool& msaa = m_Viewport->GetEnableMSAA();
        if (ImGui::Checkbox("Enable MSAA (4x)", &msaa)) {
            // Note: MSAA change requires framebuffer recreation
            // This will happen on next resize
        }

        ImGui::Unindent();
    }

    ImGui::End();
}

} // namespace MMO
