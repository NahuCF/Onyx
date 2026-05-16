#include "FontAtlas.h"

#include "Source/Graphics/Texture.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

#include <msdfgen.h>
#include <msdfgen-ext.h>

namespace Onyx::UI {

	namespace {

		constexpr uint32_t kAtlasMagic = 0x31544143u; // 'CAT1' little-endian
		// Bump whenever Bake() output changes (glyph layout, blit orientation,
		// plane/UV convention, pixel range). LoadFromFile rejects mismatched
		// versions and forces a rebake, so a code fix can't be masked by a
		// stale on-disk cache. (v1→v2: 131820c Y-flip blit fix.)
		constexpr uint32_t kAtlasVersion = 2;

		struct FileHeader
		{
			uint32_t magic;
			uint32_t version;
			int32_t  atlasW;
			int32_t  atlasH;
			uint32_t glyphCount;
			float	 bakeSize;
			float	 ascender;
			float	 descender;
			float	 lineHeight;
			float	 pixelRange;
		};

		struct FileGlyph
		{
			uint32_t codepoint;
			float u0, v0, u1, v1;
			float planeL, planeT, planeR, planeB;
			float advance;
		};

		// Shelf-pack glyph rectangles (sorted by descending height) into the
		// smallest power-of-two atlas that fits. Returns chosen size + per-glyph
		// origins. If even maxAtlas can't fit, returns false.
		struct ShelfRect
		{
			int w, h;
			int x, y;
			size_t glyphIndex; // index into the caller's glyph list
		};

		bool ShelfPack(std::vector<ShelfRect>& rects, int& outW, int& outH, int padding)
		{
			// Try doubling sizes from 256 up to 4096. Latin at 32 px ~ 250 glyphs
			// fits comfortably in 512×512.
			static const int kSizes[] = {256, 512, 1024, 2048, 4096};

			std::vector<ShelfRect> sorted = rects;
			std::stable_sort(sorted.begin(), sorted.end(),
							 [](const ShelfRect& a, const ShelfRect& b) { return a.h > b.h; });

			for (int size : kSizes)
			{
				std::vector<ShelfRect> placed = sorted;
				int x = 0, shelfY = 0, shelfH = 0;
				bool fit = true;
				for (ShelfRect& r : placed)
				{
					const int rw = r.w + padding;
					const int rh = r.h + padding;
					if (x + rw > size)
					{
						x = 0;
						shelfY += shelfH;
						shelfH = 0;
					}
					if (shelfY + rh > size) { fit = false; break; }
					r.x = x;
					r.y = shelfY;
					x += rw;
					if (rh > shelfH) shelfH = rh;
				}
				if (!fit) continue;
				rects = std::move(placed);
				outW = size;
				outH = size;
				return true;
			}
			return false;
		}

		// Convert one channel of msdfgen's float [0..1] (with 0.5 = boundary)
		// to an unsigned byte. Outside the range gets clamped.
		uint8_t Encode(float v)
		{
			float c = v;
			if (c < 0.0f) c = 0.0f;
			if (c > 1.0f) c = 1.0f;
			return static_cast<uint8_t>(c * 255.0f + 0.5f);
		}

	} // namespace

	std::vector<uint32_t> DefaultLatinCharset()
	{
		std::vector<uint32_t> cs;
		cs.reserve(256);
		// Basic Latin printable: 0x20..0x7E
		for (uint32_t cp = 0x20; cp <= 0x7E; ++cp) cs.push_back(cp);
		// Latin-1 Supplement printable: 0xA0..0xFF
		for (uint32_t cp = 0xA0; cp <= 0xFF; ++cp) cs.push_back(cp);
		// Latin Extended-A: 0x100..0x17F
		for (uint32_t cp = 0x100; cp <= 0x17F; ++cp) cs.push_back(cp);
		return cs;
	}

	FontAtlas::FontAtlas() = default;
	FontAtlas::~FontAtlas() = default;

	const Glyph* FontAtlas::FindGlyph(uint32_t codepoint) const
	{
		auto it = m_Glyphs.find(codepoint);
		return it == m_Glyphs.end() ? nullptr : &it->second;
	}

