#pragma once

#include "EditorPanel.h"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace MMO {

	struct AssetEntry
	{
		std::string name;
		std::string path;
		std::string extension;
		bool isDirectory;
	};

	class AssetBrowserPanel : public EditorPanel
	{
	public:
		AssetBrowserPanel() { m_Name = "Asset Browser"; }
		~AssetBrowserPanel() override = default;

		void OnImGuiRender() override;

		void SetRootDirectory(const std::string& path);
		const std::string& GetRootDirectory() const { return m_RootDirectory; }

		void SetMaterialOpenCallback(std::function<void(const std::string&)> callback)
		{
			m_OnMaterialOpen = std::move(callback);
		}

		void SetMaterialCreateCallback(std::function<void(const std::string&)> callback)
		{
			m_OnMaterialCreate = std::move(callback);
		}

		// Navigate to a specific directory path
		void NavigateToPath(const std::string& path);

	private:
		void RefreshDirectory();
		void NavigateTo(const std::string& path);
		void NavigateUp();

		std::string GetAssetIcon(const AssetEntry& entry) const;
		bool IsModelFile(const std::string& extension) const;
		bool IsTextureFile(const std::string& extension) const;
		bool IsShaderFile(const std::string& extension) const;
		bool IsMaterialFile(const std::string& extension) const;

		std::string m_RootDirectory;
		std::string m_CurrentDirectory;
		std::vector<AssetEntry> m_CurrentEntries;

		std::function<void(const std::string&)> m_OnMaterialOpen;
		std::function<void(const std::string&)> m_OnMaterialCreate;

		// Settings
		float m_ThumbnailSize = 80.0f;
		float m_Padding = 8.0f;

		// Search filter
		char m_SearchBuffer[256] = "";
	};

} // namespace MMO
