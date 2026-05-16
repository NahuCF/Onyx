#include "Modal.h"

#include "Source/UI/Render/UIRenderer.h"

namespace Onyx::UI {

	namespace { constexpr uint32_t kKeyEscape = 256; } // GLFW_KEY_ESCAPE

	Modal::Modal()
	{
		SetInteractive(true); // swallow backdrop clicks (modal input grab)
		SetFocusable(true);   // lets Escape route here when focused
		SetContainerMode(ContainerMode::Free);
		FillSize();
	}

	void Modal::SetContent(std::unique_ptr<Widget> content)
	{
		if (!content) return;
		content->GetLayoutSpec().anchor = AnchorEdge::Center;
		m_Content = content.get();
		AddChild(std::move(content));
	}

	void Modal::Close()
	{
		if (m_OnClose)
			m_OnClose();
	}

	void Modal::OnDraw(UIRenderer& r)
	{
		// Dim the whole screen. Drawn before children, so the panel + content
		// composite on top.
		r.DrawRect(GetBounds(), m_Backdrop);

		if (m_DrawPanel && m_Content)
		{
			const Rect2D panel = m_Content->GetBounds().Inset(-m_PanelPad);
			r.DrawRoundedRect(panel, m_PanelColor, m_PanelRadius);
		}
	}

	void Modal::OnInput(const InputEvent& e)
	{
		if (e.type == InputEventType::KeyDown &&
			e.keyCode == kKeyEscape && m_CloseOnEscape)
		{
			Close();
			return;
		}

		if (e.type == InputEventType::MouseDown && m_CloseOnBackdrop)
		{
			// A click only reaches Modal::OnInput when it didn't land on an
			// interactive content child (the tree routes to the deepest
			// interactive widget). Still, guard against the panel region so a
			// click on non-interactive panel chrome doesn't dismiss.
			if (m_Content)
			{
				const Rect2D hit = m_DrawPanel
									   ? m_Content->GetBounds().Inset(-m_PanelPad)
									   : m_Content->GetBounds();
				if (hit.Contains(e.position))
					return;
			}
			Close();
		}
	}

} // namespace Onyx::UI
