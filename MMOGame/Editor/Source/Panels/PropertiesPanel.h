#pragma once

#include "EditorPanel.h"
#include "EditorContext.h"

namespace MMO {

// ============================================================
// PROPERTIES PANEL
// Shows properties for map, current tool, and selected tile
// ============================================================

class PropertiesPanel : public EditorPanel {
public:
    PropertiesPanel();
    ~PropertiesPanel() override = default;

    void OnImGuiRender(bool& isOpen) override;

private:
    void RenderMapProperties();
    void RenderToolProperties();
    void RenderSelectedTileInfo();
};

} // namespace MMO
