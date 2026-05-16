#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widgets/WorldAnchoredWidget.h"

#include <string>

namespace Onyx::UI {

	class FontAtlas;

	// Rising, fading world-anchored text — Phase-1 damage numbers, heal/XP
	// pops. Spawn one at the hit point, add it to a HUD container, and remove
	// it once IsDead(). Self-animating; no bindings needed.
	class FloatingText : public WorldAnchoredWidget
	{
	public:
		FloatingText(const FontAtlas* font, std::string text,
					 const Color& color, float pxSize = 18.0f);

		void  SetLifetime(float seconds) { m_Life = seconds; }
		void  SetRiseSpeed(float pxPerS) { m_RiseSpeed = pxPerS; }
		bool  IsDead() const             { return m_Dead; }

		void OnUpdate(float dt) override;
		void OnDraw(UIRenderer& r) override;

	private:
		const FontAtlas* m_Font;
		std::string m_Text;
		Color m_Color;
		float m_PxSize;

		float m_Age       = 0.0f;
		float m_Life      = 1.1f;
		float m_RiseSpeed = 46.0f; // screen px / second, upward
		bool  m_Dead      = false;
	};

} // namespace Onyx::UI
