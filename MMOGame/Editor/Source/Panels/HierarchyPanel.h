#pragma once

#include "EditorPanel.h"
#include "EditorContext.h"

namespace MMO {

// ============================================================
// HIERARCHY PANEL
// Displays and manages map layers
// ============================================================

class HierarchyPanel : public EditorPanel {
public:
    HierarchyPanel();
    ~HierarchyPanel() override = default;

    void OnImGuiRender(bool& isOpen) override;

private:
    void RenderLayersSection();

    // Rename popup state
    uint16_t m_RenamingLayerId = 0;
    char m_RenameBuffer[256] = {};
    bool m_ShowRenamePopup = false;

    // Delete popup state
    uint16_t m_DeletingLayerId = 0;
    bool m_ShowDeletePopup = false;
};

} // namespace MMO
