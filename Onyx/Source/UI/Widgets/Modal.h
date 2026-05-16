#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widget.h"

#include <functional>

namespace Onyx::UI {

	// Full-viewport dimmed overlay with a centered content panel. Phase-3
	// dialogue UI. Being interactive, it swallows clicks on the backdrop so
	// input doesn't fall through to the HUD behind (true modal grab); clicks on
	// interactive content children still route to them normally.
	//
	// Place at/near the tree root with FillSize (the ctor sets that). Escape-
	// to-close requires the modal or a child to hold focus (keys route to the
	// focused widget); Close() is also exposed for explicit owner wiring.
	class Modal : public Widget
	{
	public:
		Modal();

		void SetContent(std::unique_ptr<Widget> content);
		Widget* GetContent() const { return m_Content; }

		void SetBackdropColor(const Color& c) { m_Backdrop = c; }
		void SetPanel(bool on, const Color& c = Color::FromRGBA8(0x232a36F2),
					  float pad = 18.0f, float radius = 8.0f)
		{ m_DrawPanel = on; m_PanelColor = c; m_PanelPad = pad; m_PanelRadius = radius; }

		void SetCloseOnBackdrop(bool b) { m_CloseOnBackdrop = b; }
		void SetCloseOnEscape(bool b)   { m_CloseOnEscape = b; }
		void SetOnClose(std::function<void()> fn) { m_OnClose = std::move(fn); }

		// Fire the close handler (owner decides what "close" means — usually
		// SetVisible(false) or popping the overlay).
		void Close();

		void OnDraw(UIRenderer& r) override;
		void OnInput(const InputEvent& e) override;

	private:
		Widget* m_Content = nullptr; // owned via AddChild

		Color m_Backdrop   = Color::FromRGBA8(0x0000008C);
		bool  m_DrawPanel   = true;
		Color m_PanelColor  = Color::FromRGBA8(0x232A36F2);
		float m_PanelPad    = 18.0f;
		float m_PanelRadius  = 8.0f;

		bool m_CloseOnBackdrop = true;
		bool m_CloseOnEscape   = true;
		std::function<void()> m_OnClose;
	};

} // namespace Onyx::UI
