#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widget.h"

namespace Onyx::UI {

	class FontAtlas;

	// Radial "sweep" mask drawn over an action-bar slot while an ability is on
	// cooldown. Layer it above an Icon in the slot. Remaining fraction is
	// pushed imperatively (1 = just triggered, 0 = ready) — wired from the
	// server cooldown events for the demo; no M7 binding needed.
	class CooldownOverlay : public Widget
	{
	public:
		CooldownOverlay() = default;

		// 1 = full cooldown remaining, 0 = ready (overlay draws nothing).
		void SetRemaining(float f01);
		float GetRemaining() const { return m_Remaining; }

		// Optional numeric countdown. Duration is the ability's full cooldown
		// so the label can show remaining seconds.
		void SetCountdownFont(const FontAtlas* f, float pxSize = 15.0f)
		{ m_Font = f; m_LabelPx = pxSize; }
		void SetDuration(float seconds) { m_Duration = seconds; }

		void SetMaskColor(const Color& c) { m_MaskColor = c; }
		void SetLabelColor(const Color& c) { m_LabelColor = c; }

		void OnDraw(UIRenderer& r) override;

	private:
		float m_Remaining = 0.0f;
		float m_Duration  = 0.0f;
		const FontAtlas* m_Font = nullptr;
		float m_LabelPx = 15.0f;
		Color m_MaskColor  = Color::FromRGBA8(0x00000099);
		Color m_LabelColor = Color::FromRGBA8(0xFFFFFFE6);
	};

} // namespace Onyx::UI
