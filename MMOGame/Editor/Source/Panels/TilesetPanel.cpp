#include "TilesetPanel.h"
#include <imgui.h>

namespace MMO {

TilesetPanel::TilesetPanel() {
    m_ID = "tileset";
    m_Name = "Tileset";
}

void TilesetPanel::OnImGuiRender(bool& isOpen) {
    auto& ctx = EditorContext::Get();

    ImGui::Begin("Tileset", &isOpen);

    if (ctx.selectedTilesetIndex == 0 || TilesetManager::Instance().GetTilesetCount() == 0) {
        ImGui::TextDisabled("Select a tileset from Asset Browser");
        ImGui::End();
        return;
    }

    Tileset* tileset = TilesetManager::Instance().GetTileset(ctx.selectedTilesetIndex);
    if (!tileset) {
        ImGui::TextDisabled("Invalid tileset");
        ImGui::End();
        return;
    }

    ImGui::Text("%s (%dx%d tiles)", tileset->GetName().c_str(),
                tileset->GetTilesPerRow(), tileset->GetTilesPerCol());
    ImGui::Separator();

    // Tile grid
    int tilesPerRow = tileset->GetTilesPerRow();
    int totalTiles = tileset->GetTileCount();

    ImTextureID texId = static_cast<ImTextureID>(tileset->GetTexture()->GetTextureID());

    float texWidth = static_cast<float>(tileset->GetTextureWidth());
    float texHeight = static_cast<float>(tileset->GetTextureHeight());
    float tileW = static_cast<float>(tileset->GetTileWidth());
    float tileH = static_cast<float>(tileset->GetTileHeight());

    ImGui::BeginChild("TileGrid", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

    for (int i = 0; i < totalTiles; ++i) {
        int col = i % tilesPerRow;
        if (col > 0) {
            ImGui::SameLine();
        }

        float u, v;
        tileset->GetTileUV(static_cast<uint16_t>(i), u, v);

        ImVec2 uvMin(u / texWidth, v / texHeight);
        ImVec2 uvMax((u + tileW) / texWidth, (v + tileH) / texHeight);

        ImGui::PushID(i);

        bool isSelected = (ctx.selectedTile.tilesetIndex == ctx.selectedTilesetIndex &&
                          ctx.selectedTile.tileIndex == i);

        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        }

        char buttonId[32];
        snprintf(buttonId, sizeof(buttonId), "##tile%d", i);
        if (ImGui::ImageButton(buttonId, texId, ImVec2(m_TileDisplaySize, m_TileDisplaySize), uvMin, uvMax)) {
            ctx.selectedTile.tilesetIndex = ctx.selectedTilesetIndex;
            ctx.selectedTile.tileIndex = static_cast<uint16_t>(i);
            ctx.currentTool = EditorTool::BRUSH;

            // Notify selection changed
            if (ctx.onSelectionChanged) {
                ctx.onSelectionChanged();
            }
        }

        if (isSelected) {
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
    ImGui::End();
}

} // namespace MMO
