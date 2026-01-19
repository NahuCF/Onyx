#pragma once

#include "EditorPanel.h"
#include "EditorContext.h"
#include "../Tileset.h"

namespace MMO {

// ============================================================
// TILESET PANEL
// Displays the selected tileset's tiles for selection
// ============================================================

class TilesetPanel : public EditorPanel {
public:
    TilesetPanel();
    ~TilesetPanel() override = default;

    void OnImGuiRender(bool& isOpen) override;

private:
    float m_TileDisplaySize = 40.0f;
};

} // namespace MMO
