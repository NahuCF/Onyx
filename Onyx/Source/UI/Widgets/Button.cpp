#include "Button.h"

#include "Source/UI/Render/UIRenderer.h"
#include "Source/UI/Text/FontAtlas.h"

namespace Onyx::UI {

	Button::Button(const FontAtlas& font, std::string text, float pxSize)
		: m_Font(font), m_Text(std::move(text)), m_PxSize(pxSize)
	{
		SetInteractive(true);
		SetFocusable(true);
	}

	void Button::ApplyStyle(Style s)
	{
		switch (s)
		{
			case Style::Primary:
				m_BaseColor    = Color::FromRGBA8(0x3a7bd5FF);
				m_HoverColor   = Color::FromRGBA8(0x5a9bffFF);
				m_PressedColor = Color::FromRGBA8(0x2858a5FF);
				m_TextColor    = Color::White();
				break;
			case Style::Secondary:
				m_BaseColor    = Color::FromRGBA8(0x4a5160FF);
				m_HoverColor   = Color::FromRGBA8(0x5e6573FF);
				m_PressedColor = Color::FromRGBA8(0x363c48FF);
				m_TextColor    = Color::FromRGBA8(0xeef0f6FF);
				break;
			case Style::Danger:
				m_BaseColor    = Color::FromRGBA8(0xc23a3aFF);
				m_HoverColor   = Color::FromRGBA8(0xe05050FF);
				m_PressedColor = Color::FromRGBA8(0x8a2424FF);
				m_TextColor    = Color::White();
				break;
			case Style::Ghost:
				m_BaseColor    = Color::FromRGBA8(0x00000000);
				m_HoverColor   = Color::FromRGBA8(0xffffff20);
				m_PressedColor = Color::FromRGBA8(0xffffff10);
				m_TextColor    = Color::FromRGBA8(0xeef0f6FF);
				break;
		}
	}

	void Button::OnDraw(UIRenderer& r)
	{
		Color bg = m_BaseColor;
		if (IsPressed())
			bg = m_PressedColor;
		else if (IsHovered())
			bg = m_HoverColor;

		// Background.
		r.DrawRoundedRect(GetBounds(), bg, m_CornerRadius);

		// Focus indicator — accessibility requirement (Q-spec calls this out
		// in M6 but we honor it from the start since the widget is focusable).
		if (IsFocused())
		{
			const Color outline = Color::FromRGBA8(0xffd24aFF);
			const Rect2D b = GetBounds();
			const float t = 1.5f;
			r.DrawRect(Rect2D{glm::vec2(b.min.x, b.min.y),         glm::vec2(b.max.x, b.min.y + t)}, outline);
			r.DrawRect(Rect2D{glm::vec2(b.min.x, b.max.y - t),     glm::vec2(b.max.x, b.max.y)},     outline);
			r.DrawRect(Rect2D{glm::vec2(b.min.x, b.min.y),         glm::vec2(b.min.x + t, b.max.y)}, outline);
			r.DrawRect(Rect2D{glm::vec2(b.max.x - t, b.min.y),     glm::vec2(b.max.x, b.max.y)},     outline);
		}

		// Centered label.
		if (!m_Text.empty() && m_Font.IsLoaded())
		{
			const FontMetrics& fm = m_Font.GetMetrics();
			const float scale = m_PxSize / fm.bakeSize;
			const float textHeight = fm.lineHeight * scale;

			Rect2D textBounds = GetBounds();
			const float yOffset = (textBounds.Height() - textHeight) * 0.5f;
			textBounds.min.y += yOffset;
			textBounds.max.y -= yOffset;

			r.DrawText(textBounds, m_Text, m_Font, m_PxSize, m_TextColor,
					   UIRenderer::TextAlignH::Center);
		}
	}

	void Button::OnInput(const InputEvent& e)
	{
		if (e.type == InputEventType::MouseUp && e.button == MouseButton::Left && IsHovered())
		{
			if (m_OnClick)
				m_OnClick();
		}
		// Keyboard activation: Enter / Space when focused fires the click handler.
		else if (e.type == InputEventType::KeyDown && IsFocused())
		{
			// GLFW_KEY_ENTER = 257, GLFW_KEY_SPACE = 32
			if (e.keyCode == 257 || e.keyCode == 32)
			{
				if (m_OnClick)
					m_OnClick();
			}
		}
	}

} // namespace Onyx::UI
