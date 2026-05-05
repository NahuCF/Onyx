#pragma once

#include "EditorPanel.h"
#include "Terrain/TerrainMaterialLibrary.h"
#include <Graphics/Material.h>
#include <array>
#include <string>

namespace MMO {

	class MaterialEditorPanel : public EditorPanel
	{
	public:
		MaterialEditorPanel()
		{
			m_Name = "Material Editor";
			m_IsOpen = false;
		}
		~MaterialEditorPanel() override = default;

		void OnImGuiRender() override;

		void SetMaterialLibrary(Editor3D::TerrainMaterialLibrary* lib) { m_MaterialLibrary = lib; }
		void OpenMaterial(const std::string& materialId);

	private:
		Editor3D::TerrainMaterialLibrary* m_MaterialLibrary = nullptr;
		std::string m_CurrentMaterialId;
		Onyx::Material m_EditingCopy;
		bool m_HasUnsavedChanges = false;

		void RenderTextureSlot(const char* label, std::string& texturePath, int slotIndex);
		void HandleFileDialogs();

		// Track which slot's file dialog is currently open (-1 = none)
		int m_BrowsingSlot = -1;
	};

} // namespace MMO
