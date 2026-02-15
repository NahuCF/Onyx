#include "TerrainPanel.h"
#include "ViewportPanel.h"
#include "Terrain/TerrainMaterialLibrary.h"
#include "Terrain/TerrainChunk.h"
#include <imgui.h>
#include <cmath>

namespace MMO {

void TerrainPanel::OnImGuiRender() {
    ImGui::Begin("Terrain", &m_IsOpen);

    if (!m_Viewport) {
        ImGui::Text("No viewport connected");
        ImGui::End();
        return;
    }

    bool enabled = m_Viewport->IsTerrainEnabled();
    if (ImGui::Checkbox("Enable Terrain", &enabled)) {
        m_Viewport->SetTerrainEnabled(enabled);
    }

    if (!enabled) {
        ImGui::End();
        return;
    }

    auto& tool = m_Viewport->m_TerrainTool;

    ImGui::Separator();

    ImGui::Text("Tool Mode");

    auto ToolButton = [&](const char* label, ViewportPanel::TerrainToolMode toolMode) {
        bool selected = tool.toolActive && (tool.mode == toolMode);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
        }
        if (ImGui::Button(label, ImVec2(80, 0))) {
            if (selected) {
                tool.toolActive = false;
            } else {
                tool.toolActive = true;
                tool.mode = toolMode;
                tool.rampPlacing = false;
            }
        }
        if (selected) {
            ImGui::PopStyleColor();
        }
    };

    ImGui::Text("Sculpt:");
    ImGui::SameLine();
    ToolButton("Raise", ViewportPanel::TerrainToolMode::Raise);
    ImGui::SameLine();
    ToolButton("Lower", ViewportPanel::TerrainToolMode::Lower);
    ImGui::SameLine();
    ToolButton("Flatten", ViewportPanel::TerrainToolMode::Flatten);

    ToolButton("Smooth", ViewportPanel::TerrainToolMode::Smooth);
    ImGui::SameLine();
    ToolButton("Ramp", ViewportPanel::TerrainToolMode::Ramp);

    ImGui::Text("Other:");
    ImGui::SameLine();
    ToolButton("Paint", ViewportPanel::TerrainToolMode::Paint);
    ImGui::SameLine();
    ToolButton("Holes", ViewportPanel::TerrainToolMode::Hole);

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Brush Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Radius", &tool.brushRadius, 1.0f, 50.0f, "%.1f");
        ImGui::SliderFloat("Strength", &tool.brushStrength, 0.1f, 20.0f, "%.1f");

