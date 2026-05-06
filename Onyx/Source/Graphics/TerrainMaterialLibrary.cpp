#include "pch.h"

#include "TerrainMaterialLibrary.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace Onyx {

	namespace {

		// Texture array layer dimensions. All terrain layers are resampled to
		// this size so they can share a single sampler2DArray. 1024 trades
		// VRAM for detail; matches the editor's previous default.
		constexpr int LAYER_WIDTH = 1024;
		constexpr int LAYER_HEIGHT = 1024;

	} // namespace

	bool TerrainMaterialLibrary::ParseSolidColor(const std::string& path,
												 uint8_t& r, uint8_t& g, uint8_t& b)
	{
		if (path.rfind("__solid_", 0) != 0)
			return false;
		std::string rgb = path.substr(8);
		r = g = b = 128;
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

	void TerrainMaterialLibrary::EnsureDefaults()
	{
		if (!m_DefaultDiffuse)
			m_DefaultDiffuse = Texture::CreateSolidColor(255, 255, 255);
		if (!m_DefaultNormal)
			m_DefaultNormal = Texture::CreateSolidColor(128, 128, 255);
		if (!m_DefaultRMA)
			m_DefaultRMA = Texture::CreateSolidColor(128, 0, 255);
	}

	Texture* TerrainMaterialLibrary::GetDefaultDiffuse() { EnsureDefaults(); return m_DefaultDiffuse.get(); }
	Texture* TerrainMaterialLibrary::GetDefaultNormal()  { EnsureDefaults(); return m_DefaultNormal.get();  }
	Texture* TerrainMaterialLibrary::GetDefaultRMA()     { EnsureDefaults(); return m_DefaultRMA.get();     }

	const Material* TerrainMaterialLibrary::GetMaterial(const std::string& id) const
	{
		auto it = m_Materials.find(id);
		return (it != m_Materials.end()) ? &it->second : nullptr;
	}

	int TerrainMaterialLibrary::GetMaterialArrayIndex(const std::string& id) const
	{
		if (id.empty())
			return 0;
		auto it = m_MaterialArrayIndex.find(id);
		return (it != m_MaterialArrayIndex.end()) ? it->second : 0;
	}

	float TerrainMaterialLibrary::GetTilingScale(const std::string& id) const
	{
		const Material* m = GetMaterial(id);
		return m ? m->tilingScale : 8.0f;
	}

	float TerrainMaterialLibrary::GetNormalStrength(const std::string& id) const
	{
		const Material* m = GetMaterial(id);
		return m ? m->normalStrength : 1.0f;
	}

	Texture* TerrainMaterialLibrary::LoadOrGetCachedTexture(const std::string& path)
	{
		if (path.empty())
			return nullptr;

		auto it = m_TextureCache.find(path);
		if (it != m_TextureCache.end())
			return it->second.get();

		uint8_t r, g, b;
		if (ParseSolidColor(path, r, g, b))
		{
			auto solid = Texture::CreateSolidColor(r, g, b);
			Texture* ptr = solid.get();
			m_TextureCache[path] = std::move(solid);
			return ptr;
		}

		std::string fullPath = path;
		if (!std::filesystem::exists(fullPath) && !m_AssetRoot.empty())
			fullPath = m_AssetRoot + "/" + path;

		if (!std::filesystem::exists(fullPath))
		{
			std::cerr << "[TerrainMaterialLibrary] Texture not found: " << path << '\n';
			m_TextureCache[path] = nullptr;
			return nullptr;
		}

		try
		{
			auto tex = std::make_unique<Texture>(fullPath.c_str());
			Texture* ptr = tex.get();
			m_TextureCache[path] = std::move(tex);
			return ptr;
		}
		catch (...)
		{
			std::cerr << "[TerrainMaterialLibrary] Failed to load texture: " << fullPath << '\n';
			m_TextureCache[path] = nullptr;
			return nullptr;
		}
	}

	void TerrainMaterialLibrary::LoadArrayLayer(TextureArray* array, int layer, const std::string& path,
												uint8_t fallbackR, uint8_t fallbackG, uint8_t fallbackB) const
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
		if (!std::filesystem::exists(fullPath) && !m_AssetRoot.empty())
			fullPath = m_AssetRoot + "/" + path;

		if (std::filesystem::exists(fullPath))
		{
			array->LoadLayerFromFile(fullPath, layer);
		}
		else
		{
			array->SetLayerSolidColor(layer, fallbackR, fallbackG, fallbackB);
		}
	}

	void TerrainMaterialLibrary::Build(const std::vector<Material>& materials)
	{
		m_Materials.clear();
		m_MaterialArrayIndex.clear();

		// Sorted-by-id ordering keeps the per-id array index stable across
		// editor and client runs even if directory iteration order differs.
		std::vector<const Material*> sorted;
		sorted.reserve(materials.size());
		for (const auto& m : materials)
		{
			if (m.id.empty())
				continue;
			m_Materials[m.id] = m;
			sorted.push_back(&m_Materials[m.id]);
		}
		std::sort(sorted.begin(), sorted.end(),
				  [](const Material* a, const Material* b) { return a->id < b->id; });

		const int numMaterials = std::max(1, static_cast<int>(sorted.size()));

		m_DiffuseArray = std::make_unique<TextureArray>();
		m_NormalArray = std::make_unique<TextureArray>();
		m_RMAArray = std::make_unique<TextureArray>();

		m_DiffuseArray->Create(LAYER_WIDTH, LAYER_HEIGHT, numMaterials, 4);
		m_NormalArray->Create(LAYER_WIDTH, LAYER_HEIGHT, numMaterials, 4);
		m_RMAArray->Create(LAYER_WIDTH, LAYER_HEIGHT, numMaterials, 4);

		for (int i = 0; i < static_cast<int>(sorted.size()); i++)
		{
			const Material* mat = sorted[i];
			m_MaterialArrayIndex[mat->id] = i;

			LoadArrayLayer(m_DiffuseArray.get(), i, mat->albedoPath, 255, 255, 255);
			LoadArrayLayer(m_NormalArray.get(), i, mat->normalPath, 128, 128, 255);
			LoadArrayLayer(m_RMAArray.get(), i, mat->rmaPath, 128, 0, 255);
		}
	}

} // namespace Onyx
