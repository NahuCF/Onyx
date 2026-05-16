#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widgets/WorldAnchoredWidget.h"

#include <string>

namespace Onyx::UI {

	class FontAtlas;

	// Name label + optional health bar floating above a creature/NPC head.
	// Covers the Phase-1 nameplate/overhead-bar use. The game sets the world
	// position (entity head), name, and HP fraction each frame; the widget
	// projects, sizes itself, and paints. Self-drawing leaf — no sub-widgets.
	class Nameplate : public WorldAnchoredWidget
	{
	public:
		explicit Nameplate(const FontAtlas* font, float pxSize = 13.0f);

		void SetName(std::string n) { m_Name = std::move(n); }
		void SetNameColor(const Color& c) { m_NameColor = c; }

		void SetShowHealth(bool s) { m_ShowHealth = s; }
		void SetHealthFraction(float f01); // clamps 0..1
		void SetHealthColors(const Color& track, const Color& fill)
		{ m_HpTrack = track; m_HpFill = fill; }
		void SetBackgroundColor(const Color& c) { m_Background = c; }
		void SetBarWidth(float w) { m_BarWidth = w; }

		void OnUpdate(float dt) override;
		void OnDraw(UIRenderer& r) override;

	private:
		const FontAtlas* m_Font;
		float m_PxSize;
		std::string m_Name;

		bool  m_ShowHealth = true;
		float m_Hp = 1.0f;
		float m_BarWidth = 96.0f;

		Color m_NameColor  = Color::FromRGBA8(0xF2F4F8FF);
		Color m_Background  = Color::FromRGBA8(0x0A0D12B4);
		Color m_HpTrack    = Color::FromRGBA8(0x000000A0);
		Color m_HpFill     = Color::FromRGBA8(0xC0382EFF);
	};

} // namespace Onyx::UI
