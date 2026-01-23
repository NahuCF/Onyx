#include "HierarchyPanel.h"
#include "World/EditorWorld.h"
#include "World/WorldTypes.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>

namespace MMO {

void HierarchyPanel::OnImGuiRender() {
    // Hazel-style window padding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Hierarchy");
    ImGui::PopStyleVar();  // WindowPadding

    if (!m_World) {
        ImGui::Text("No world loaded");
        ImGui::End();
        return;
    }

    // Search filter with some padding
    ImGui::SetCursorPosX(4.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 8.0f);
    ImGui::InputTextWithHint("##Search", "Search...", &m_SearchFilter[0], 256);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

    // Context menu for empty space
    if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        RenderContextMenu();
        ImGui::EndPopup();
    }

    // Hazel-style tree styling - tight spacing, minimal padding
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 11.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 0.0f));

    // Get root objects in display order
    std::vector<WorldObject*> rootObjects = m_World->GetRootObjects();

    // Render root-level objects with drop zones between them
    size_t rootIndex = 0;
    for (WorldObject* obj : rootObjects) {
        // Apply search filter
        if (!m_SearchFilter.empty() && obj->GetName().find(m_SearchFilter) == std::string::npos) {
            rootIndex++;
            continue;
        }

        // Drop zone BEFORE this item
        RenderDropZoneBetweenItems(rootIndex, nullptr);

        // Render the item
        if (obj->GetObjectType() == WorldObjectType::GROUP) {
            RenderGroupNode(static_cast<GroupObject*>(obj), rootIndex);
        } else {
            RenderObjectNode(obj, rootIndex, nullptr);
        }

        rootIndex++;
    }

    // Final drop zone at the end (after all items)
    RenderDropZoneBetweenItems(rootIndex, nullptr);

    // Pop style vars (4 vars: ItemSpacing, IndentSpacing, FramePadding, CellPadding)
    ImGui::PopStyleVar(4);

    // Extra drop zone filling remaining space
    ImVec2 availSpace = ImGui::GetContentRegionAvail();
    if (availSpace.y < 30) availSpace.y = 30;
    ImGui::InvisibleButton("##DropZoneEnd", availSpace);

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("WORLD_OBJECT")) {
            uint64_t droppedGuid = *static_cast<const uint64_t*>(payload->Data);
            WorldObject* droppedObj = m_World->GetObject(droppedGuid);
            if (droppedObj) {
                if (droppedObj->HasParent()) {
                    // Unparent at the end
                    m_World->UnparentAtIndex(droppedObj, m_World->GetRootDisplayOrder().size());
                } else {
                    // Move to end
                    m_World->MoveInDisplayOrder(droppedGuid, m_World->GetRootDisplayOrder().size());
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void HierarchyPanel::RenderObjectNode(WorldObject* object, size_t indexInParent, GroupObject* parent) {
    if (!object) return;

    bool isRenaming = (m_RenamingGuid == object->GetGuid());

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (object->IsSelected()) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    // Icon based on type
    const char* icon = "";
    switch (object->GetObjectType()) {
        case WorldObjectType::STATIC_OBJECT: icon = "[M] "; break;
        case WorldObjectType::SPAWN_POINT: icon = "[S] "; break;
        case WorldObjectType::LIGHT: icon = "[L] "; break;
        case WorldObjectType::PARTICLE_EMITTER: icon = "[P] "; break;
        case WorldObjectType::TRIGGER_VOLUME: icon = "[T] "; break;
        case WorldObjectType::INSTANCE_PORTAL: icon = "[I] "; break;
        default: break;
    }

    // Locked/hidden indicators
    std::string label = icon + object->GetName();
    if (object->IsLocked()) label += " (Locked)";
    if (!object->IsVisible()) label += " (Hidden)";

    if (isRenaming) {
        // Show rename input instead of tree node
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (m_FocusRenameInput) {
            ImGui::SetKeyboardFocusHere();
            m_FocusRenameInput = false;
        }
        if (ImGui::InputText("##Rename", m_RenameBuffer, sizeof(m_RenameBuffer),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            FinishRename();
        }
        // Cancel on Escape or click outside
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            (!ImGui::IsItemFocused() && ImGui::IsMouseClicked(0))) {
            m_RenamingGuid = 0;
        }
    } else {
        ImGui::TreeNodeEx(reinterpret_cast<void*>(object->GetGuid()), flags, "%s", label.c_str());

        // Double-click to rename
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            StartRename(object);
        }

        // Click to select
        if (ImGui::IsItemClicked()) {
            bool addToSelection = ImGui::GetIO().KeyShift;
            m_World->Select(object, addToSelection);
        }

        // Context menu for object
        if (ImGui::BeginPopupContextItem()) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

            if (ImGui::MenuItem("Rename", "F2")) {
                StartRename(object);
            }

            ImGui::Separator();

            bool hasMultipleSelected = m_World->GetSelectionCount() > 1;
            bool isGroup = object->GetObjectType() == WorldObjectType::GROUP;

            if (ImGui::MenuItem("Group", nullptr, false, hasMultipleSelected)) {
                m_World->GroupSelected();
            }
            if (ImGui::MenuItem("Ungroup", nullptr, false, isGroup)) {
                m_World->UngroupSelected();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Duplicate")) {
                m_World->Select(object);
                m_World->Duplicate();
            }
            if (ImGui::MenuItem("Delete", "Del")) {
                m_World->DeleteObject(object->GetGuid());
            }

            ImGui::Separator();

            bool locked = object->IsLocked();
            if (ImGui::MenuItem(locked ? "Unlock" : "Lock")) {
                object->SetLocked(!locked);
            }
            bool visible = object->IsVisible();
            if (ImGui::MenuItem(visible ? "Hide" : "Show")) {
                object->SetVisible(!visible);
            }

            ImGui::PopStyleVar();
            ImGui::EndPopup();
        }

        // Drag source - allow dragging this object
        if (ImGui::BeginDragDropSource()) {
            uint64_t guid = object->GetGuid();
            ImGui::SetDragDropPayload("WORLD_OBJECT", &guid, sizeof(guid));
            ImGui::Text("%s", object->GetName().c_str());
            ImGui::EndDragDropSource();
        }
    }
}

void HierarchyPanel::RenderGroupNode(GroupObject* group, size_t indexInParent) {
    if (!group) return;

    bool isRenaming = (m_RenamingGuid == group->GetGuid());

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    bool isSelected = m_World->IsSelected(group);
    if (isSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    // Groups with no children show as leaf nodes
    if (group->IsEmpty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    // Group icon prefix
    std::string label = "[G] " + group->GetName();

    bool isOpen = false;

    if (isRenaming) {
        // Show rename input instead of tree node
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (m_FocusRenameInput) {
            ImGui::SetKeyboardFocusHere();
            m_FocusRenameInput = false;
        }
        if (ImGui::InputText("##RenameGroup", m_RenameBuffer, sizeof(m_RenameBuffer),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            FinishRename();
        }
        // Cancel on Escape or click outside
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            (!ImGui::IsItemFocused() && ImGui::IsMouseClicked(0))) {
            m_RenamingGuid = 0;
        }
    } else {
        isOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(group->GetGuid()), flags, "%s", label.c_str());

        // Double-click to rename
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            StartRename(group);
        }

        // Selection handling
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            bool addToSelection = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift;
            m_World->Select(group, addToSelection);
        }

        // Context menu for group
        if (ImGui::BeginPopupContextItem()) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

            if (ImGui::MenuItem("Rename", "F2")) {
                StartRename(group);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Ungroup")) {
                m_World->Select(group);
                m_World->UngroupSelected();
            }
            if (ImGui::MenuItem("Delete", "Del")) {
                m_World->DeleteObject(group->GetGuid());
            }

            ImGui::Separator();

            bool locked = group->IsLocked();
            if (ImGui::MenuItem(locked ? "Unlock" : "Lock")) {
                group->SetLocked(!locked);
            }
            bool visible = group->IsVisible();
            if (ImGui::MenuItem(visible ? "Hide" : "Show")) {
                group->SetVisible(!visible);
            }

            ImGui::PopStyleVar();
            ImGui::EndPopup();
        }

        // Drag source - groups can be dragged too
        if (ImGui::BeginDragDropSource()) {
            uint64_t guid = group->GetGuid();
            ImGui::SetDragDropPayload("WORLD_OBJECT", &guid, sizeof(guid));
            ImGui::Text("%s", group->GetName().c_str());
            ImGui::EndDragDropSource();
        }

        // Drop target - accept objects to add as children (at the end)
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("WORLD_OBJECT")) {
                uint64_t droppedGuid = *static_cast<const uint64_t*>(payload->Data);
                WorldObject* droppedObj = m_World->GetObject(droppedGuid);
                if (droppedObj && droppedObj != group) {
                    // Check for cycle
                    bool wouldCreateCycle = false;
                    if (droppedObj->GetObjectType() == WorldObjectType::GROUP) {
                        WorldObject* current = group;
                        while (current && current->HasParent()) {
                            if (current->GetParentGuid() == droppedGuid) {
                                wouldCreateCycle = true;
                                break;
                            }
                            current = m_World->GetObject(current->GetParentGuid());
                        }
                    }
                    if (!wouldCreateCycle) {
                        m_World->SetParentAtIndex(droppedObj, group, group->GetChildCount());
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    // Render children if open
    if (isOpen) {
        const auto& children = group->GetChildren();
        size_t childIndex = 0;
        for (uint64_t childGuid : children) {
            WorldObject* child = m_World->GetObject(childGuid);
            if (child) {
                // Drop zone before this child
                RenderDropZoneBetweenItems(childIndex, group);

                // Render child
                if (child->GetObjectType() == WorldObjectType::GROUP) {
                    RenderGroupNode(static_cast<GroupObject*>(child), childIndex);
                } else {
                    RenderObjectNode(child, childIndex, group);
                }
            }
            childIndex++;
        }
        // Drop zone after last child
        RenderDropZoneBetweenItems(childIndex, group);

        ImGui::TreePop();
    }
}

void HierarchyPanel::RenderDropZoneBetweenItems(size_t insertIndex, GroupObject* parent) {
    // Create a unique ID for this drop zone
    char id[64];
    if (parent) {
        snprintf(id, sizeof(id), "##DropZone_%llu_%zu", (unsigned long long)parent->GetGuid(), insertIndex);
    } else {
        snprintf(id, sizeof(id), "##DropZone_Root_%zu", insertIndex);
    }

    // Very thin drop zone - only visible during drag
    ImGui::InvisibleButton(id, ImVec2(-1, 3));

    // Visual feedback when hovering with payload
    if (ImGui::BeginDragDropTarget()) {
        // Draw a highlighted line to show where the item will be inserted (Hazel-style blue)
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(min.x, min.y),
            ImVec2(max.x, max.y),
            IM_COL32(39, 185, 242, 100)  // Hazel highlight color with transparency
        );
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(min.x, (min.y + max.y) * 0.5f),
            ImVec2(max.x, (min.y + max.y) * 0.5f),
            IM_COL32(39, 185, 242, 255), 2.0f  // Hazel highlight color
        );

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("WORLD_OBJECT")) {
            uint64_t droppedGuid = *static_cast<const uint64_t*>(payload->Data);
            WorldObject* droppedObj = m_World->GetObject(droppedGuid);
            if (droppedObj) {
                if (parent) {
                    // Dropping into a group at specific position
                    // Check for cycle if dropping a group
                    bool wouldCreateCycle = false;
                    if (droppedObj->GetObjectType() == WorldObjectType::GROUP) {
                        WorldObject* current = parent;
                        while (current) {
                            if (current->GetGuid() == droppedGuid) {
                                wouldCreateCycle = true;
                                break;
                            }
                            if (current->HasParent()) {
                                current = m_World->GetObject(current->GetParentGuid());
                            } else {
                                break;
                            }
                        }
                    }

                    if (!wouldCreateCycle && droppedObj != parent) {
                        if (droppedObj->HasParent() && droppedObj->GetParentGuid() == parent->GetGuid()) {
                            // Already a child of this parent, just reorder
                            m_World->MoveChildInGroup(parent, droppedGuid, insertIndex);
                        } else {
                            // Move to this parent at specific position
                            m_World->SetParentAtIndex(droppedObj, parent, insertIndex);
                        }
                    }
                } else {
                    // Dropping at root level
                    if (droppedObj->HasParent()) {
                        // Unparent and insert at position
                        m_World->UnparentAtIndex(droppedObj, insertIndex);
                    } else {
                        // Already at root, just reorder
                        m_World->MoveInDisplayOrder(droppedGuid, insertIndex);
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void HierarchyPanel::RenderContextMenu() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

    if (ImGui::BeginMenu("Create")) {
        if (ImGui::MenuItem("Static Object")) {
            auto* obj = m_World->CreateStaticObject();
            m_World->Select(obj);
        }
        if (ImGui::MenuItem("Spawn Point")) {
            auto* obj = m_World->CreateSpawnPoint();
            m_World->Select(obj);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Point Light")) {
            auto* obj = m_World->CreateLight(LightType::POINT);
            m_World->Select(obj);
        }
        if (ImGui::MenuItem("Spot Light")) {
            auto* obj = m_World->CreateLight(LightType::SPOT);
            m_World->Select(obj);
        }
        if (ImGui::MenuItem("Directional Light")) {
            auto* obj = m_World->CreateLight(LightType::DIRECTIONAL);
            m_World->Select(obj);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Particle Emitter")) {
            auto* obj = m_World->CreateParticleEmitter();
            m_World->Select(obj);
        }
        if (ImGui::MenuItem("Trigger Volume")) {
            auto* obj = m_World->CreateTriggerVolume();
            m_World->Select(obj);
        }
        if (ImGui::MenuItem("Instance Portal")) {
            auto* obj = m_World->CreateInstancePortal();
            m_World->Select(obj);
        }
        ImGui::EndMenu();
    }

    ImGui::PopStyleVar();
}

void HierarchyPanel::StartRename(WorldObject* object) {
    if (!object) return;

    m_RenamingGuid = object->GetGuid();
    strncpy(m_RenameBuffer, object->GetName().c_str(), sizeof(m_RenameBuffer) - 1);
    m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';
    m_FocusRenameInput = true;
}

void HierarchyPanel::FinishRename() {
    if (m_RenamingGuid == 0) return;

    WorldObject* object = m_World->GetObject(m_RenamingGuid);
    if (object && m_RenameBuffer[0] != '\0') {
        object->SetName(m_RenameBuffer);
    }

    m_RenamingGuid = 0;
}

} // namespace MMO