	bool FontAtlas::LoadOrBake(const std::string& fontPath,
							   const std::string& cachePath,
							   float pxSize,
							   const std::vector<uint32_t>& charset)
	{
		// Try cache first.
		if (LoadFromFile(cachePath))
		{
			UploadToGPU();
			return true;
		}

		// No cache (or load failed): bake from TTF, then save.
		std::cout << "[UI] baking font atlas from " << fontPath << " (size=" << pxSize << ", glyphs=" << charset.size() << ")\n";
		if (!Bake(fontPath, pxSize, charset))
		{
			std::cerr << "[UI] FontAtlas bake failed for " << fontPath << '\n';
			return false;
		}
		if (!SaveToFile(cachePath))
			std::cerr << "[UI] FontAtlas cache save failed for " << cachePath << " — bake will repeat next run\n";

		UploadToGPU();
		return true;
	}

	bool FontAtlas::LoadFromFile(const std::string& cachePath)
	{
		std::ifstream f(cachePath, std::ios::binary);
		if (!f.is_open())
			return false;

		FileHeader h{};
		f.read(reinterpret_cast<char*>(&h), sizeof(h));
		if (!f) return false;
		if (h.magic != kAtlasMagic || h.version != kAtlasVersion)
		{
			std::cerr << "[UI] FontAtlas cache " << cachePath
					  << " has bad magic/version — will rebake\n";
			return false;
		}

		m_AtlasWidth = h.atlasW;
		m_AtlasHeight = h.atlasH;
		m_Metrics.bakeSize = h.bakeSize;
		m_Metrics.ascender = h.ascender;
		m_Metrics.descender = h.descender;
		m_Metrics.lineHeight = h.lineHeight;
		m_Metrics.pixelRange = h.pixelRange;

		m_Glyphs.clear();
		m_Glyphs.reserve(h.glyphCount);
		for (uint32_t i = 0; i < h.glyphCount; ++i)
		{
			FileGlyph fg{};
			f.read(reinterpret_cast<char*>(&fg), sizeof(fg));
			if (!f) return false;
			Glyph g;
			g.codepoint = fg.codepoint;
			g.uvMin = {fg.u0, fg.v0};
			g.uvMax = {fg.u1, fg.v1};
			g.planeMin = {fg.planeL, fg.planeT};
			g.planeMax = {fg.planeR, fg.planeB};
			g.advance = fg.advance;
			m_Glyphs.emplace(fg.codepoint, g);
		}

		const size_t pixelBytes = static_cast<size_t>(h.atlasW) * h.atlasH * 3;
		m_Pixels.resize(pixelBytes);
		f.read(reinterpret_cast<char*>(m_Pixels.data()), pixelBytes);
		if (!f)
		{
			m_Pixels.clear();
			return false;
		}
		return true;
	}

	bool FontAtlas::SaveToFile(const std::string& cachePath)
	{
		std::ofstream f(cachePath, std::ios::binary | std::ios::trunc);
		if (!f.is_open()) return false;

		FileHeader h{};
		h.magic = kAtlasMagic;
		h.version = kAtlasVersion;
		h.atlasW = m_AtlasWidth;
		h.atlasH = m_AtlasHeight;
		h.glyphCount = static_cast<uint32_t>(m_Glyphs.size());
		h.bakeSize = m_Metrics.bakeSize;
		h.ascender = m_Metrics.ascender;
		h.descender = m_Metrics.descender;
		h.lineHeight = m_Metrics.lineHeight;
		h.pixelRange = m_Metrics.pixelRange;
		f.write(reinterpret_cast<const char*>(&h), sizeof(h));

		for (const auto& [cp, g] : m_Glyphs)
		{
			FileGlyph fg{};
			fg.codepoint = g.codepoint;
			fg.u0 = g.uvMin.x; fg.v0 = g.uvMin.y;
			fg.u1 = g.uvMax.x; fg.v1 = g.uvMax.y;
			fg.planeL = g.planeMin.x; fg.planeT = g.planeMin.y;
			fg.planeR = g.planeMax.x; fg.planeB = g.planeMax.y;
			fg.advance = g.advance;
			f.write(reinterpret_cast<const char*>(&fg), sizeof(fg));
		}

		f.write(reinterpret_cast<const char*>(m_Pixels.data()),
				static_cast<std::streamsize>(m_Pixels.size()));
		return f.good();
	}

