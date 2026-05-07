#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Render/UIRenderer.h"
#include "Source/UI/Widget.h"

#include <functional>
#include <string>

namespace Onyx::UI {

	class FontAtlas;

	// Standard interactive button: rounded rect background + centered label.
	// Background tints on hover / press; click fires the OnClick callback.
	// Focusable so Tab navigation works.
	//
	// Visual: theme JSON will own colors / radius once M6 lands; for now the
	// widget exposes per-instance setters so the example stages can tint
	// (primary / secondary / danger) without a theme.
	class Button : public Widget
	{
	public:
		Button(const FontAtlas& font, std::string text, float pxSize = 16.0f);

		const std::string& GetText() const { return m_Text; }
		void SetText(std::string text) { m_Text = std::move(text); }

		float GetPixelSize() const { return m_PxSize; }
		void SetPixelSize(float px) { m_PxSize = px; }

		const Color& GetBaseColor() const { return m_BaseColor; }
		void SetBaseColor(const Color& c) { m_BaseColor = c; }

		const Color& GetHoverColor() const { return m_HoverColor; }
		void SetHoverColor(const Color& c) { m_HoverColor = c; }

		const Color& GetPressedColor() const { return m_PressedColor; }
		void SetPressedColor(const Color& c) { m_PressedColor = c; }

		const Color& GetTextColor() const { return m_TextColor; }
		void SetTextColor(const Color& c) { m_TextColor = c; }

		float GetCornerRadius() const { return m_CornerRadius; }
		void SetCornerRadius(float r) { m_CornerRadius = r; }

		// Convenience preset: primary / secondary / danger / ghost. Sets
		// base + hover + pressed colors so callers can opt into a coherent
		// palette without listing all three.
		enum class Style : uint8_t { Primary, Secondary, Danger, Ghost };
		void ApplyStyle(Style s);

		void SetOnClick(std::function<void()> handler) { m_OnClick = std::move(handler); }

		void OnDraw(UIRenderer& r) override;
		void OnInput(const InputEvent& e) override;

	private:
		const FontAtlas& m_Font;
		std::string m_Text;
		float m_PxSize;

		Color m_BaseColor    = Color::FromRGBA8(0x3a7bd5FF);
		Color m_HoverColor   = Color::FromRGBA8(0x5a9bffFF);
		Color m_PressedColor = Color::FromRGBA8(0x2858a5FF);
		Color m_TextColor    = Color::White();
		float m_CornerRadius = 6.0f;

		std::function<void()> m_OnClick;
	};

} // namespace Onyx::UI
