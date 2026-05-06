#pragma once

#include "Material.h"
#include "Texture.h"
#include "TextureArray.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Onyx {

	// Runtime library: packs a set of terrain materials' diffuse/normal/RMA
	// textures into three TextureArrays, and exposes per-id array index +
	// per-layer tiling/normal-strength so a terrain shader can sample any
	// material from a single sampler2DArray binding.
	//
	// Pure runtime — no JSON, no AssetManager. The editor and client both feed
	// it a vector<Material>; how they obtain that list is their concern (the
	// editor scans .terrainmat files via AssetManager + MaterialSerializer, the
	// client scans the runtime Data/ dir).
	class TerrainMaterialLibrary
	{
	public:
		// All non-absolute texture paths in materials are resolved relative to
		// this root. Synthetic "__solid_R_G_B" paths are handled in-place and
		// don't touch disk.
		void SetAssetRoot(const std::string& root) { m_AssetRoot = root; }
		const std::string& GetAssetRoot() const { return m_AssetRoot; }

		// Replace materials catalogue and rebuild GPU arrays in one shot.
		// Materials are sorted by id so array indices are stable across runs.
		void Build(const std::vector<Material>& materials);

		const Material* GetMaterial(const std::string& id) const;
		int GetMaterialArrayIndex(const std::string& id) const;
		float GetTilingScale(const std::string& id) const;
		float GetNormalStrength(const std::string& id) const;

		TextureArray* GetDiffuseArray() { return m_DiffuseArray.get(); }
		TextureArray* GetNormalArray() { return m_NormalArray.get(); }
		TextureArray* GetRMAArray() { return m_RMAArray.get(); }

		// Lazy-allocated 1×1 fallbacks. White albedo, neutral (128,128,255)
		// normal, neutral (128,0,255) RMA — same defaults the editor used.
		Texture* GetDefaultDiffuse();
		Texture* GetDefaultNormal();
		Texture* GetDefaultRMA();

		// Resolve "__solid_R_G_B" or a path (absolute or asset-root relative)
		// to a Texture. Cached. Used by editor UI thumbnails.
		Texture* LoadOrGetCachedTexture(const std::string& path);

		// True when "__solid_R_G_B" — fills r/g/b.
		static bool ParseSolidColor(const std::string& path,
									uint8_t& r, uint8_t& g, uint8_t& b);

	private:
		void EnsureDefaults();
		void LoadArrayLayer(TextureArray* array, int layer, const std::string& path,
							uint8_t fallbackR, uint8_t fallbackG, uint8_t fallbackB) const;

		std::string m_AssetRoot;

		std::unordered_map<std::string, Material> m_Materials;
		std::unordered_map<std::string, int> m_MaterialArrayIndex;

		std::unique_ptr<TextureArray> m_DiffuseArray;
		std::unique_ptr<TextureArray> m_NormalArray;
		std::unique_ptr<TextureArray> m_RMAArray;

		std::unique_ptr<Texture> m_DefaultDiffuse;
		std::unique_ptr<Texture> m_DefaultNormal;
		std::unique_ptr<Texture> m_DefaultRMA;

		std::unordered_map<std::string, std::unique_ptr<Texture>> m_TextureCache;
	};

} // namespace Onyx
