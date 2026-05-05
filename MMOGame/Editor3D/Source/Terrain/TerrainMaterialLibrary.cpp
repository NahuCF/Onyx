#include "TerrainMaterialLibrary.h"
#include <Graphics/AssetManager.h>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace Editor3D {

	static bool ParseSolidColor(const std::string& path, uint8_t& r, uint8_t& g, uint8_t& b)
	{
		if (path.rfind("__solid_", 0) != 0)
			return false;
		std::string rgb = path.substr(8);
		r = 128;
		g = 128;
		b = 128;
		size_t p1 = rgb.find('_');
		if (p1 != std::string::npos)
		{
			r = static_cast<uint8_t>(std::stoi(rgb.substr(0, p1)));
			size_t p2 = rgb.find('_', p1 + 1);
			if (p2 != std::string::npos)
			{
				g = static_cast<uint8_t>(std::stoi(rgb.substr(p1 + 1, p2 - p1 - 1)));
				b = static_cast<uint8_t>(std::stoi(rgb.substr(p2 + 1)));
			}
		}
		return true;
	}

	void TerrainMaterialLibrary::Init(const std::string& assetRoot, Onyx::AssetManager* assetManager)
	{
		m_AssetRoot = assetRoot;
		m_MaterialsDir = assetRoot + "/materials/terrain";
		m_AssetManager = assetManager;

		std::filesystem::create_directories(m_MaterialsDir);
		CreateDefaultTextures();
		ScanAndRegisterMaterials(m_AssetRoot, *m_AssetManager);
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
		{
			mat->filePath = m_MaterialsDir + "/" + id + ".material";
		}

		Editor3D::SaveMaterial(*mat, mat->filePath);
	}

	void TerrainMaterialLibrary::UpdateMaterial(const std::string& id, const Onyx::Material& mat)
	{
		if (!m_AssetManager)
			return;
		m_AssetManager->RegisterMaterial(mat);
		m_ArraysDirty = true;
	}

	Onyx::Texture* TerrainMaterialLibrary::GetMaterialTexture(
		const std::string& materialId,
		std::string Onyx::Material::* pathMember, Onyx::Texture* fallback)
	{
		auto* mat = GetMaterial(materialId);
		if (!mat || (mat->*pathMember).empty())
			return fallback;
		return LoadOrGetCachedTexture(mat->*pathMember);
	}

	Onyx::Texture* TerrainMaterialLibrary::GetDiffuseTexture(const std::string& materialId)
	{
		return GetMaterialTexture(materialId, &Onyx::Material::albedoPath, m_DefaultDiffuse.get());
	}

	Onyx::Texture* TerrainMaterialLibrary::GetNormalTexture(const std::string& materialId)
	{
		return GetMaterialTexture(materialId, &Onyx::Material::normalPath, m_DefaultNormal.get());
	}

	Onyx::Texture* TerrainMaterialLibrary::GetRMATexture(const std::string& materialId)
	{
		return GetMaterialTexture(materialId, &Onyx::Material::rmaPath, m_DefaultRMA.get());
	}

	Onyx::Texture* TerrainMaterialLibrary::LoadOrGetCachedTexture(const std::string& path)
	{
		if (path.empty())
			return nullptr;

		auto it = m_TextureCache.find(path);
		if (it != m_TextureCache.end())
		{
			return it->second.get();
		}

		uint8_t r, g, b;
		if (ParseSolidColor(path, r, g, b))
		{
			m_TextureCache[path] = Onyx::Texture::CreateSolidColor(r, g, b);
			return m_TextureCache[path].get();
		}

		std::string fullPath = path;
		if (!std::filesystem::exists(fullPath))
		{
			fullPath = m_AssetRoot + "/" + path;
		}

		if (!std::filesystem::exists(fullPath))
		{
			std::cerr << "[TerrainMaterialLibrary] Texture not found: " << path << std::endl;
			m_TextureCache[path] = nullptr;
			return nullptr;
		}

		try
		{
			auto texture = std::make_unique<Onyx::Texture>(fullPath.c_str());
			Onyx::Texture* ptr = texture.get();
			m_TextureCache[path] = std::move(texture);
			return ptr;
		}
		catch (...)
		{
			std::cerr << "[TerrainMaterialLibrary] Failed to load texture: " << fullPath << std::endl;
			m_TextureCache[path] = nullptr;
			return nullptr;
		}
	}

	void TerrainMaterialLibrary::CreateDefaultTextures()
	{
		m_DefaultDiffuse = Onyx::Texture::CreateSolidColor(255, 255, 255);
		m_DefaultNormal = Onyx::Texture::CreateSolidColor(128, 128, 255);
		m_DefaultRMA = Onyx::Texture::CreateSolidColor(128, 0, 255);
	}

	std::vector<std::string> TerrainMaterialLibrary::GetMaterialIds() const
	{
		if (!m_AssetManager)
			return {};
		auto ids = m_AssetManager->GetAllMaterialIds();
		std::sort(ids.begin(), ids.end());
		return ids;
	}

	void TerrainMaterialLibrary::LoadArrayLayer(Onyx::TextureArray* array, int layer,
												const std::string& path, uint8_t fallbackR, uint8_t fallbackG, uint8_t fallbackB)
	{
		if (path.empty())
		{
			array->SetLayerSolidColor(layer, fallbackR, fallbackG, fallbackB);
			return;
		}

		uint8_t r, g, b;
		if (ParseSolidColor(path, r, g, b))
		{
			array->SetLayerSolidColor(layer, r, g, b);
			return;
		}

		std::string fullPath = path;
		if (!std::filesystem::exists(fullPath))
			fullPath = m_AssetRoot + "/" + path;

		if (std::filesystem::exists(fullPath))
			array->LoadLayerFromFile(fullPath, layer);
		else
			array->SetLayerSolidColor(layer, fallbackR, fallbackG, fallbackB);
	}

	void TerrainMaterialLibrary::RebuildTextureArrays()
	{
		if (!m_ArraysDirty || !m_AssetManager)
			return;
		m_ArraysDirty = false;

		auto ids = GetMaterialIds();
		int numMaterials = std::max(1, (int)ids.size());
		const int ARRAY_SIZE = 1024;

		m_DiffuseArray = std::make_unique<Onyx::TextureArray>();
		m_NormalArray = std::make_unique<Onyx::TextureArray>();
		m_RMAArray = std::make_unique<Onyx::TextureArray>();

		m_DiffuseArray->Create(ARRAY_SIZE, ARRAY_SIZE, numMaterials, 4);
		m_NormalArray->Create(ARRAY_SIZE, ARRAY_SIZE, numMaterials, 4);
		m_RMAArray->Create(ARRAY_SIZE, ARRAY_SIZE, numMaterials, 4);

		m_MaterialArrayIndex.clear();

		for (int i = 0; i < (int)ids.size(); i++)
		{
			const auto& matId = ids[i];
			m_MaterialArrayIndex[matId] = i;

			const auto* mat = GetMaterial(matId);
			if (!mat)
				continue;

			LoadArrayLayer(m_DiffuseArray.get(), i, mat->albedoPath, 255, 255, 255);
			LoadArrayLayer(m_NormalArray.get(), i, mat->normalPath, 128, 128, 255);
			LoadArrayLayer(m_RMAArray.get(), i, mat->rmaPath, 128, 0, 255);
		}
	}

	int TerrainMaterialLibrary::GetMaterialArrayIndex(const std::string& materialId) const
	{
		if (materialId.empty())
			return 0;
		auto it = m_MaterialArrayIndex.find(materialId);
		if (it != m_MaterialArrayIndex.end())
			return it->second;
		return 0;
	}

} // namespace Editor3D
