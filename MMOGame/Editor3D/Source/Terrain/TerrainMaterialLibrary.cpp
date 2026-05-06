#include "TerrainMaterialLibrary.h"

#include <Graphics/AssetManager.h>
#include <algorithm>
#include <filesystem>

namespace Editor3D {

	void TerrainMaterialLibrary::Init(const std::string& assetRoot, Onyx::AssetManager* assetManager)
	{
		m_Library.SetAssetRoot(assetRoot);
		m_MaterialsDir = assetRoot + "/materials/terrain";
		m_AssetManager = assetManager;

		std::filesystem::create_directories(m_MaterialsDir);
		ScanAndRegisterMaterials(assetRoot, *m_AssetManager);
		EnsureDefaultMaterials(m_MaterialsDir, *m_AssetManager);
		m_ArraysDirty = true;
	}

	const Onyx::Material* TerrainMaterialLibrary::GetMaterial(const std::string& id) const
	{
		if (!m_AssetManager)
			return nullptr;
		return m_AssetManager->GetMaterial(id);
	}

	Onyx::Material* TerrainMaterialLibrary::GetMaterialMutable(const std::string& id)
	{
		if (!m_AssetManager)
			return nullptr;
		return m_AssetManager->GetMaterial(id);
	}

	std::string TerrainMaterialLibrary::CreateMaterial(const std::string& name, const std::string& directory)
	{
		if (!m_AssetManager)
			return "";

		std::string id = m_AssetManager->GenerateUniqueMaterialId(name);
		std::string dir = directory.empty() ? m_MaterialsDir : directory;

		std::filesystem::create_directories(dir);

		Onyx::Material mat;
		mat.name = name;
		mat.id = id;
		mat.tilingScale = 8.0f;
		mat.normalStrength = 1.0f;
		mat.filePath = dir + "/" + id + ".material";

		Editor3D::SaveMaterial(mat, mat.filePath);
		m_AssetManager->RegisterMaterial(mat);
		m_ArraysDirty = true;
		return id;
	}

	void TerrainMaterialLibrary::SaveMaterial(const std::string& id)
	{
		if (!m_AssetManager)
			return;

		auto* mat = m_AssetManager->GetMaterial(id);
		if (!mat)
			return;

		if (mat->filePath.empty())
			mat->filePath = m_MaterialsDir + "/" + id + ".material";

		Editor3D::SaveMaterial(*mat, mat->filePath);
	}

	void TerrainMaterialLibrary::UpdateMaterial(const std::string& id, const Onyx::Material& mat)
	{
		if (!m_AssetManager)
			return;
		m_AssetManager->RegisterMaterial(mat);
		m_ArraysDirty = true;
	}

	std::vector<std::string> TerrainMaterialLibrary::GetMaterialIds() const
	{
		if (!m_AssetManager)
			return {};
		auto ids = m_AssetManager->GetAllMaterialIds();
		std::sort(ids.begin(), ids.end());
		return ids;
	}

	Onyx::Texture* TerrainMaterialLibrary::GetDiffuseTexture(const std::string& materialId)
	{
		const auto* mat = GetMaterial(materialId);
		if (!mat || mat->albedoPath.empty())
			return m_Library.GetDefaultDiffuse();
		Onyx::Texture* tex = m_Library.LoadOrGetCachedTexture(mat->albedoPath);
		return tex ? tex : m_Library.GetDefaultDiffuse();
	}

	Onyx::Texture* TerrainMaterialLibrary::GetNormalTexture(const std::string& materialId)
	{
		const auto* mat = GetMaterial(materialId);
		if (!mat || mat->normalPath.empty())
			return m_Library.GetDefaultNormal();
		Onyx::Texture* tex = m_Library.LoadOrGetCachedTexture(mat->normalPath);
		return tex ? tex : m_Library.GetDefaultNormal();
	}

	Onyx::Texture* TerrainMaterialLibrary::GetRMATexture(const std::string& materialId)
	{
		const auto* mat = GetMaterial(materialId);
		if (!mat || mat->rmaPath.empty())
			return m_Library.GetDefaultRMA();
		Onyx::Texture* tex = m_Library.LoadOrGetCachedTexture(mat->rmaPath);
		return tex ? tex : m_Library.GetDefaultRMA();
	}

	void TerrainMaterialLibrary::RebuildTextureArrays()
	{
		if (!m_ArraysDirty || !m_AssetManager)
			return;
		m_ArraysDirty = false;

		std::vector<Onyx::Material> snapshot;
		auto ids = m_AssetManager->GetAllMaterialIds();
		snapshot.reserve(ids.size());
		for (const auto& id : ids)
		{
			if (const auto* mat = m_AssetManager->GetMaterial(id))
				snapshot.push_back(*mat);
		}
		m_Library.Build(snapshot);
	}

} // namespace Editor3D
