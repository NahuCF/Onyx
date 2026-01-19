#include "PropertiesPanel.h"
#include "../Tileset.h"
#include <imgui.h>
#include <cstring>

namespace MMO {

PropertiesPanel::PropertiesPanel() {
    m_ID = "properties";
    m_Name = "Properties";
}

void PropertiesPanel::OnImGuiRender(bool& isOpen) {
    ImGui::Begin("Properties", &isOpen);

    RenderMapProperties();
    RenderToolProperties();
    RenderSelectedTileInfo();

    ImGui::End();
}

void PropertiesPanel::RenderMapProperties() {
    auto& ctx = EditorContext::Get();
    if (!ctx.tileMap) return;

    if (ImGui::CollapsingHeader("Map", ImGuiTreeNodeFlags_DefaultOpen)) {
        MapMetadata meta = ctx.tileMap->GetMetadata();
        bool changed = false;

        char nameBuffer[256];
        strncpy(nameBuffer, meta.name.c_str(), sizeof(nameBuffer) - 1);
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            meta.name = nameBuffer;
            changed = true;
        }

        if (ImGui::InputInt("ID", reinterpret_cast<int*>(&meta.id))) {
            changed = true;
        }

        ImGui::Text("Tile Size: %dx%d", meta.tileWidth, meta.tileHeight);
        ImGui::Text("Chunk Size: %d", meta.chunkSize);

        if (changed) {
            ctx.tileMap->SetMetadata(meta);
        }
    }
}

void PropertiesPanel::RenderToolProperties() {
    auto& ctx = EditorContext::Get();
    if (!ctx.tileMap) return;

    if (ImGui::CollapsingHeader("Tool", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool hasTileset = TilesetManager::Instance().GetTilesetCount() > 0;

        if (!hasTileset) {
            ImGui::TextDisabled("Load a tileset to use painting tools");
            ImGui::TextDisabled("(Tileset > Load Tileset...)");
        } else {
            const char* toolNames[] = { "Select", "Brush (B)", "Fill (G)", "Eraser (E)", "Entity" };
            int currentTool = static_cast<int>(ctx.currentTool);
            if (ImGui::Combo("Active Tool", &currentTool, toolNames, 5)) {
                ctx.currentTool = static_cast<EditorTool>(currentTool);
            }
        }
    }
}

void PropertiesPanel::RenderSelectedTileInfo() {
    auto& ctx = EditorContext::Get();

    if (ImGui::CollapsingHeader("Selected Tile", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ctx.selectedTile.IsEmpty()) {
            ImGui::TextDisabled("No tile selected");
        } else {
            ImGui::Text("Tileset: %d", ctx.selectedTile.tilesetIndex);
            ImGui::Text("Tile: %d", ctx.selectedTile.tileIndex);

            Tileset* tileset = TilesetManager::Instance().GetTileset(ctx.selectedTile.tilesetIndex);
            if (tileset) {
                ImGui::Text("Tileset Name: %s", tileset->GetName().c_str());

                // Show preview
                float u, v;
                tileset->GetTileUV(ctx.selectedTile.tileIndex, u, v);

                ImTextureID texId = static_cast<ImTextureID>(tileset->GetTexture()->GetTextureID());

                float texWidth = static_cast<float>(tileset->GetTextureWidth());
                float texHeight = static_cast<float>(tileset->GetTextureHeight());
                float tileW = static_cast<float>(tileset->GetTileWidth());
                float tileH = static_cast<float>(tileset->GetTileHeight());

                ImVec2 uvMin(u / texWidth, v / texHeight);
                ImVec2 uvMax((u + tileW) / texWidth, (v + tileH) / texHeight);

                ImGui::Image(texId, ImVec2(64, 64), uvMin, uvMax);
            }
        }
    }
}

} // namespace MMO
