#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Onyx {
	class Texture;
}

namespace Onyx::UI {

	// Per-glyph layout + atlas info. Plane bounds are pixel offsets relative to
	// the glyph origin (pen position on the baseline), in atlas-bake pixel space.
	// Atlas UVs are normalized 0..1.
	struct Glyph
	{
		uint32_t codepoint = 0;
		glm::vec2 uvMin{0.0f};
		glm::vec2 uvMax{0.0f};
		glm::vec2 planeMin{0.0f}; // (left, top) — Y up; bake-time pixels
		glm::vec2 planeMax{0.0f}; // (right, bottom)
		float advance = 0.0f;	   // horizontal advance in bake-time pixels
	};

	// Font-wide metrics in bake-time pixels (i.e. relative to the size used to
	// generate the atlas). The runtime scales by (displaySize / bakeSize) at
	// draw time.
	struct FontMetrics
	{
		float bakeSize = 32.0f; // pixel size the atlas was generated at
		float ascender = 0.0f;	// distance from baseline to top of cap (positive)
		float descender = 0.0f; // distance from baseline to bottom (negative)
		float lineHeight = 0.0f;
		float pixelRange = 4.0f; // MSDF distance field range, in atlas pixels
	};

	// Loads a baked MSDF atlas binary, or bakes one on first run from a TTF
	// using msdfgen + FreeType. Bakes are cached to disk so subsequent runs
	// just load the binary.
	//
	// The atlas binary format is `Onyx/Source/UI/Text/atlas-format.md` (in this
	// repo). Single file: header + glyph table + RGB MSDF pixel data.
	class FontAtlas
	{
	public:
		FontAtlas();
		~FontAtlas();

		// Loads the cached atlas at `cachePath` if it exists; otherwise bakes
		// from `fontPath` at `pxSize` using `charset`, writes the cache, and
		// uploads to GPU. Returns false on hard failure (font not found, etc.).
		bool LoadOrBake(const std::string& fontPath,
						const std::string& cachePath,
						float pxSize,
						const std::vector<uint32_t>& charset);

		const Glyph* FindGlyph(uint32_t codepoint) const;
		const FontMetrics& GetMetrics() const { return m_Metrics; }

		Onyx::Texture* GetTexture() const { return m_Texture.get(); }
		int GetAtlasWidth() const { return m_AtlasWidth; }
		int GetAtlasHeight() const { return m_AtlasHeight; }
		bool IsLoaded() const { return m_Texture != nullptr; }

	private:
		bool LoadFromFile(const std::string& cachePath);
		bool SaveToFile(const std::string& cachePath);
		bool Bake(const std::string& fontPath, float pxSize, const std::vector<uint32_t>& charset);
		void UploadToGPU();

		FontMetrics m_Metrics;
		std::unordered_map<uint32_t, Glyph> m_Glyphs;
		std::vector<uint8_t> m_Pixels; // RGB, atlasWidth * atlasHeight * 3
		int m_AtlasWidth = 0;
		int m_AtlasHeight = 0;

		std::unique_ptr<Onyx::Texture> m_Texture;
	};

	// Default Latin charset (Basic Latin + Latin-1 Supplement + Latin Extended-A,
	// minus control characters). Suitable for English / Spanish / French /
	// German / Italian / Portuguese / Polish.
	std::vector<uint32_t> DefaultLatinCharset();

} // namespace Onyx::UI
