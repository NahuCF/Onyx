#include "Label.h"

#include "Source/UI/Text/FontAtlas.h"

namespace Onyx::UI {

	Label::Label(const FontAtlas& font, std::string text, float pxSize, const Color& color)
		: m_Font(font), m_Text(std::move(text)), m_PxSize(pxSize), m_Color(color)
	{
		// Labels don't intercept input by default; mark interactive only when
		// the consumer wants tooltips or click-to-copy semantics.
	}

	void Label::OnDraw(UIRenderer& r)
	{
		r.DrawText(GetBounds(), m_Text, m_Font, m_PxSize, m_Color, m_Align);
	}

} // namespace Onyx::UI
