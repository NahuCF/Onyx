#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace MMO {

class EditorWorld;

struct AssetEntry {
    std::string name;
    std::string path;
    std::string extension;
    bool isDirectory;
    // TODO: Add thumbnail texture ID
};

class AssetBrowserPanel {
public:
    void Init(EditorWorld* world);
    void OnImGuiRender();

    // Set the root directory for asset browsing
    void SetRootDirectory(const std::string& path);
    const std::string& GetRootDirectory() const { return m_RootDirectory; }

private:
    void RefreshDirectory();
    void NavigateTo(const std::string& path);
    void NavigateUp();

    std::string GetAssetIcon(const AssetEntry& entry) const;
    bool IsModelFile(const std::string& extension) const;
    bool IsTextureFile(const std::string& extension) const;
    bool IsShaderFile(const std::string& extension) const;

    EditorWorld* m_World = nullptr;

    std::string m_RootDirectory;
    std::string m_CurrentDirectory;
    std::vector<AssetEntry> m_CurrentEntries;

    // Settings
    float m_ThumbnailSize = 80.0f;
    float m_Padding = 8.0f;

    // Search filter
    char m_SearchBuffer[256] = "";
};

} // namespace MMO
