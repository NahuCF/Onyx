#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <filesystem>

namespace MMO {

// ============================================================
// IMGUI FILE BROWSER
// Simple ImGui-based file browser (no external dependencies)
// ============================================================

class ImGuiFileBrowser {
public:
    enum class Mode {
        OPEN_FILE,
        OPEN_FOLDER,
        SAVE_FILE
    };

    struct Filter {
        std::string name;        // Display name (e.g., "Image Files")
        std::string extensions;  // Comma-separated extensions (e.g., "png,jpg,gif")
    };

    ImGuiFileBrowser();

    // Open the file browser
    void Open(Mode mode, const std::string& title = "File Browser",
              const std::vector<Filter>& filters = {});

    // Render the file browser (call every frame)
    // Returns true when a selection is made
    bool Render();

    // Get the selected path (valid after Render returns true)
    const std::string& GetSelectedPath() const { return m_SelectedPath; }

    // Check if browser is currently open
    bool IsOpen() const { return m_IsOpen; }

    // Close the browser
    void Close();

private:
    void RefreshFileList();
    bool MatchesFilter(const std::string& filename) const;
    std::string GetFileExtension(const std::string& filename) const;

    bool m_IsOpen = false;
    Mode m_Mode = Mode::OPEN_FILE;
    std::string m_Title;
    std::vector<Filter> m_Filters;
    int m_CurrentFilterIndex = 0;

    std::filesystem::path m_CurrentPath;
    std::vector<std::filesystem::directory_entry> m_Entries;
    int m_SelectedIndex = -1;
    std::string m_SelectedPath;

    char m_FileNameBuffer[256] = "";
    char m_PathBuffer[512] = "";
};

// ============================================================
// FILE DIALOG (Simple wrapper)
// ============================================================

class FileDialog {
public:
    // Initialize (no-op for ImGui version)
    static void Init() {}

    // Cleanup (no-op for ImGui version)
    static void Quit() {}

    // Get the singleton file browser instance
    static ImGuiFileBrowser& GetBrowser();
};

} // namespace MMO
