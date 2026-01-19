#pragma once

#include "EditorPanel.h"
#include "EditorContext.h"

namespace MMO {

// ============================================================
// ASSET BROWSER PANEL
// Lists loaded tilesets for selection
// ============================================================

class AssetBrowserPanel : public EditorPanel {
public:
    AssetBrowserPanel();
    ~AssetBrowserPanel() override = default;

    void OnImGuiRender(bool& isOpen) override;
};

} // namespace MMO
