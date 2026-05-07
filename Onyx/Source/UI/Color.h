#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Onyx::UI {

	// RGBA float color, components 0..1. The renderer converts to premultiplied
	// alpha at submit time (see Q15 — MSDF + premul). Authors work in straight
	// alpha because that's the natural authoring model and what theme JSON uses.
	struct Color
	{
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 1.0f;

		Color() = default;
		constexpr Color(float r_, float g_, float b_, float a_ = 1.0f)
			: r(r_), g(g_), b(b_), a(a_) {}

		// 0xRRGGBBAA from a uint32 hex literal — handy for theme parsing.
		static constexpr Color FromRGBA8(uint32_t rgba)
		{
			return Color{
				static_cast<float>((rgba >> 24) & 0xFF) / 255.0f,
				static_cast<float>((rgba >> 16) & 0xFF) / 255.0f,
				static_cast<float>((rgba >> 8) & 0xFF) / 255.0f,
				static_cast<float>(rgba & 0xFF) / 255.0f};
		}

		// Pack to 0xAABBGGRR for fast equality / hashing.
		uint32_t ToABGR8() const
		{
			auto q = [](float c) -> uint32_t {
				if (c < 0.0f) c = 0.0f;
				if (c > 1.0f) c = 1.0f;
				return static_cast<uint32_t>(c * 255.0f + 0.5f);
			};
			return (q(a) << 24) | (q(b) << 16) | (q(g) << 8) | q(r);
		}

		glm::vec4 ToVec4() const { return {r, g, b, a}; }

		// Premultiplied form for the renderer.
		Color Premultiplied() const { return Color{r * a, g * a, b * a, a}; }

		bool operator==(const Color& o) const noexcept
		{
			return r == o.r && g == o.g && b == o.b && a == o.a;
		}

		// Common constants — semantic tokens live in Theme JSON, these are
		// just pixel-level conveniences.
		static constexpr Color White() { return Color{1, 1, 1, 1}; }
		static constexpr Color Black() { return Color{0, 0, 0, 1}; }
		static constexpr Color Transparent() { return Color{0, 0, 0, 0}; }
	};

} // namespace Onyx::UI
