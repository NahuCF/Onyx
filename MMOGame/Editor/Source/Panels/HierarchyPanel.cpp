#include "HierarchyPanel.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>

namespace MMO {

HierarchyPanel::HierarchyPanel() {
    m_ID = "hierarchy";
    m_Name = "Hierarchy";
}

void HierarchyPanel::OnImGuiRender(bool& isOpen) {
    ImGui::Begin("Hierarchy", &isOpen);

    RenderLayersSection();

    ImGui::End();
}

void HierarchyPanel::RenderLayersSection() {
    auto& ctx = EditorContext::Get();
    if (!ctx.tileMap) return;

    if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
        MapMetadata& meta = ctx.tileMap->GetMetadata();

        // Get layers sorted by sortOrder, reversed (highest = top of list = front)
        auto sortedLayers = meta.GetSortedLayers();
        std::reverse(sortedLayers.begin(), sortedLayers.end());

        float availWidth = ImGui::GetContentRegionAvail().x;
        float rowHeight = ImGui::GetTextLineHeightWithSpacing() + 4;

        for (size_t i = 0; i < sortedLayers.size(); i++) {
            const auto& layerDef = sortedLayers[i];
            ImGui::PushID(layerDef.id);

            bool selected = (ctx.selectedLayerId == layerDef.id);
            bool visible = layerDef.visible;
            bool ySort = layerDef.ySortEnabled;

            ImVec2 rowStart = ImGui::GetCursorScreenPos();
            float textY = rowStart.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f;

            // Draw row background
            ImU32 bgColor = selected ? IM_COL32(70, 100, 150, 150) : IM_COL32(45, 45, 48, 255);
            ImGui::GetWindowDrawList()->AddRectFilled(
                rowStart,
                ImVec2(rowStart.x + availWidth, rowStart.y + rowHeight),
                bgColor,
                3.0f
            );

            // Visibility icon
            ImU32 visColor = visible ? IM_COL32(255, 255, 255, 255) : IM_COL32(100, 100, 100, 255);
            ImGui::GetWindowDrawList()->AddText(ImVec2(rowStart.x + 8, textY), visColor, visible ? "o" : "-");

            // Layer name
            ImU32 nameColor = visible ? IM_COL32(255, 255, 255, 255) : IM_COL32(120, 120, 120, 255);
            ImGui::GetWindowDrawList()->AddText(ImVec2(rowStart.x + 28, textY), nameColor, layerDef.name.c_str());

            // Y-Sort indicator
            float ySortX = rowStart.x + availWidth - 18;
            ImU32 ySortColor = ySort ? IM_COL32(100, 220, 100, 255) : IM_COL32(100, 100, 100, 255);
            ImGui::GetWindowDrawList()->AddText(ImVec2(ySortX, textY), ySortColor, ySort ? "Y" : "-");

            // Check mouse position for click handling
            ImVec2 mousePos = ImGui::GetMousePos();

            // Dummy item for the row
            ImGui::SetCursorScreenPos(rowStart);
            ImGui::InvisibleButton("##row", ImVec2(availWidth, rowHeight));

            bool rowClicked = ImGui::IsItemClicked(0);
            bool rowHovered = ImGui::IsItemHovered();

            // Handle clicks based on mouse X position
            if (rowClicked) {
                float relX = mousePos.x - rowStart.x;
                if (relX < 24) {
                    // Clicked on visibility toggle
                    if (auto* layer = meta.GetLayer(layerDef.id)) {
                        layer->visible = !layer->visible;
                        if (ctx.onCacheInvalidated) {
                            ctx.onCacheInvalidated();
                        }
                    }
                } else if (relX > availWidth - 28) {
                    // Clicked on Y-Sort toggle
                    if (auto* layer = meta.GetLayer(layerDef.id)) {
                        layer->ySortEnabled = !layer->ySortEnabled;
                        if (ctx.onCacheInvalidated) {
                            ctx.onCacheInvalidated();
                        }
                    }
                } else {
                    // Clicked on layer name - select it
                    ctx.selectedLayerId = layerDef.id;
                }
            }

            // Tooltips based on hover position
            if (rowHovered) {
                float relX = mousePos.x - rowStart.x;
                if (relX < 24) {
                    ImGui::SetTooltip(visible ? "Hide layer" : "Show layer");
                } else if (relX > availWidth - 28) {
                    ImGui::SetTooltip("Y-Sort: %s\nClick to toggle", ySort ? "ON (sorts with entities)" : "OFF");
                }
            }

            // Drag source for reordering
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("LAYER_REORDER", &layerDef.id, sizeof(uint16_t));
                ImGui::Text("%s", layerDef.name.c_str());
                ImGui::EndDragDropSource();
            }

            // Drop target for reordering
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_REORDER")) {
                    uint16_t draggedId = *(const uint16_t*)payload->Data;
                    if (draggedId != layerDef.id) {
                        LayerDefinition* draggedLayer = meta.GetLayer(draggedId);
                        LayerDefinition* targetLayer = meta.GetLayer(layerDef.id);
                        if (draggedLayer && targetLayer) {
                            std::swap(draggedLayer->sortOrder, targetLayer->sortOrder);
                            if (ctx.onCacheInvalidated) {
                                ctx.onCacheInvalidated();
                            }
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Right-click context menu
            if (ImGui::BeginPopupContextItem("layer_context")) {
                if (ImGui::MenuItem("Rename")) {
                    m_RenamingLayerId = layerDef.id;
                    strncpy(m_RenameBuffer, layerDef.name.c_str(), sizeof(m_RenameBuffer) - 1);
                    m_ShowRenamePopup = true;
                }
                if (ImGui::MenuItem("Delete", nullptr, false, meta.layers.size() > 1)) {
                    m_DeletingLayerId = layerDef.id;
                    m_ShowDeletePopup = true;
                }
                ImGui::EndPopup();
            }

            // Move cursor to next row
            ImGui::SetCursorScreenPos(ImVec2(rowStart.x, rowStart.y + rowHeight + 2));

            ImGui::PopID();
        }

        ImGui::Spacing();

        // Add layer button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        if (ImGui::Button("+ Add Layer", ImVec2(availWidth, 0))) {
            LayerDefinition newLayer;
            newLayer.id = meta.nextLayerId++;
            newLayer.name = "Layer " + std::to_string(newLayer.id);
            int32_t maxSort = 0;
            for (const auto& l : meta.layers) {
                if (l.sortOrder > maxSort) maxSort = l.sortOrder;
            }
            newLayer.sortOrder = maxSort + 10;
            newLayer.ySortEnabled = false;
            meta.layers.push_back(newLayer);
        }
        ImGui::PopStyleColor();
    }

