#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widget.h"

#include <string>

namespace Onyx::UI {

	class FontAtlas;

	// Horizontal left-to-right fill bar, normalized value 0..1. Track + fill +
	// optional border and centered label. Drives Phase-1 HP bars (player +
	// target) and Phase-2 cast bars. Data is pushed imperatively via SetValue /
	// SetCurrent until M7 reactive bindings land — the design doc explicitly
	// permits imperative wiring for the demo.
	class ProgressBar : public Widget
	{
	public:
		ProgressBar() = default;

		float GetValue() const { return m_Value; }
		void  SetValue(float v01);            // clamps to [0,1]

		// Convenience for HP/mana/cast: set a range once, then push current.
		void  SetRange(float lo, float hi);
		void  SetCurrent(float current);      // normalized against the range

		void  SetTrackColor(const Color& c)  { m_TrackColor = c; }
		void  SetFillColor(const Color& c)   { m_FillColor = c; }
		void  SetBorderColor(const Color& c) { m_BorderColor = c; }
		void  SetBorderThickness(float t)    { m_BorderThickness = t; m_HasBorder = t > 0.0f; }
		void  SetCornerRadius(float r)       { m_CornerRadius = r; }

		// Optional centered label. `font` is non-owning (e.g.
		// Manager::GetDefaultFont()); pass nullptr / empty text for no label.
		void  SetLabel(const FontAtlas* font, std::string text,
					   float pxSize = 13.0f, const Color& color = Color::White());
		void  SetLabelText(std::string text) { m_LabelText = std::move(text); }

		void OnDraw(UIRenderer& r) override;

	private:
		float m_Value   = 0.0f;
		float m_RangeLo = 0.0f;
		float m_RangeHi = 1.0f;

		Color m_TrackColor  = Color::FromRGBA8(0x222831C0);
		Color m_FillColor   = Color::FromRGBA8(0x4CAF50FF);
		Color m_BorderColor = Color::FromRGBA8(0x000000A0);
		bool  m_HasBorder       = true;
		float m_BorderThickness = 1.0f;
		float m_CornerRadius    = 3.0f;

		const FontAtlas* m_Font = nullptr;
		std::string m_LabelText;
		float m_LabelPx = 13.0f;
		Color m_LabelColor = Color::White();
	};

} // namespace Onyx::UI
