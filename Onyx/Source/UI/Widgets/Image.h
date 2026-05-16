#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widget.h"

#include <cstdint>

namespace Onyx { class Texture; }

namespace Onyx::UI {

	// Static textured widget. The proper path for full-bleed art (Phase-4
	// loading screen, splash, portraits) instead of the Label+DebugRect
	// stop-gap. Texture is non-owning — the caller / AssetManager owns it.
	class Image : public Widget
	{
	public:
		enum class ScaleMode : uint8_t
		{
			Stretch, // fill bounds exactly, ignore source aspect
			Fit,     // largest size that fits inside bounds, aspect preserved
			Fill,    // smallest size that covers bounds, aspect preserved (clipped)
		};

		Image() = default;
		explicit Image(Onyx::Texture* tex) : m_Texture(tex) {}

		void SetTexture(Onyx::Texture* t) { m_Texture = t; }
		Onyx::Texture* GetTexture() const { return m_Texture; }

		void SetTint(const Color& c)     { m_Tint = c; }
		void SetScaleMode(ScaleMode m)   { m_ScaleMode = m; }

		void OnDraw(UIRenderer& r) override;

	private:
		Onyx::Texture* m_Texture = nullptr;
		Color     m_Tint      = Color::White();
		ScaleMode m_ScaleMode = ScaleMode::Stretch;
	};

} // namespace Onyx::UI
