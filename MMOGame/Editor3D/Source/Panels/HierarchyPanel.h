#pragma once

#include "EditorPanel.h"
#include <string>
#include <cstdint>

namespace MMO {

class HierarchyPanel : public EditorPanel {
public:
    HierarchyPanel() { m_Name = "Hierarchy"; }
    ~HierarchyPanel() override = default;

    void OnImGuiRender() override;

private:
    void RenderObjectNode(class WorldObject* object, size_t indexInParent, class GroupObject* parent);
    void RenderGroupNode(class GroupObject* group, size_t indexInParent);
    void RenderDropZoneBetweenItems(size_t insertIndex, class GroupObject* parent);
    void RenderContextMenu();
    void StartRename(class WorldObject* object);
    void FinishRename();

    std::string m_SearchFilter;

    // Rename state
    uint64_t m_RenamingGuid = 0;
    char m_RenameBuffer[256] = {};
    bool m_FocusRenameInput = false;
};

} // namespace MMO