	bool FontAtlas::Bake(const std::string& fontPath, float pxSize,
						 const std::vector<uint32_t>& charset)
	{
		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		if (!ft) return false;

		struct FtScope
		{
			msdfgen::FreetypeHandle* ft;
			~FtScope() { msdfgen::deinitializeFreetype(ft); }
		} ftScope{ft};

		msdfgen::FontHandle* font = msdfgen::loadFont(ft, fontPath.c_str());
		if (!font)
		{
			std::cerr << "[UI] msdfgen::loadFont failed for " << fontPath << '\n';
			return false;
		}
		struct FontScope
		{
			msdfgen::FontHandle* f;
			~FontScope() { msdfgen::destroyFont(f); }
		} fontScope{font};

		msdfgen::FontMetrics fm{};
		if (!msdfgen::getFontMetrics(fm, font, msdfgen::FONT_SCALING_EM_NORMALIZED))
		{
			std::cerr << "[UI] getFontMetrics failed\n";
			return false;
		}

		// One em == pxSize at runtime. We bake glyphs in atlas-pixel space
		// where 1 atlas pixel == 1 pixel at pxSize display.
		const double scale = static_cast<double>(pxSize); // em → pixels
		const double pxRangeBake = 4.0;					   // distance field range in atlas pixels

		m_Metrics.bakeSize = pxSize;
		m_Metrics.ascender = static_cast<float>(fm.ascenderY * scale);
		m_Metrics.descender = static_cast<float>(fm.descenderY * scale);
		m_Metrics.lineHeight = static_cast<float>(fm.lineHeight * scale);
		m_Metrics.pixelRange = static_cast<float>(pxRangeBake);

		// Pass 1: load all shapes + compute bitmap sizes.
		struct GlyphData
		{
			uint32_t codepoint;
			msdfgen::Shape shape;
			double advance;
			double l, b, r, t;	  // shape bounds (em)
			int    bitmapW, bitmapH;
			double scaleX, scaleY;
			double translateX, translateY; // atlas-pixel translation
			float  planeL, planeT, planeR, planeB; // for runtime use
			bool   hasShape;
		};

		std::vector<GlyphData> entries;
		entries.reserve(charset.size());

		for (uint32_t cp : charset)
		{
			GlyphData g{};
			g.codepoint = cp;
			g.advance = 0.0;
			g.hasShape = false;
			if (!msdfgen::loadGlyph(g.shape, font, cp, msdfgen::FONT_SCALING_EM_NORMALIZED, &g.advance))
			{
				// Glyph not in font — skip silently. Runtime renders .notdef.
				continue;
			}
			g.shape.normalize();
			msdfgen::edgeColoringSimple(g.shape, 3.0);

			// Skip empty glyphs (e.g. space) but keep their advance.
			g.l = g.b = 0.0; g.r = g.t = 0.0;
			if (!g.shape.contours.empty() && g.shape.validate())
			{
				g.shape.bound(g.l, g.b, g.r, g.t);
				if (g.r > g.l && g.t > g.b)
				{
					// Pad the shape rect by pxRange/scale (in em units) so the
					// distance field has room.
					const double pad = pxRangeBake / scale;
					g.l -= pad;
					g.b -= pad;
					g.r += pad;
					g.t += pad;

					g.bitmapW = std::max(1, static_cast<int>(std::ceil((g.r - g.l) * scale)));
					g.bitmapH = std::max(1, static_cast<int>(std::ceil((g.t - g.b) * scale)));
					g.scaleX = scale;
					g.scaleY = scale;
					g.translateX = -g.l;
					g.translateY = -g.b;
					g.planeL = static_cast<float>(g.l * scale);
					g.planeB = static_cast<float>(g.b * scale);
					g.planeR = static_cast<float>(g.r * scale);
					g.planeT = static_cast<float>(g.t * scale);
					g.hasShape = true;
				}
			}
			// Record advance for both visible and whitespace glyphs.
			entries.push_back(std::move(g));
			entries.back().advance = static_cast<float>(g.advance * scale);
		}

		// Pass 2: pack visible glyphs into atlas.
		std::vector<ShelfRect> rects;
		rects.reserve(entries.size());
		std::vector<size_t> rectToEntry; // rect index -> entry index
		for (size_t i = 0; i < entries.size(); ++i)
		{
			const GlyphData& g = entries[i];
			if (!g.hasShape) continue;
			ShelfRect r;
			r.w = g.bitmapW;
			r.h = g.bitmapH;
			r.x = r.y = 0;
			r.glyphIndex = rectToEntry.size();
			rectToEntry.push_back(i);
			rects.push_back(r);
		}

		const int padding = 2;
		if (!ShelfPack(rects, m_AtlasWidth, m_AtlasHeight, padding))
		{
			std::cerr << "[UI] FontAtlas shelf pack failed (atlas too large)\n";
			return false;
		}

		// Allocate atlas pixels (RGB, init to neutral 0x80 = "outside" SDF).
		m_Pixels.assign(static_cast<size_t>(m_AtlasWidth) * m_AtlasHeight * 3, 0x00);

		// Pass 3: generate MSDF for each visible glyph + blit into atlas.
		for (const ShelfRect& r : rects)
		{
			const size_t entryIdx = rectToEntry[r.glyphIndex];
			GlyphData& g = entries[entryIdx];

			msdfgen::Bitmap<float, 3> msdf(g.bitmapW, g.bitmapH);
			msdfgen::SDFTransformation tx(
				msdfgen::Projection(msdfgen::Vector2(g.scaleX, g.scaleY),
									msdfgen::Vector2(g.translateX, g.translateY)),
				msdfgen::Range(pxRangeBake / g.scaleX));
			msdfgen::generateMSDF(msdf, g.shape, tx);

			// Blit msdf bitmap into m_Pixels at (r.x, r.y).
			// msdfgen's bitmap is Y-up (row 0 = shape bottom); our atlas is
			// Y-down (row 0 = top). Flip the source row index so the runtime,
			// which samples in Y-down convention, sees glyphs upright.
			for (int y = 0; y < g.bitmapH; ++y)
			{
				const int srcY = g.bitmapH - 1 - y;
				for (int x = 0; x < g.bitmapW; ++x)
				{
					const float* px = msdf(x, srcY);
					const int dstX = r.x + x;
					const int dstY = r.y + y;
					const size_t off = (static_cast<size_t>(dstY) * m_AtlasWidth + dstX) * 3;
					m_Pixels[off + 0] = Encode(px[0]);
					m_Pixels[off + 1] = Encode(px[1]);
					m_Pixels[off + 2] = Encode(px[2]);
				}
			}

			Glyph out;
			out.codepoint = g.codepoint;
			out.uvMin = glm::vec2(static_cast<float>(r.x) / m_AtlasWidth,
								  static_cast<float>(r.y) / m_AtlasHeight);
			out.uvMax = glm::vec2(static_cast<float>(r.x + g.bitmapW) / m_AtlasWidth,
								  static_cast<float>(r.y + g.bitmapH) / m_AtlasHeight);
			// Plane: top-left origin pixel offset from baseline pen.
			// msdfgen's Y is positive up; flip into top-left-origin pixels for
			// the renderer (which uses Y-down screen space).
			out.planeMin = glm::vec2(g.planeL, -g.planeT); // top
			out.planeMax = glm::vec2(g.planeR, -g.planeB); // bottom
			out.advance = g.advance;
			m_Glyphs.emplace(g.codepoint, out);
		}

		// Whitespace glyphs (space, tab, etc.) without shapes: keep advance only.
		for (const GlyphData& g : entries)
		{
			if (g.hasShape) continue;
			if (m_Glyphs.count(g.codepoint)) continue;
			Glyph out;
			out.codepoint = g.codepoint;
			out.advance = g.advance;
			m_Glyphs.emplace(g.codepoint, out);
		}

		std::cout << "[UI] baked " << m_Glyphs.size() << " glyphs into "
				  << m_AtlasWidth << "x" << m_AtlasHeight << " atlas\n";
		return true;
	}

	void FontAtlas::UploadToGPU()
	{
		if (m_Pixels.empty() || m_AtlasWidth <= 0 || m_AtlasHeight <= 0)
			return;
		// 3-channel RGB MSDF data. Texture::CreateFromData with channels=3
		// uploads as GL_RGB; the shader samples .rgb and runs median().
		m_Texture = Onyx::Texture::CreateFromData(m_Pixels.data(),
												  m_AtlasWidth, m_AtlasHeight, 3);
	}

} // namespace Onyx::UI
