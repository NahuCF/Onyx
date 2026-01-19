#include "ToolbarPanel.h"
#include "World/EditorWorld.h"
#include <imgui.h>

namespace MMO {

void ToolbarPanel::Init(EditorWorld* world) {
    m_World = world;
}

void ToolbarPanel::OnImGuiRender() {
    ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (!m_World) {
        ImGui::End();
        return;
    }

    // Transform mode buttons
    ImGui::Text("Transform:");
    ImGui::SameLine();

    GizmoMode currentMode = m_World->GetGizmoMode();

    // Translate button
    bool translateActive = (currentMode == GizmoMode::TRANSLATE);
    if (translateActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    }
    if (ImGui::Button("Translate (1)")) {
        m_World->SetGizmoMode(GizmoMode::TRANSLATE);
    }
    if (translateActive) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    // Rotate button
    bool rotateActive = (currentMode == GizmoMode::ROTATE);
    if (rotateActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    }
    if (ImGui::Button("Rotate (2)")) {
        m_World->SetGizmoMode(GizmoMode::ROTATE);
    }
    if (rotateActive) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    // Scale button
    bool scaleActive = (currentMode == GizmoMode::SCALE);
    if (scaleActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    }
    if (ImGui::Button("Scale (3)")) {
        m_World->SetGizmoMode(GizmoMode::SCALE);
    }
    if (scaleActive) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // Snap toggle and settings
    bool snapEnabled = m_World->IsSnapEnabled();
    if (ImGui::Checkbox("Snap (G)", &snapEnabled)) {
        m_World->SetSnapEnabled(snapEnabled);
    }

    if (snapEnabled) {
        ImGui::SameLine();

        // Show snap value based on current mode
        if (currentMode == GizmoMode::TRANSLATE) {
            float snapValue = m_World->GetSnapValue();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::DragFloat("##TransSnap", &snapValue, 0.1f, 0.1f, 10.0f, "%.1f")) {
                m_World->SetSnapValue(snapValue);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("units");
        }
        else if (currentMode == GizmoMode::ROTATE) {
            float snapValue = m_World->GetRotationSnapValue();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::DragFloat("##RotSnap", &snapValue, 1.0f, 1.0f, 90.0f, "%.0f")) {
                m_World->SetRotationSnapValue(snapValue);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("deg");
        }
        else if (currentMode == GizmoMode::SCALE) {
            float snapValue = m_World->GetScaleSnapValue();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::DragFloat("##ScaleSnap", &snapValue, 0.05f, 0.1f, 2.0f, "%.2f")) {
                m_World->SetScaleSnapValue(snapValue);
            }
        }
    }

    ImGui::Separator();

    // Quick create buttons
    ImGui::Text("Create:");
    ImGui::SameLine();

    if (ImGui::Button("Cube")) {
        auto* obj = m_World->CreateStaticObject("Cube");
        m_World->Select(obj);
    }

    ImGui::SameLine();
    if (ImGui::Button("Spawn")) {
        auto* obj = m_World->CreateSpawnPoint();
        m_World->Select(obj);
    }

    ImGui::SameLine();
    if (ImGui::Button("Light")) {
        auto* obj = m_World->CreateLight(LightType::POINT);
        m_World->Select(obj);
    }

    ImGui::SameLine();
    if (ImGui::Button("Trigger")) {
        auto* obj = m_World->CreateTriggerVolume();
        m_World->Select(obj);
    }

    ImGui::SameLine();
    if (ImGui::Button("Portal")) {
        auto* obj = m_World->CreateInstancePortal();
        m_World->Select(obj);
    }

    ImGui::End();
}

} // namespace MMO
