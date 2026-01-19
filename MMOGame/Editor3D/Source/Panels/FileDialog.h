#pragma once

#include <string>
#include <initializer_list>

namespace MMO {

// Filter item for file dialogs (same structure as NFD uses)
struct FileDialogFilter {
    const char* Name;  // Display name (e.g., "3D Models")
    const char* Spec;  // Filter spec (e.g., "obj,fbx,gltf")
};

class FileDialog {
public:
    // Initialize NFD (call once at startup)
    static void Init();

    // Shutdown NFD (call at cleanup)
    static void Shutdown();

    // Opens a native file dialog to select a file
    // Returns empty string if cancelled
    static std::string OpenFile(const std::initializer_list<FileDialogFilter>& filters = {});

    // Opens a native folder browser dialog
    // Returns empty string if cancelled
    static std::string OpenFolder(const char* initialDir = nullptr);

    // Opens a save file dialog
    static std::string SaveFile(const std::initializer_list<FileDialogFilter>& filters = {},
                                const char* defaultName = nullptr);
};

} // namespace MMO
