#include "FloatingText.h"

#include "Source/UI/Render/UIRenderer.h"
#include "Source/UI/Text/FontAtlas.h"

#include <algorithm>

namespace Onyx::UI {

	FloatingText::FloatingText(const FontAtlas* font, std::string text,
							   const Color& color, float pxSize)
		: m_Font(font), m_Text(std::move(text)), m_Color(color), m_PxSize(pxSize)
	{
	}

	void FloatingText::OnUpdate(float dt)
	{
		WorldAnchoredWidget::OnUpdate(dt); // refresh projection

		m_Age += dt;
		if (m_Age >= m_Life)
			m_Dead = true;

		if (!m_OnScreen || m_Dead || !m_Font || !m_Font->IsLoaded())
		{
			SetBounds(Rect2D{}); // collapse: skip hit-test/draw, keep updating
			return;
		}

		// Caller measures so bounds tightly wrap the run (centered on anchor,
		// rising over its lifetime).
		// Note: MeasureText needs a renderer; size is recomputed in OnDraw via
		// the font metrics instead to avoid holding a renderer here.
		const FontMetrics& fm = m_Font->GetMetrics();
		const float scale = m_PxSize / fm.bakeSize;
		const float h = fm.lineHeight * scale;
		const float wApprox = static_cast<float>(m_Text.size()) * m_PxSize * 0.6f;

		const float rise = m_RiseSpeed * m_Age;
		const glm::vec2 c{m_ScreenPos.x, m_ScreenPos.y - rise};
		SetBounds(Rect2D{glm::vec2(c.x - wApprox * 0.5f, c.y - h * 0.5f),
						 glm::vec2(c.x + wApprox * 0.5f, c.y + h * 0.5f)});
	}

	void FloatingText::OnDraw(UIRenderer& r)
	{
		if (!m_OnScreen || m_Dead || GetBounds().Empty() ||
			!m_Font || !m_Font->IsLoaded())
			return;

		const float t = m_Age / m_Life;
		float alpha = 1.0f;
		if (t > 0.55f)
			alpha = std::clamp(1.0f - (t - 0.55f) / 0.45f, 0.0f, 1.0f);

		Color c = m_Color;
		c.a *= alpha;
		r.DrawText(GetBounds(), m_Text, *m_Font, m_PxSize, c,
				   UIRenderer::TextAlignH::Center);
	}

} // namespace Onyx::UI
