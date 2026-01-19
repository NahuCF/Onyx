#include "AssetBrowserPanel.h"
#include "World/EditorWorld.h"
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <algorithm>
#include <iostream>

namespace MMO {

void AssetBrowserPanel::Init(EditorWorld* world) {
    m_World = world;

    // Default to assets directory
    m_RootDirectory = "MMOGame/assets";
    m_CurrentDirectory = m_RootDirectory;
    RefreshDirectory();
}

void AssetBrowserPanel::OnImGuiRender() {
    ImGui::Begin("Asset Browser");

    // Navigation bar
    if (ImGui::Button("<")) {
        NavigateUp();
    }
    ImGui::SameLine();

    // Breadcrumb path display
    std::string relativePath = m_CurrentDirectory;
    if (m_CurrentDirectory.length() > m_RootDirectory.length()) {
        relativePath = m_CurrentDirectory.substr(m_RootDirectory.length());
        if (!relativePath.empty() && relativePath[0] == '/') {
            relativePath = relativePath.substr(1);
        }
    } else {
        relativePath = "/";
    }
    ImGui::Text("Path: %s", relativePath.c_str());

    ImGui::SameLine(ImGui::GetWindowWidth() - 200);

    // Refresh button
    if (ImGui::Button("Refresh")) {
        RefreshDirectory();
    }

    ImGui::SameLine();

    // Import Model button - opens ImGuiFileDialog
    if (ImGui::Button("Import Model...")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog(
            "ImportModelDlg",
            "Choose a 3D Model",
            ".obj,.fbx,.gltf,.glb,.dae",
            config
        );
    }

    // Handle the file dialog
    if (ImGuiFileDialog::Instance()->Display("ImportModelDlg")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

            // Normalize path
            std::replace(filePath.begin(), filePath.end(), '\\', '/');

            if (m_World) {
                auto* obj = m_World->CreateStaticObject(fileName);
                obj->SetModelPath(filePath);
                m_World->Select(obj);
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Separator();

    // Search bar
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputTextWithHint("##Search", "Search assets...", m_SearchBuffer, sizeof(m_SearchBuffer))) {
        // Filter happens in render loop
    }

    ImGui::Separator();

    // Content area
    float panelWidth = ImGui::GetContentRegionAvail().x;
    float cellSize = m_ThumbnailSize + m_Padding;
    int columnCount = std::max(1, static_cast<int>(panelWidth / cellSize));

    ImGui::Columns(columnCount, nullptr, false);

    std::string searchStr = m_SearchBuffer;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    for (size_t i = 0; i < m_CurrentEntries.size(); i++) {
        const auto& entry = m_CurrentEntries[i];

        // Apply search filter
        if (!searchStr.empty()) {
            std::string lowerName = entry.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (lowerName.find(searchStr) == std::string::npos) {
                continue;
            }
        }

        ImGui::PushID(static_cast<int>(i));

        // Icon/thumbnail button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

        std::string icon = GetAssetIcon(entry);
        ImGui::Button(icon.c_str(), ImVec2(m_ThumbnailSize, m_ThumbnailSize));

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Double-click to open directory
        if (entry.isDirectory && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            NavigateTo(entry.path);
        }

        // Drag source for files (for future drag-drop into scene)
        if (!entry.isDirectory && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("ASSET_PATH", entry.path.c_str(), entry.path.size() + 1);
            ImGui::Text("%s", entry.name.c_str());
            ImGui::EndDragDropSource();
        }

        // Tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", entry.name.c_str());
            if (!entry.isDirectory) {
                ImGui::TextDisabled("%s", entry.extension.c_str());
            }
            ImGui::EndTooltip();
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            if (entry.isDirectory) {
                if (ImGui::MenuItem("Open")) {
                    NavigateTo(entry.path);
                }
            } else {
                if (IsModelFile(entry.extension)) {
                    if (ImGui::MenuItem("Add to Scene")) {
                        auto* obj = m_World->CreateStaticObject(entry.name);
                        obj->SetModelPath(entry.path);
                        m_World->Select(obj);
                    }
                }
            }
            ImGui::EndPopup();
        }

        // Label
        ImGui::TextWrapped("%s", entry.name.c_str());

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    // Empty directory message
    if (m_CurrentEntries.empty()) {
        ImGui::TextDisabled("Empty directory");
    }

    ImGui::End();
}

void AssetBrowserPanel::SetRootDirectory(const std::string& path) {
    m_RootDirectory = path;
    m_CurrentDirectory = path;
    RefreshDirectory();
}

void AssetBrowserPanel::RefreshDirectory() {
    m_CurrentEntries.clear();

    namespace fs = std::filesystem;

    try {
        fs::path dirPath(m_CurrentDirectory);

        if (!fs::exists(dirPath)) {
            std::cerr << "[AssetBrowser] Directory does not exist: " << m_CurrentDirectory << std::endl;
            return;
        }

        for (const auto& entry : fs::directory_iterator(dirPath)) {
            try {
                std::string fileName = entry.path().filename().string();

                // Skip . and ..
                if (fileName == "." || fileName == "..") {
                    continue;
                }

                // Skip hidden files
                if (!fileName.empty() && fileName[0] == '.') {
                    continue;
                }

                // Skip Zone.Identifier and ADS files
                if (fileName.find("Zone.Identifier") != std::string::npos ||
                    fileName.find(':') != std::string::npos) {
                    continue;
                }

                AssetEntry assetEntry;
                assetEntry.name = fileName;
                assetEntry.path = entry.path().generic_string();
                assetEntry.isDirectory = entry.is_directory();

                if (!assetEntry.isDirectory) {
                    assetEntry.extension = entry.path().extension().string();
                }

                m_CurrentEntries.push_back(assetEntry);
            }
            catch (const std::exception& e) {
                std::cerr << "[AssetBrowser] Skipping file: " << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "[AssetBrowser] Skipping file: unknown error" << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[AssetBrowser] Error: " << e.what() << std::endl;
        return;
    }
    catch (...) {
        std::cerr << "[AssetBrowser] Unknown error" << std::endl;
        return;
    }

    // Sort: directories first, then alphabetically
    std::sort(m_CurrentEntries.begin(), m_CurrentEntries.end(),
        [](const AssetEntry& a, const AssetEntry& b) {
            if (a.isDirectory != b.isDirectory) {
                return a.isDirectory > b.isDirectory;
            }
            return a.name < b.name;
        });
}

void AssetBrowserPanel::NavigateTo(const std::string& path) {
    m_CurrentDirectory = path;
    std::replace(m_CurrentDirectory.begin(), m_CurrentDirectory.end(), '\\', '/');
    RefreshDirectory();
}

void AssetBrowserPanel::NavigateUp() {
    if (m_CurrentDirectory == m_RootDirectory) {
        return;
    }

    namespace fs = std::filesystem;
    fs::path current(m_CurrentDirectory);
    fs::path parent = current.parent_path();

    std::string parentStr = parent.generic_string();
    if (parentStr.length() >= m_RootDirectory.length()) {
        m_CurrentDirectory = parentStr;
        RefreshDirectory();
    }
}

std::string AssetBrowserPanel::GetAssetIcon(const AssetEntry& entry) const {
    if (entry.isDirectory) {
        return "[DIR]";
    }

    if (IsModelFile(entry.extension)) {
        return "[3D]";
    }

    if (IsTextureFile(entry.extension)) {
        return "[IMG]";
    }

    if (IsShaderFile(entry.extension)) {
        return "[SHD]";
    }

    return "[?]";
}

bool AssetBrowserPanel::IsModelFile(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae";
}

bool AssetBrowserPanel::IsTextureFile(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp";
}

bool AssetBrowserPanel::IsShaderFile(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".vert" || ext == ".frag" || ext == ".glsl" || ext == ".hlsl";
}

} // namespace MMO
