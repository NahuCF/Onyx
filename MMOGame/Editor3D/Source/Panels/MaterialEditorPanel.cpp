#include "MaterialEditorPanel.h"
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <cstring>
#include <algorithm>

namespace MMO {

void MaterialEditorPanel::OpenMaterial(const std::string& materialId) {
    if (!m_MaterialLibrary) return;

    const auto* mat = m_MaterialLibrary->GetMaterial(materialId);
    if (!mat) return;

    m_CurrentMaterialId = materialId;
    m_EditingCopy = *mat;
    m_HasUnsavedChanges = false;
    m_IsOpen = true;
}

void MaterialEditorPanel::OnImGuiRender() {
    ImGui::Begin("Material Editor", &m_IsOpen);

    if (!m_MaterialLibrary) {
        ImGui::Text("No material library connected");
        ImGui::End();
        HandleFileDialogs();
        return;
    }

    if (m_CurrentMaterialId.empty()) {
        ImGui::TextDisabled("No material selected");
        ImGui::TextDisabled("Double-click a .material or .terrainmat file in the Asset Browser");
        ImGui::End();
        HandleFileDialogs();
        return;
    }

    char nameBuf[128];
    std::strncpy(nameBuf, m_EditingCopy.name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
        m_EditingCopy.name = nameBuf;
        m_HasUnsavedChanges = true;
    }

    ImGui::TextDisabled("ID: %s", m_EditingCopy.id.c_str());

    ImGui::Separator();

    ImGui::Text("Textures");
    RenderTextureSlot("Albedo", m_EditingCopy.albedoPath, 0);
    RenderTextureSlot("Normal", m_EditingCopy.normalPath, 1);
    RenderTextureSlot("RMA", m_EditingCopy.rmaPath, 2);

    ImGui::Separator();

    if (ImGui::SliderFloat("Tiling Scale", &m_EditingCopy.tilingScale, 0.1f, 64.0f, "%.1f")) {
        m_HasUnsavedChanges = true;
    }
    if (ImGui::SliderFloat("Normal Strength", &m_EditingCopy.normalStrength, 0.0f, 2.0f, "%.2f")) {
        m_HasUnsavedChanges = true;
    }

    ImGui::Separator();

    if (m_HasUnsavedChanges) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Save")) {
            m_MaterialLibrary->UpdateMaterial(m_CurrentMaterialId, m_EditingCopy);
            m_MaterialLibrary->SaveMaterial(m_CurrentMaterialId);
            m_HasUnsavedChanges = false;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("Revert")) {
            const auto* mat = m_MaterialLibrary->GetMaterial(m_CurrentMaterialId);
            if (mat) {
                m_EditingCopy = *mat;
                m_HasUnsavedChanges = false;
            }
        }
    }

    ImGui::End();

    HandleFileDialogs();
}

void MaterialEditorPanel::RenderTextureSlot(const char* label, std::string& texturePath, int slotIndex) {
    ImGui::PushID(slotIndex);

    Onyx::Texture* tex = nullptr;
    if (!texturePath.empty() && m_MaterialLibrary) {
        tex = m_MaterialLibrary->LoadOrGetCachedTexture(texturePath);
    }

    ImGui::Text("%s:", label);
    ImGui::SameLine(100);

    if (tex) {
        ImTextureID texId = (ImTextureID)(uintptr_t)(tex->GetTextureID());
        ImGui::Image(texId, ImVec2(48, 48));
    } else {
        ImGui::Button("(none)", ImVec2(48, 48));
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            std::string droppedPath(static_cast<const char*>(payload->Data));
            std::string ext = droppedPath.substr(droppedPath.find_last_of('.'));
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
                texturePath = droppedPath;
                m_HasUnsavedChanges = true;
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Browse...")) {
        m_BrowsingSlot = slotIndex;
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog(
            "MatTextureDlg",
            "Choose Texture",
            ".png,.jpg,.jpeg,.tga,.bmp",
            config
        );
    }

    ImGui::SameLine();
    if (!texturePath.empty()) {
        std::string filename = texturePath;
        size_t lastSlash = filename.find_last_of('/');
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }
        ImGui::Text("%s", filename.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            texturePath.clear();
            m_HasUnsavedChanges = true;
        }
    } else {
        ImGui::TextDisabled("(empty)");
    }

    ImGui::PopID();
}

void MaterialEditorPanel::HandleFileDialogs() {
    if (ImGuiFileDialog::Instance()->Display("MatTextureDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::replace(filePath.begin(), filePath.end(), '\\', '/');

            if (m_BrowsingSlot >= 0 && m_BrowsingSlot <= 2) {
                switch (m_BrowsingSlot) {
                    case 0: m_EditingCopy.albedoPath = filePath; break;
                    case 1: m_EditingCopy.normalPath = filePath; break;
                    case 2: m_EditingCopy.rmaPath = filePath; break;
                }
                m_HasUnsavedChanges = true;
            }
        }
        ImGuiFileDialog::Instance()->Close();
        m_BrowsingSlot = -1;
    }
}

} // namespace MMO
