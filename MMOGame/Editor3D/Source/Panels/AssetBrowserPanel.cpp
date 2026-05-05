#include "AssetBrowserPanel.h"
#include "World/EditorWorld.h"
#include <ImGuiFileDialog.h>
#include <algorithm>
#include <imgui.h>
#include <iostream>

namespace MMO {

	void AssetBrowserPanel::OnImGuiRender()
	{
		if (m_RootDirectory.empty())
		{
			m_RootDirectory = "MMOGame/Editor3D/assets";
			m_CurrentDirectory = m_RootDirectory;
			RefreshDirectory();
		}

		ImGui::Begin("Asset Browser");

		if (ImGui::Button("<"))
		{
			NavigateUp();
		}
		ImGui::SameLine();

		std::string relativePath = m_CurrentDirectory;
		if (m_CurrentDirectory.length() > m_RootDirectory.length())
		{
			relativePath = m_CurrentDirectory.substr(m_RootDirectory.length());
			if (!relativePath.empty() && relativePath[0] == '/')
			{
				relativePath = relativePath.substr(1);
			}
		}
		else
		{
			relativePath = "/";
		}
		ImGui::Text("Path: %s", relativePath.c_str());

		ImGui::SameLine(ImGui::GetWindowWidth() - 200);

		if (ImGui::Button("Refresh"))
		{
			RefreshDirectory();
		}

		ImGui::SameLine();

		if (ImGui::Button("Import Model..."))
		{
			IGFD::FileDialogConfig config;
			config.path = ".";
			ImGuiFileDialog::Instance()->OpenDialog(
				"ImportModelDlg",
				"Choose a 3D Model",
				".obj,.fbx,.gltf,.glb,.dae",
				config);
		}

		if (ImGuiFileDialog::Instance()->Display("ImportModelDlg"))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();

				std::replace(filePath.begin(), filePath.end(), '\\', '/');

				if (m_World)
				{
					auto* obj = m_World->CreateStaticObject(fileName);
					obj->SetModelPath(filePath);
					m_World->Select(obj);
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}

		ImGui::Separator();

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::InputTextWithHint("##Search", "Search assets...", m_SearchBuffer, sizeof(m_SearchBuffer)))
		{
		}

		ImGui::Separator();

		float panelWidth = ImGui::GetContentRegionAvail().x;
		float cellSize = m_ThumbnailSize + m_Padding;
		int columnCount = std::max(1, static_cast<int>(panelWidth / cellSize));

		ImGui::Columns(columnCount, nullptr, false);

		std::string searchStr = m_SearchBuffer;
		std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

