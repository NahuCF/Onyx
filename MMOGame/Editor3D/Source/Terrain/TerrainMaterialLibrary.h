#pragma once

#include "MaterialSerializer.h"
#include <Graphics/Material.h>
#include <Graphics/TerrainMaterialLibrary.h>
#include <Graphics/Texture.h>
#include <Graphics/TextureArray.h>
#include <string>
#include <vector>

namespace Onyx {
	class AssetManager;
}

namespace Editor3D {

	// Editor authoring shim around Onyx::TerrainMaterialLibrary. Owns the
	// AssetManager wiring, file authoring (Create/Save/Update), the
	// asset-relative texture cache used by panel UI, and a dirty bit so the
	// viewport rebuilds GPU arrays only when something actually changed.
	//
	// Pure runtime concerns — texture arrays, per-id array index, the
	// __solid_R_G_B parser, default fallback textures — live in the engine
	// library this composes.
	class TerrainMaterialLibrary
	{
	public:
		void Init(const std::string& assetRoot, Onyx::AssetManager* assetManager);

		const Onyx::Material* GetMaterial(const std::string& id) const;
		Onyx::Material* GetMaterialMutable(const std::string& id);
		std::string CreateMaterial(const std::string& name, const std::string& directory = "");
		void SaveMaterial(const std::string& id);
		void UpdateMaterial(const std::string& id, const Onyx::Material& mat);
		std::vector<std::string> GetMaterialIds() const;

		Onyx::Texture* GetDiffuseTexture(const std::string& materialId);
		Onyx::Texture* GetNormalTexture(const std::string& materialId);
		Onyx::Texture* GetRMATexture(const std::string& materialId);

		Onyx::Texture* GetDefaultDiffuse() { return m_Library.GetDefaultDiffuse(); }
		Onyx::Texture* GetDefaultNormal() { return m_Library.GetDefaultNormal(); }
		Onyx::Texture* GetDefaultRMA() { return m_Library.GetDefaultRMA(); }

		const std::string& GetAssetRoot() const { return m_Library.GetAssetRoot(); }

		Onyx::Texture* LoadOrGetCachedTexture(const std::string& path)
		{
			return m_Library.LoadOrGetCachedTexture(path);
		}

		// Rebuild the engine library's texture arrays from the current
		// AssetManager material list. The viewport calls this on dirty.
		void RebuildTextureArrays();
		int GetMaterialArrayIndex(const std::string& materialId) const
		{
			return m_Library.GetMaterialArrayIndex(materialId);
		}
		Onyx::TextureArray* GetDiffuseArray() { return m_Library.GetDiffuseArray(); }
		Onyx::TextureArray* GetNormalArray() { return m_Library.GetNormalArray(); }
		Onyx::TextureArray* GetRMAArray() { return m_Library.GetRMAArray(); }
		bool IsArraysDirty() const { return m_ArraysDirty; }
		void MarkArraysDirty() { m_ArraysDirty = true; }

	private:
		Onyx::TerrainMaterialLibrary m_Library;

		Onyx::AssetManager* m_AssetManager = nullptr;
		std::string m_MaterialsDir;
		bool m_ArraysDirty = true;
	};

} // namespace Editor3D
