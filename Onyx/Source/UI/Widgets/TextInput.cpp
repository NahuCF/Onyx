#include "TextInput.h"

#include "Source/UI/Render/UIRenderer.h"
#include "Source/UI/Text/FontAtlas.h"

#include <algorithm>

namespace Onyx::UI {

	namespace {
		// GLFW key codes used for editing. Pulled in by value so this widget
		// doesn't need to include the GLFW header — the engine already maps
		// platform keys to GLFW codes in InputEvent.keyCode.
		constexpr uint32_t KEY_ENTER     = 257;
		constexpr uint32_t KEY_KP_ENTER  = 335;
		constexpr uint32_t KEY_TAB       = 258;
		constexpr uint32_t KEY_BACKSPACE = 259;
		constexpr uint32_t KEY_DELETE    = 261;
		constexpr uint32_t KEY_RIGHT     = 262;
		constexpr uint32_t KEY_LEFT      = 263;
		constexpr uint32_t KEY_HOME      = 268;
		constexpr uint32_t KEY_END       = 269;
	}

	TextInput::TextInput(const FontAtlas& font, float pxSize)
		: m_Font(font), m_PxSize(pxSize)
	{
		SetInteractive(true);
		SetFocusable(true);
	}

	void TextInput::SetText(std::string text)
	{
		m_Text = std::move(text);
		if (m_Cursor > m_Text.size())
			m_Cursor = m_Text.size();
		NotifyChange();
	}

	std::string TextInput::DisplayString() const
	{
		if (!m_Password)
			return m_Text;
		// One bullet per character. Latin-1 BULLET (U+2022) is outside our
		// atlas range; using ASCII '*' keeps it inside the baked charset.
		return std::string(m_Text.size(), '*');
	}

	void TextInput::NotifyChange()
	{
		if (m_OnChange)
			m_OnChange(m_Text);
	}

	void TextInput::OnUpdate(float dt)
	{
		if (IsFocused())
		{
			m_CursorBlink += dt;
			if (m_CursorBlink >= 1.0f)
				m_CursorBlink = 0.0f;
		}
		else
		{
			m_CursorBlink = 0.0f;
		}
	}

	void TextInput::OnDraw(UIRenderer& r)
	{
		const Rect2D bounds = GetBounds();

		// Background tints when focused (slightly lighter to read as active).
		const Color bg = IsFocused() ? m_FocusColor : m_BaseColor;
		r.DrawRoundedRect(bounds, bg, 4.0f);

		// Outline when focused — accessibility focus indicator.
		if (IsFocused())
		{
			const float t = 1.5f;
			r.DrawRect(Rect2D{glm::vec2(bounds.min.x, bounds.min.y),         glm::vec2(bounds.max.x, bounds.min.y + t)}, m_OutlineColor);
			r.DrawRect(Rect2D{glm::vec2(bounds.min.x, bounds.max.y - t),     glm::vec2(bounds.max.x, bounds.max.y)},     m_OutlineColor);
			r.DrawRect(Rect2D{glm::vec2(bounds.min.x, bounds.min.y),         glm::vec2(bounds.min.x + t, bounds.max.y)}, m_OutlineColor);
			r.DrawRect(Rect2D{glm::vec2(bounds.max.x - t, bounds.min.y),     glm::vec2(bounds.max.x, bounds.max.y)},     m_OutlineColor);
		}

		if (!m_Font.IsLoaded())
			return;

		const FontMetrics& fm = m_Font.GetMetrics();
		const float scale = m_PxSize / fm.bakeSize;
		const float textHeight = fm.lineHeight * scale;

		// Inset rect for text (left padding 8, vertical center).
		const float padX = 8.0f;
		const float yOffset = std::max(0.0f, (bounds.Height() - textHeight) * 0.5f);
		const Rect2D textBounds{
			glm::vec2(bounds.min.x + padX, bounds.min.y + yOffset),
			glm::vec2(bounds.max.x - padX, bounds.max.y - yOffset),
		};

		// Display text + placeholder fallback.
		const std::string display = DisplayString();
		if (!display.empty())
		{
			r.DrawText(textBounds, display, m_Font, m_PxSize, m_TextColor,
					   UIRenderer::TextAlignH::Left);
		}
		else if (!m_Placeholder.empty() && !IsFocused())
		{
			r.DrawText(textBounds, m_Placeholder, m_Font, m_PxSize, m_PlaceholderColor,
					   UIRenderer::TextAlignH::Left);
		}

		// Cursor — only when focused and on the visible half of the blink.
		if (IsFocused() && m_CursorBlink < 0.5f)
		{
			const std::string before = display.substr(0, std::min(m_Cursor, display.size()));
			const float advance = r.MeasureText(before, m_Font, m_PxSize).x;
			const float cx = textBounds.min.x + advance;
			// Cursor height tracks a typical glyph height (cap-height-ish):
			// half the textHeight gives a clean bar that doesn't drop into
			// descender space.
			const float cursorTop    = bounds.min.y + (bounds.Height() - textHeight) * 0.5f + 2.0f;
			const float cursorBottom = bounds.max.y - (bounds.Height() - textHeight) * 0.5f - 2.0f;
			r.DrawRect(Rect2D{glm::vec2(cx, cursorTop), glm::vec2(cx + 1.5f, cursorBottom)},
					   m_CursorColor);
		}
	}

	void TextInput::OnInput(const InputEvent& e)
	{
		switch (e.type)
		{
			case InputEventType::MouseDown:
				// WidgetTree auto-focuses focusable widgets on MouseDown; we
				// just set the cursor position. Approximation: place cursor
				// at end (full hit-test against per-glyph advances is M9
				// polish, not needed for credentials / character names).
				m_Cursor = m_Text.size();
				m_CursorBlink = 0.0f;
				break;

			case InputEventType::Char:
				// ASCII printable only for now — atlas covers Latin-1 but
				// the buffer-edit path is byte-indexed; multi-byte UTF-8
				// cursor stepping is M9 polish.
				if (e.character >= 32 && e.character <= 126 &&
					m_Text.size() < m_MaxLength)
				{
					m_Text.insert(m_Cursor, 1, static_cast<char>(e.character));
					++m_Cursor;
					m_CursorBlink = 0.0f;
					NotifyChange();
				}
				break;

			case InputEventType::KeyDown:
				switch (e.keyCode)
				{
					case KEY_BACKSPACE:
						if (m_Cursor > 0)
						{
							m_Text.erase(m_Cursor - 1, 1);
							--m_Cursor;
							NotifyChange();
						}
						break;
					case KEY_DELETE:
						if (m_Cursor < m_Text.size())
						{
							m_Text.erase(m_Cursor, 1);
							NotifyChange();
						}
						break;
					case KEY_LEFT:
						if (m_Cursor > 0)
							--m_Cursor;
						break;
					case KEY_RIGHT:
						if (m_Cursor < m_Text.size())
							++m_Cursor;
						break;
					case KEY_HOME:
						m_Cursor = 0;
						break;
					case KEY_END:
						m_Cursor = m_Text.size();
						break;
					case KEY_ENTER:
					case KEY_KP_ENTER:
						if (m_OnSubmit)
							m_OnSubmit(m_Text);
						break;
					default:
						break;
				}
				m_CursorBlink = 0.0f;
				break;

			default:
				break;
		}
	}

} // namespace Onyx::UI
