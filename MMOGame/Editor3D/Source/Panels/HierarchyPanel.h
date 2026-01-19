#pragma once

#include <string>
#include <cstdint>

namespace MMO {

class EditorWorld;

class HierarchyPanel {
public:
    HierarchyPanel() = default;
    ~HierarchyPanel() = default;

    void Init(EditorWorld* world);
    void OnImGuiRender();

private:
    void RenderObjectNode(class WorldObject* object, size_t indexInParent, class GroupObject* parent);
    void RenderGroupNode(class GroupObject* group, size_t indexInParent);
    void RenderDropZoneBetweenItems(size_t insertIndex, class GroupObject* parent);
    void RenderContextMenu();
    void StartRename(class WorldObject* object);
    void FinishRename();

    EditorWorld* m_World = nullptr;
    std::string m_SearchFilter;

    // Rename state
    uint64_t m_RenamingGuid = 0;
    char m_RenameBuffer[256] = {};
    bool m_FocusRenameInput = false;
};

} // namespace MMO
