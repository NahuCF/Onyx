#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widget.h"

namespace Onyx::UI {

	// Minimal interactive widget for M2 smoke testing. Renders a filled rect
	// that brightens on hover and shifts hue on press; logs click events to
	// stdout. Lives in widgets/ so the editor can push one without depending
	// on a higher-level theme or layout system.
	class DebugRect : public Widget
	{
	public:
		DebugRect(const Color& base, const std::string& label = "DebugRect");

		void OnDraw(UIRenderer& r) override;
		void OnInput(const InputEvent& e) override;

		uint32_t GetClickCount() const { return m_Clicks; }

		const Color& GetBaseColor() const { return m_Base; }
		void SetBaseColor(const Color& c) { m_Base = c; }

		const std::string& GetLabel() const { return m_Label; }
		void SetLabel(std::string label) { m_Label = std::move(label); }

	private:
		Color m_Base;
		std::string m_Label;
		uint32_t m_Clicks = 0;
	};

} // namespace Onyx::UI
