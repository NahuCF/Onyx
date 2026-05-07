#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widget.h"

#include <functional>
#include <string>

namespace Onyx::UI {

	class FontAtlas;

	// Single-line text input. Click to focus, type to insert, Backspace /
	// Delete / arrows / Home / End / Enter all behave conventionally. The
	// cursor blinks while focused. Password mode renders each character as
	// a bullet glyph.
	//
	// ASCII-only for the first cut — the full-width Latin codepoint handling
	// (U+0080-U+017F) lands when the example stages need diacritics for
	// player names (the atlas already covers them).
	class TextInput : public Widget
	{
	public:
		TextInput(const FontAtlas& font, float pxSize = 14.0f);

		const std::string& GetText() const { return m_Text; }
		void               SetText(std::string text);

		const std::string& GetPlaceholder() const { return m_Placeholder; }
		void               SetPlaceholder(std::string p) { m_Placeholder = std::move(p); }

		bool IsPasswordMode() const { return m_Password; }
		void SetPasswordMode(bool yes) { m_Password = yes; }

		size_t GetMaxLength() const { return m_MaxLength; }
		void   SetMaxLength(size_t n) { m_MaxLength = n; }

		float GetPixelSize() const { return m_PxSize; }
		void  SetPixelSize(float px) { m_PxSize = px; }

		// Style hooks. Theme JSON owns these once M6 lands; per-instance
		// setters keep the example stages legible without a theme.
		void SetBaseColor(const Color& c)        { m_BaseColor = c; }
		void SetFocusColor(const Color& c)       { m_FocusColor = c; }
		void SetTextColor(const Color& c)        { m_TextColor = c; }
		void SetPlaceholderColor(const Color& c) { m_PlaceholderColor = c; }
		void SetCursorColor(const Color& c)      { m_CursorColor = c; }
		void SetOutlineColor(const Color& c)     { m_OutlineColor = c; }

		void SetOnSubmit(std::function<void(const std::string&)> handler) { m_OnSubmit = std::move(handler); }
		void SetOnChange(std::function<void(const std::string&)> handler) { m_OnChange = std::move(handler); }

		void OnDraw(UIRenderer& r) override;
		void OnInput(const InputEvent& e) override;
		void OnUpdate(float dt) override;

	private:
		void NotifyChange();
		std::string DisplayString() const; // password mask if needed

		const FontAtlas& m_Font;
		float            m_PxSize;
		std::string      m_Text;
		std::string      m_Placeholder;
		size_t           m_Cursor    = 0;
		size_t           m_MaxLength = 256;
		bool             m_Password  = false;

		float m_CursorBlink = 0.0f; // 0..1, > 0.5 hides cursor

		Color m_BaseColor        = Color::FromRGBA8(0x2a3140FF);
		Color m_FocusColor       = Color::FromRGBA8(0x323a4cFF);
		Color m_TextColor        = Color::FromRGBA8(0xeef0f6FF);
		Color m_PlaceholderColor = Color::FromRGBA8(0x808898FF);
		Color m_CursorColor      = Color::FromRGBA8(0xeef0f6FF);
		Color m_OutlineColor     = Color::FromRGBA8(0x3a7bd5FF);

		std::function<void(const std::string&)> m_OnChange;
		std::function<void(const std::string&)> m_OnSubmit;
	};

} // namespace Onyx::UI
