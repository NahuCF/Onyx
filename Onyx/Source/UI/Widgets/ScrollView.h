#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widget.h"

namespace Onyx::UI {

	// Vertically scrolling, scissor-clipped container. Phase-3 quest tracker /
	// chat-style lists. Add rows via Content() (an internal vertical stack);
	// the view computes content height from child specs and clamps scroll.
	//
	// v1 is non-virtualized (all rows submitted, clipped). Virtualization
	// (render-only-visible + recycling) is the documented M9 perf follow-up —
	// this is a correct v1 of the same widget, not a stop-gap. Fine for the
	// demo's row counts; revisit before a 10k-line chat log.
	class ScrollView : public Widget
	{
	public:
		ScrollView();

		// Internal content holder (vertical stack). Add list items here.
		Widget* Content() const { return m_Content; }
		void AddItem(std::unique_ptr<Widget> item);

		void SetGap(float g)            { m_Gap = g; }
		void SetScrollSpeed(float px)   { m_WheelStep = px; }
		void SetShowScrollbar(bool s)   { m_ShowScrollbar = s; }
		void SetScrollbarColor(const Color& track, const Color& thumb)
		{ m_BarTrack = track; m_BarThumb = thumb; }

		float GetScroll() const { return m_ScrollY; }
		void  ScrollToBottom()  { m_PinBottom = true; }

		void OnUpdate(float dt) override;
		void OnInput(const InputEvent& e) override;
		void OnDraw(UIRenderer& r) override;

	private:
		float ContentHeight() const;

		Widget* m_Content = nullptr; // owned via AddChild
		float m_ScrollY     = 0.0f;
		float m_MaxScroll   = 0.0f;
		float m_Gap         = 4.0f;
		float m_WheelStep   = 1.0f; // multiplier on the (already *30) wheel delta
		bool  m_ShowScrollbar = true;
		bool  m_PinBottom     = false;

		Color m_BarTrack = Color::FromRGBA8(0xFFFFFF14);
		Color m_BarThumb = Color::FromRGBA8(0xFFFFFF55);
	};

} // namespace Onyx::UI
