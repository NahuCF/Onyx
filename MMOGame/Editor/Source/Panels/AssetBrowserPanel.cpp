#include "AssetBrowserPanel.h"
#include "../Tileset.h"
#include <imgui.h>

namespace MMO {

AssetBrowserPanel::AssetBrowserPanel() {
    m_ID = "asset_browser";
    m_Name = "Asset Browser";
}

void AssetBrowserPanel::OnImGuiRender(bool& isOpen) {
    auto& ctx = EditorContext::Get();

    ImGui::Begin("Asset Browser", &isOpen);

    ImGui::Text("Assets");
    ImGui::Separator();

    // List loaded tilesets
    const auto& tilesets = TilesetManager::Instance().GetTilesets();
    if (tilesets.empty()) {
        ImGui::TextDisabled("No tilesets loaded");
        ImGui::TextDisabled("Use Tileset > Load Tileset...");
    } else {
        for (size_t i = 0; i < tilesets.size(); ++i) {
            if (ImGui::Selectable(tilesets[i]->GetName().c_str(), ctx.selectedTilesetIndex == i + 1)) {
                ctx.selectedTilesetIndex = static_cast<uint16_t>(i + 1);
            }
        }
    }

    ImGui::End();
}

} // namespace MMO
