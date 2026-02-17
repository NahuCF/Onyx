#include "MapBrowserDialog.h"
#include <imgui.h>
#include <cstring>

namespace Editor3D {

bool MapBrowserDialog::Render() {
    m_HasSelection = false;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_Appearing);

    if (!ImGui::Begin("Map Browser", nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) {
        ImGui::End();
        return false;
    }

    RenderMapList();

    if (m_ShowNewMapDialog) {
        ImGui::OpenPopup("Create New Map");
        m_ShowNewMapDialog = false;
    }

    RenderNewMapDialog();

    if (m_ShowDeleteConfirm) {
        ImGui::OpenPopup("Delete Map?");
        m_ShowDeleteConfirm = false;
    }

    if (ImGui::BeginPopupModal("Delete Map?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const MapEntry* entry = m_Registry->GetMapById(m_DeleteTargetId);
        if (entry) {
            ImGui::Text("Delete \"%s\"? Tas seguro amiguito?", entry->displayName.c_str());
        }
        ImGui::Separator();
        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            m_Registry->DeleteMap(m_DeleteTargetId);
            m_DeleteTargetId = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_DeleteTargetId = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
    return m_HasSelection;
}

void MapBrowserDialog::RenderMapList() {
    const auto& maps = m_Registry->GetMaps();

    // Buttons at top
    if (ImGui::Button("New Map")) {
        m_ShowNewMapDialog = true;
        std::memset(m_NewMapName, 0, sizeof(m_NewMapName));
        m_NewMapType = MapInstanceType::OpenWorld;
        m_NewMapMaxPlayers = 0;
    }

    if (!maps.empty()) {
        ImGui::SameLine();
        // Only show Open/Delete if a row is highlighted
        static uint32_t hoveredMapId = 0;

        if (hoveredMapId > 0) {
            if (ImGui::Button("Open")) {
                m_SelectedMapId = hoveredMapId;
                m_HasSelection = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                m_DeleteTargetId = hoveredMapId;
                m_ShowDeleteConfirm = true;
            }
        }

        ImGui::Separator();

        // Map table
        if (ImGui::BeginTable("MapTable", 3,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
                ImVec2(0, -1))) {

            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableHeadersRow();

            hoveredMapId = 0;
            for (const auto& entry : maps) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                bool selected = (m_SelectedMapId == entry.id);
                if (ImGui::Selectable(entry.displayName.c_str(), selected,
                        ImGuiSelectableFlags_SpanAllColumns |
                        ImGuiSelectableFlags_AllowDoubleClick)) {
                    m_SelectedMapId = entry.id;
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        m_HasSelection = true;
                    }
                }

                if (m_SelectedMapId == entry.id) {
                    hoveredMapId = entry.id;
                }

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(MapInstanceTypeName(entry.instanceType));

                ImGui::TableNextColumn();
                ImGui::Text("%u", entry.id);
            }

            ImGui::EndTable();
        }
    } else {
        ImGui::Separator();
        ImGui::TextWrapped("No maps found.");
    }
}

void MapBrowserDialog::RenderNewMapDialog() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Create New Map", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Name", m_NewMapName, sizeof(m_NewMapName));
        if (ImGui::BeginCombo("Type", MapInstanceTypeName(m_NewMapType))) {
            for (int i = 0; i <= static_cast<int>(MapInstanceType::Arena); i++) {
                auto type = static_cast<MapInstanceType>(i);
                bool selected = (m_NewMapType == type);
                if (ImGui::Selectable(MapInstanceTypeName(type), selected)) {
                    m_NewMapType = type;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::InputInt("Max Players (0 = unlimited)", &m_NewMapMaxPlayers);
        if (m_NewMapMaxPlayers < 0) m_NewMapMaxPlayers = 0;

        ImGui::Separator();

        bool nameValid = std::strlen(m_NewMapName) > 0;

        if (!nameValid) ImGui::BeginDisabled();
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            uint32_t newId = m_Registry->CreateMap(
                m_NewMapName,
                m_NewMapType,
                m_NewMapMaxPlayers
            );
            m_SelectedMapId = newId;
            m_HasSelection = true;
            ImGui::CloseCurrentPopup();
        }
        if (!nameValid) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void MapBrowserDialog::Reset() {
    m_HasSelection = false;
    m_SelectedMapId = 0;
    m_ShowNewMapDialog = false;
    m_ShowDeleteConfirm = false;
    m_DeleteTargetId = 0;
}

} // namespace Editor3D
