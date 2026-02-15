#include "LightingPanel.h"
#include "ViewportPanel.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace MMO {

void LightingPanel::OnImGuiRender() {
    if (!m_Viewport) return;

    ImGui::Begin("Lighting", &m_IsOpen);

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        glm::vec3& lightDir = m_Viewport->GetLightDir();
        if (ImGui::DragFloat3("Direction", glm::value_ptr(lightDir), 0.01f, -1.0f, 1.0f)) {
            float len = glm::length(lightDir);
            if (len > 0.001f) {
                lightDir = glm::normalize(lightDir);
            }
        }

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

        glm::vec3& lightColor = m_Viewport->GetLightColor();
        ImGui::ColorEdit3("Color", glm::value_ptr(lightColor));

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

        float& ambient = m_Viewport->GetAmbientStrength();
        ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);

        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        bool& enableShadows = m_Viewport->GetEnableShadows();
        ImGui::Checkbox("Enable Shadows", &enableShadows);

        if (enableShadows) {
            float& bias = m_Viewport->GetShadowBias();
            ImGui::SliderFloat("Bias", &bias, 0.0001f, 0.01f, "%.4f");

            float& distance = m_Viewport->GetShadowDistance();
            ImGui::SliderFloat("Distance", &distance, 10.0f, 500.0f);

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

            ImGui::Separator();
            ImGui::Text("Cascaded Shadow Maps");
            ImGui::SliderFloat("Split Lambda", &m_Viewport->GetSplitLambda(), 0.0f, 1.0f,
                "%.2f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SetItemTooltip("0 = uniform splits, 1 = logarithmic splits");
            ImGui::Checkbox("Show Cascades", &m_Viewport->GetShowCascades());
        }

        ImGui::Unindent();
    }

    ImGui::End();
}

} // namespace MMO