		for (size_t i = 0; i < m_CurrentEntries.size(); i++)
		{
			const auto& entry = m_CurrentEntries[i];

			if (!searchStr.empty())
			{
				std::string lowerName = entry.name;
				std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
				if (lowerName.find(searchStr) == std::string::npos)
				{
					continue;
				}
			}

			ImGui::PushID(static_cast<int>(i));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

			std::string icon = GetAssetIcon(entry);
			ImGui::Button(icon.c_str(), ImVec2(m_ThumbnailSize, m_ThumbnailSize));

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
			{
				if (entry.isDirectory)
				{
					NavigateTo(entry.path);
				}
				else if (IsMaterialFile(entry.extension) && m_OnMaterialOpen)
				{
					m_OnMaterialOpen(entry.path);
				}
			}

			if (!entry.isDirectory && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::SetDragDropPayload("ASSET_PATH", entry.path.c_str(), entry.path.size() + 1);
				ImGui::Text("%s", entry.name.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("%s", entry.name.c_str());
				if (!entry.isDirectory)
				{
					ImGui::TextDisabled("%s", entry.extension.c_str());
				}
				ImGui::EndTooltip();
			}

			if (ImGui::BeginPopupContextItem())
			{
				if (entry.isDirectory)
				{
					if (ImGui::MenuItem("Open"))
					{
						NavigateTo(entry.path);
					}
				}
				else
				{
					if (IsModelFile(entry.extension))
					{
						if (ImGui::MenuItem("Add to Scene"))
						{
							auto* obj = m_World->CreateStaticObject(entry.name);
							obj->SetModelPath(entry.path);
							m_World->Select(obj);
						}
					}
					if (IsMaterialFile(entry.extension) && m_OnMaterialOpen)
					{
						if (ImGui::MenuItem("Edit Material"))
						{
							m_OnMaterialOpen(entry.path);
						}
					}
				}
				ImGui::EndPopup();
			}

			ImGui::TextWrapped("%s", entry.name.c_str());

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);

		if (m_CurrentEntries.empty())
		{
			ImGui::TextDisabled("Empty directory");
		}

		if (ImGui::BeginPopupContextWindow("AssetBrowserContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			if (m_OnMaterialCreate && ImGui::MenuItem("Create Material"))
			{
				m_OnMaterialCreate(m_CurrentDirectory);
				RefreshDirectory();
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void AssetBrowserPanel::SetRootDirectory(const std::string& path)
	{
		m_RootDirectory = path;
		m_CurrentDirectory = path;
		RefreshDirectory();
	}

	void AssetBrowserPanel::RefreshDirectory()
	{
		m_CurrentEntries.clear();

		namespace fs = std::filesystem;

		try
		{
			fs::path dirPath(m_CurrentDirectory);

			if (!fs::exists(dirPath))
			{
				std::cerr << "[AssetBrowser] Directory does not exist: " << m_CurrentDirectory << '\n';
				return;
			}

			for (const auto& entry : fs::directory_iterator(dirPath))
			{
				try
				{
					std::string fileName = entry.path().filename().string();

					if (fileName == "." || fileName == "..")
					{
						continue;
					}

					if (!fileName.empty() && fileName[0] == '.')
					{
						continue;
					}

					if (fileName.find("Zone.Identifier") != std::string::npos ||
						fileName.find(':') != std::string::npos)
					{
						continue;
					}

					AssetEntry assetEntry;
					assetEntry.name = fileName;
					assetEntry.path = entry.path().generic_string();
					assetEntry.isDirectory = entry.is_directory();

					if (!assetEntry.isDirectory)
					{
						assetEntry.extension = entry.path().extension().string();
					}

					m_CurrentEntries.push_back(assetEntry);
				}
				catch (const std::exception& e)
				{
					std::cerr << "[AssetBrowser] Skipping file: " << e.what() << '\n';
				}
				catch (...)
				{
					std::cerr << "[AssetBrowser] Skipping file: unknown error" << '\n';
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "[AssetBrowser] Error: " << e.what() << '\n';
			return;
		}
		catch (...)
		{
			std::cerr << "[AssetBrowser] Unknown error" << '\n';
			return;
		}

		std::sort(m_CurrentEntries.begin(), m_CurrentEntries.end(),
				  [](const AssetEntry& a, const AssetEntry& b) {
					  if (a.isDirectory != b.isDirectory)
					  {
						  return a.isDirectory > b.isDirectory;
					  }
					  return a.name < b.name;
				  });
	}

	void AssetBrowserPanel::NavigateTo(const std::string& path)
	{
		m_CurrentDirectory = path;
		std::replace(m_CurrentDirectory.begin(), m_CurrentDirectory.end(), '\\', '/');
		RefreshDirectory();
	}

	void AssetBrowserPanel::NavigateToPath(const std::string& path)
	{
		namespace fs = std::filesystem;
		if (fs::exists(path) && fs::is_directory(path))
		{
			m_CurrentDirectory = path;
			std::replace(m_CurrentDirectory.begin(), m_CurrentDirectory.end(), '\\', '/');
			RefreshDirectory();
		}
	}

	void AssetBrowserPanel::NavigateUp()
	{
		if (m_CurrentDirectory == m_RootDirectory)
		{
			return;
		}

		namespace fs = std::filesystem;
		fs::path current(m_CurrentDirectory);
		fs::path parent = current.parent_path();

		std::string parentStr = parent.generic_string();
		if (parentStr.length() >= m_RootDirectory.length())
		{
			m_CurrentDirectory = parentStr;
			RefreshDirectory();
		}
	}

	std::string AssetBrowserPanel::GetAssetIcon(const AssetEntry& entry) const
	{
		if (entry.isDirectory)
		{
			return "[DIR]";
		}

		if (IsModelFile(entry.extension))
		{
			return "[3D]";
		}

		if (IsTextureFile(entry.extension))
		{
			return "[IMG]";
		}

		if (IsShaderFile(entry.extension))
		{
			return "[SHD]";
		}

		if (IsMaterialFile(entry.extension))
		{
			return "[MAT]";
		}

		return "[?]";
	}

	bool AssetBrowserPanel::IsModelFile(const std::string& extension) const
	{
		std::string ext = extension;
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		return ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae";
	}

	bool AssetBrowserPanel::IsTextureFile(const std::string& extension) const
	{
		std::string ext = extension;
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp";
	}

	bool AssetBrowserPanel::IsShaderFile(const std::string& extension) const
	{
		std::string ext = extension;
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		return ext == ".vert" || ext == ".frag" || ext == ".glsl" || ext == ".hlsl";
	}

	bool AssetBrowserPanel::IsMaterialFile(const std::string& extension) const
	{
		std::string ext = extension;
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		return ext == ".terrainmat" || ext == ".material";
	}

} // namespace MMO
