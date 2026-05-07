#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Render/UIRenderer.h"
#include "Source/UI/Widget.h"
#include <string>

namespace Onyx::UI {

	class FontAtlas;

	// Single-line text widget. Wrapping + multi-line support comes when a
	// real layout consumer needs it (M9 widgets like Tooltip body).
	class Label : public Widget
	{
	public:
		Label(const FontAtlas& font, std::string text, float pxSize = 16.0f,
			  const Color& color = Color::White());

		void OnDraw(UIRenderer& r) override;

		const std::string& GetText() const { return m_Text; }
		void SetText(std::string text) { m_Text = std::move(text); }

		float GetPixelSize() const { return m_PxSize; }
		void SetPixelSize(float px) { m_PxSize = px; }

		const Color& GetColor() const { return m_Color; }
		void SetColor(const Color& c) { m_Color = c; }

		UIRenderer::TextAlignH GetAlign() const { return m_Align; }
		void SetAlign(UIRenderer::TextAlignH a) { m_Align = a; }

	private:
		const FontAtlas& m_Font;
		std::string m_Text;
		float m_PxSize;
		Color m_Color;
		UIRenderer::TextAlignH m_Align = UIRenderer::TextAlignH::Left;
	};

} // namespace Onyx::UI
