#include "FileDialog.h"
#include <imgui.h>
#include <algorithm>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

namespace MMO {

// ============================================================
// IMGUI FILE BROWSER IMPLEMENTATION
// ============================================================

ImGuiFileBrowser::ImGuiFileBrowser() {
    // Start at home directory or current directory
    try {
#ifdef _WIN32
        const char* home = std::getenv("USERPROFILE");
#else
        const char* home = std::getenv("HOME");
#endif
        if (home) {
            m_CurrentPath = fs::path(home);
        } else {
            m_CurrentPath = fs::current_path();
        }
    } catch (...) {
        m_CurrentPath = fs::current_path();
    }
}

void ImGuiFileBrowser::Open(Mode mode, const std::string& title,
                             const std::vector<Filter>& filters) {
    m_Mode = mode;
    m_Title = title;
    m_Filters = filters;
    m_CurrentFilterIndex = 0;
    m_SelectedIndex = -1;
    m_SelectedPath.clear();
    m_FileNameBuffer[0] = '\0';
    m_IsOpen = true;

    // Update path buffer
    std::string pathStr = m_CurrentPath.string();
    strncpy(m_PathBuffer, pathStr.c_str(), sizeof(m_PathBuffer) - 1);
    m_PathBuffer[sizeof(m_PathBuffer) - 1] = '\0';

    RefreshFileList();
}

void ImGuiFileBrowser::Close() {
    m_IsOpen = false;
}

bool ImGuiFileBrowser::Render() {
    if (!m_IsOpen) return false;

    bool result = false;

    ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin(m_Title.c_str(), &m_IsOpen, ImGuiWindowFlags_NoCollapse)) {
        // Path bar
        ImGui::Text("Location:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-100);
        if (ImGui::InputText("##Path", m_PathBuffer, sizeof(m_PathBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            fs::path newPath(m_PathBuffer);
            if (fs::exists(newPath) && fs::is_directory(newPath)) {
                m_CurrentPath = newPath;
                RefreshFileList();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Go")) {
            fs::path newPath(m_PathBuffer);
            if (fs::exists(newPath) && fs::is_directory(newPath)) {
                m_CurrentPath = newPath;
                RefreshFileList();
            }
        }

        // Navigation buttons
        if (ImGui::Button("^")) {
            if (m_CurrentPath.has_parent_path()) {
                m_CurrentPath = m_CurrentPath.parent_path();
                std::string pathStr = m_CurrentPath.string();
                strncpy(m_PathBuffer, pathStr.c_str(), sizeof(m_PathBuffer) - 1);
                RefreshFileList();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Home")) {
#ifdef _WIN32
            const char* home = std::getenv("USERPROFILE");
#else
            const char* home = std::getenv("HOME");
#endif
            if (home) {
                m_CurrentPath = fs::path(home);
                std::string pathStr = m_CurrentPath.string();
                strncpy(m_PathBuffer, pathStr.c_str(), sizeof(m_PathBuffer) - 1);
                RefreshFileList();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            RefreshFileList();
        }

        ImGui::Separator();

        // File list
        float listHeight = ImGui::GetContentRegionAvail().y - 80;
        if (ImGui::BeginChild("FileList", ImVec2(0, listHeight), true)) {
            for (size_t i = 0; i < m_Entries.size(); ++i) {
                const auto& entry = m_Entries[i];
                std::string filename = entry.path().filename().string();
                bool isDir = entry.is_directory();

                // Skip hidden files (starting with .)
                if (!filename.empty() && filename[0] == '.' && filename != "..") {
                    continue;
                }

                // Icon prefix
                std::string label;
                if (isDir) {
                    label = "[DIR] " + filename;
                } else {
                    label = "      " + filename;
                }

                // In folder mode, gray out files (they're shown for context but not selectable)
                bool isFileInFolderMode = (m_Mode == Mode::OPEN_FOLDER && !isDir);
                if (isFileInFolderMode) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                }

                bool isSelected = (m_SelectedIndex == static_cast<int>(i));
                ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;
                if (isFileInFolderMode) {
                    flags |= ImGuiSelectableFlags_Disabled;
                }
                if (ImGui::Selectable(label.c_str(), isSelected, flags)) {
                    m_SelectedIndex = static_cast<int>(i);

                    if (isDir) {
                        // Double-click to enter directory
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            m_CurrentPath = entry.path();
                            std::string pathStr = m_CurrentPath.string();
                            strncpy(m_PathBuffer, pathStr.c_str(), sizeof(m_PathBuffer) - 1);
                            RefreshFileList();
                            m_SelectedIndex = -1;
                        }
                    } else {
                        // Single click selects file
                        strncpy(m_FileNameBuffer, filename.c_str(), sizeof(m_FileNameBuffer) - 1);

                        // Double-click to open file
                        if (ImGui::IsMouseDoubleClicked(0) && m_Mode == Mode::OPEN_FILE) {
                            m_SelectedPath = entry.path().string();
                            result = true;
                            m_IsOpen = false;
                        }
                    }
                }

                if (isFileInFolderMode) {
                    ImGui::PopStyleColor();
                }
            }
        }
        ImGui::EndChild();

        ImGui::Separator();

        // Filename input (for save mode)
        if (m_Mode == Mode::SAVE_FILE) {
            ImGui::Text("File name:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-200);
            ImGui::InputText("##FileName", m_FileNameBuffer, sizeof(m_FileNameBuffer));
            ImGui::SameLine();
        }

        // Filter dropdown
        if (!m_Filters.empty()) {
            ImGui::SetNextItemWidth(150);
            if (ImGui::BeginCombo("##Filter", m_Filters[m_CurrentFilterIndex].name.c_str())) {
                for (size_t i = 0; i < m_Filters.size(); ++i) {
                    bool isSelected = (m_CurrentFilterIndex == static_cast<int>(i));
                    if (ImGui::Selectable(m_Filters[i].name.c_str(), isSelected)) {
                        m_CurrentFilterIndex = static_cast<int>(i);
                        RefreshFileList();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
        }

        // Action buttons
        const char* okLabel = (m_Mode == Mode::SAVE_FILE) ? "Save" :
                              (m_Mode == Mode::OPEN_FOLDER) ? "Select Folder" : "Open";

        if (ImGui::Button(okLabel, ImVec2(80, 0))) {
            if (m_Mode == Mode::OPEN_FOLDER) {
                m_SelectedPath = m_CurrentPath.string();
                result = true;
                m_IsOpen = false;
            } else if (m_SelectedIndex >= 0 && m_SelectedIndex < static_cast<int>(m_Entries.size())) {
                const auto& entry = m_Entries[m_SelectedIndex];
                if (!entry.is_directory()) {
                    m_SelectedPath = entry.path().string();
                    result = true;
                    m_IsOpen = false;
                }
            } else if (m_Mode == Mode::SAVE_FILE && m_FileNameBuffer[0] != '\0') {
                m_SelectedPath = (m_CurrentPath / m_FileNameBuffer).string();
                result = true;
                m_IsOpen = false;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            m_IsOpen = false;
        }
    }
    ImGui::End();

    if (!m_IsOpen && !result) {
        m_SelectedPath.clear();
    }

    return result;
}

void ImGuiFileBrowser::RefreshFileList() {
    m_Entries.clear();
    m_SelectedIndex = -1;

    try {
        if (!fs::exists(m_CurrentPath) || !fs::is_directory(m_CurrentPath)) {
            return;
        }

        // Add parent directory entry
        if (m_CurrentPath.has_parent_path() && m_CurrentPath != m_CurrentPath.root_path()) {
            // We'll handle ".." navigation via the "^" button instead
        }

        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(m_CurrentPath, ec)) {
            try {
                bool isDir = entry.is_directory();

                // For file mode, filter files
                if (!isDir && m_Mode == Mode::OPEN_FILE && !m_Filters.empty()) {
                    if (!MatchesFilter(entry.path().filename().string())) {
                        continue;
                    }
                }

                m_Entries.push_back(entry);
            } catch (...) {
                // Skip entries with encoding issues on Windows
                continue;
            }
        }

        // Sort: directories first, then alphabetically
        std::sort(m_Entries.begin(), m_Entries.end(),
            [](const fs::directory_entry& a, const fs::directory_entry& b) {
                if (a.is_directory() != b.is_directory()) {
                    return a.is_directory();  // Directories first
                }
                return a.path().filename() < b.path().filename();
            });

    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
}

bool ImGuiFileBrowser::MatchesFilter(const std::string& filename) const {
    if (m_Filters.empty() || m_CurrentFilterIndex < 0 ||
        m_CurrentFilterIndex >= static_cast<int>(m_Filters.size())) {
        return true;
    }

    std::string ext = GetFileExtension(filename);
    if (ext.empty()) return false;

    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    const std::string& extensions = m_Filters[m_CurrentFilterIndex].extensions;

    // Parse comma-separated extensions
    size_t start = 0;
    size_t end;
    while ((end = extensions.find(',', start)) != std::string::npos) {
        std::string filterExt = extensions.substr(start, end - start);
        // Trim whitespace
        filterExt.erase(0, filterExt.find_first_not_of(" "));
        filterExt.erase(filterExt.find_last_not_of(" ") + 1);
        std::transform(filterExt.begin(), filterExt.end(), filterExt.begin(), ::tolower);
        if (ext == filterExt) return true;
        start = end + 1;
    }

    // Check last extension
    std::string filterExt = extensions.substr(start);
    filterExt.erase(0, filterExt.find_first_not_of(" "));
    filterExt.erase(filterExt.find_last_not_of(" ") + 1);
    std::transform(filterExt.begin(), filterExt.end(), filterExt.begin(), ::tolower);
    return ext == filterExt;
}

std::string ImGuiFileBrowser::GetFileExtension(const std::string& filename) const {
    size_t dotPos = filename.rfind('.');
    if (dotPos == std::string::npos || dotPos == filename.length() - 1) {
        return "";
    }
    return filename.substr(dotPos + 1);
}

// ============================================================
// FILE DIALOG IMPLEMENTATION
// ============================================================

ImGuiFileBrowser& FileDialog::GetBrowser() {
    static ImGuiFileBrowser browser;
    return browser;
}

} // namespace MMO