    // Rename popup (modal)
    if (m_ShowRenamePopup) {
        ImGui::OpenPopup("Rename Layer");
        m_ShowRenamePopup = false;
    }
    if (ImGui::BeginPopupModal("Rename Layer", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter new name:");
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        bool submit = ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
        if (submit || ImGui::Button("OK")) {
            if (auto* layer = ctx.tileMap->GetMetadata().GetLayer(m_RenamingLayerId)) {
                layer->name = m_RenameBuffer;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Delete confirmation popup (modal)
    if (m_ShowDeletePopup) {
        ImGui::OpenPopup("Delete Layer");
        m_ShowDeletePopup = false;
    }
    if (ImGui::BeginPopupModal("Delete Layer", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const LayerDefinition* layerToDelete = ctx.tileMap->GetMetadata().GetLayer(m_DeletingLayerId);
        std::string layerName = layerToDelete ? layerToDelete->name : "Unknown";
        ImGui::Text("Delete layer \"%s\"?", layerName.c_str());
        ImGui::Separator();
        if (ImGui::Button("Delete")) {
            auto& layers = ctx.tileMap->GetMetadata().layers;
            auto it = std::find_if(layers.begin(), layers.end(),
                [&](const LayerDefinition& l) { return l.id == m_DeletingLayerId; });
            if (it != layers.end()) {
                layers.erase(it);
                if (ctx.selectedLayerId == m_DeletingLayerId && !layers.empty()) {
                    ctx.selectedLayerId = layers[0].id;
                }
                if (ctx.onCacheInvalidated) {
                    ctx.onCacheInvalidated();
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace MMO