        if (tool.mode == ViewportPanel::TerrainToolMode::Flatten) {
            ImGui::Separator();
            ImGui::DragFloat("Target Height", &tool.flattenHeight, 0.1f);
            ImGui::SliderFloat("Hardness", &tool.flattenHardness, 0.0f, 0.95f, "%.2f");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("0 = all gradient, 1 = all flat (wider = smoother transitions)");
            }
            if (tool.brushValid) {
                if (ImGui::Button("Sample Height")) {
                    tool.flattenHeight = tool.brushPos.y;
                }
            }
        }

        if (tool.mode == ViewportPanel::TerrainToolMode::Ramp) {
            ImGui::Separator();
            ImGui::TextDisabled("Width = Brush Radius (%.1f)", tool.brushRadius);

            if (tool.rampPlacing) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Click end point to apply ramp");
                ImGui::Text("Start: (%.1f, %.1f, %.1f)",
                    tool.rampStart.x, tool.rampStart.y, tool.rampStart.z);
                if (ImGui::Button("Cancel")) {
                    tool.rampPlacing = false;
                }
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Click start point on terrain");
            }
        }

        if (tool.mode == ViewportPanel::TerrainToolMode::Paint) {
            ImGui::Separator();
            ImGui::Text("Material Palette:");

            if (m_MaterialLibrary) {
                const auto& ids = m_MaterialLibrary->GetMaterialIds();
                float thumbSize = 48.0f;
                float padding = 4.0f;
                float panelWidth = ImGui::GetContentRegionAvail().x;
                int cols = std::max(1, (int)(panelWidth / (thumbSize + padding)));

                int col = 0;
                for (const auto& matId : ids) {
                    const auto* mat = m_MaterialLibrary->GetMaterial(matId);
                    if (!mat) continue;

                    ImGui::PushID(matId.c_str());

                    bool selected = (tool.selectedMaterialId == matId);

                    if (selected) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    }

                    auto* diffTex = m_MaterialLibrary->GetDiffuseTexture(matId);
                    bool clicked = false;
                    if (diffTex) {
                        ImTextureID texId = (ImTextureID)(uintptr_t)(diffTex->GetTextureID());
                        clicked = ImGui::ImageButton(matId.c_str(), texId,
                            ImVec2(thumbSize, thumbSize));
                    } else {
                        clicked = ImGui::Button(mat->name.c_str(), ImVec2(thumbSize, thumbSize));
                    }

                    if (clicked) {
                        tool.selectedMaterialId = matId;
                    }

                    if (selected) {
                        ImGui::PopStyleColor(2);
                        ImGui::PopStyleVar();
                    }

                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", mat->name.c_str());
                        ImGui::TextDisabled("ID: %s", matId.c_str());
                        ImGui::TextDisabled("Tiling: %.1f", mat->tilingScale);
                        ImGui::EndTooltip();
                    }

                    ImGui::PopID();

                    col++;
                    if (col < cols) {
                        ImGui::SameLine();
                    } else {
                        col = 0;
                    }
                }

                if (col != 0) ImGui::NewLine();
                if (ImGui::Button("+ New Material")) {
                    std::string newId = m_MaterialLibrary->CreateMaterial("New Material");
                    tool.selectedMaterialId = newId;
                }
            }
        }
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::Checkbox("PBR Lighting", &tool.pbr);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enable Cook-Torrance PBR with normal maps and roughness/metallic/AO");
        }
        ImGui::Checkbox("Diamond Grid", &tool.diamondGrid);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("4-triangle diamond tessellation per quad (WoW-style, smoother slopes)");
        }
        ImGui::Checkbox("Pixel Normals", &tool.pixelNormals);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Per-pixel normals from heightmap texture (eliminates triangle edge artifacts)");
        }

        if (!tool.pixelNormals) {
            ImGui::Indent();
            ImGui::Checkbox("Sobel Normals", &tool.sobelNormals);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Use 8-neighbor Sobel filter for smoother vertex normals");
            }
            ImGui::Checkbox("Smooth Normals", &tool.smoothNormals);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Extra averaging pass to further smooth vertex normals");
            }
            ImGui::Unindent();
        }

        static const char* splatDebugLabels[] = { "Off", "Weights (RGBA)", "Weight Sum", "Texel Grid" };
        ImGui::Combo("Debug Splatmap", &tool.debugSplatmap, splatDebugLabels, 4);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("1: RGBA weights as color\n2: Green=sum 1.0, Red=deviation\n3: Texel grid overlay");
        }

        ImGui::Separator();
        static const char* resLabels[] = { "33 (0.5x)", "65 (1x)", "129 (2x)", "257 (4x)" };
        static const int resValues[] = { 33, 65, 129, 257 };
        int currentIdx = 1;
        for (int i = 0; i < 4; i++) {
            if (resValues[i] == tool.meshResolution) { currentIdx = i; break; }
        }
        if (ImGui::Combo("Mesh Resolution", &currentIdx, resLabels, 4)) {
            tool.meshResolution = resValues[currentIdx];
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Vertices per side. Higher = smoother steep slopes.\nData is always 65x65, extra vertices bilinearly interpolate.");
        }
    }

    ImGui::Separator();

    auto& terrainSystem = m_Viewport->GetTerrainSystem();

    if (ImGui::Button("Save Terrain")) {
        terrainSystem.SaveDirtyChunks();
    }

    if (tool.brushValid && tool.toolActive) {
        ImGui::Separator();
        ImGui::Text("Brush: (%.1f, %.1f, %.1f)",
            tool.brushPos.x, tool.brushPos.y, tool.brushPos.z);
    }

    ImGui::End();
}

} // namespace MMO
