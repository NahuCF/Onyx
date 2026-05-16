#include "ProgressBar.h"

#include "Source/UI/Render/UIRenderer.h"
#include "Source/UI/Text/FontAtlas.h"

#include <algorithm>

namespace Onyx::UI {

	void ProgressBar::SetValue(float v01)
	{
		m_Value = std::clamp(v01, 0.0f, 1.0f);
	}

	void ProgressBar::SetRange(float lo, float hi)
	{
		m_RangeLo = lo;
		m_RangeHi = hi;
	}

	void ProgressBar::SetCurrent(float current)
	{
		const float span = m_RangeHi - m_RangeLo;
		SetValue(span > 0.0f ? (current - m_RangeLo) / span : 0.0f);
	}

	void ProgressBar::SetLabel(const FontAtlas* font, std::string text,
							   float pxSize, const Color& color)
	{
		m_Font       = font;
		m_LabelText  = std::move(text);
		m_LabelPx    = pxSize;
		m_LabelColor = color;
	}

	void ProgressBar::OnDraw(UIRenderer& r)
	{
		const Rect2D bounds = GetBounds();
		if (bounds.Empty())
			return;

		// Border = outer fill in the border color; track painted inset by the
		// border thickness. Cheaper and cleaner than four edge rects, and the
		// rounded-rect path keeps the corner radius coherent.
		Rect2D inner = bounds;
		if (m_HasBorder && m_BorderThickness > 0.0f)
		{
			r.DrawRoundedRect(bounds, m_BorderColor, m_CornerRadius);
			inner = bounds.Inset(m_BorderThickness);
		}

		const float innerRadius = std::max(0.0f, m_CornerRadius - m_BorderThickness);
		r.DrawRoundedRect(inner, m_TrackColor, innerRadius);

		// Fill is a plain rect from the left edge — a partial fill must keep a
		// square right edge, so it intentionally does not round.
		if (m_Value > 0.0f && !inner.Empty())
		{
			Rect2D fill = inner;
			fill.max.x = inner.min.x + inner.Width() * m_Value;
			r.DrawRect(fill, m_FillColor);
		}

		// Centered label (e.g. "532 / 800", "2.1s").
		if (m_Font && m_Font->IsLoaded() && !m_LabelText.empty())
		{
			const FontMetrics& fm = m_Font->GetMetrics();
			const float scale = m_LabelPx / fm.bakeSize;
			const float textH = fm.lineHeight * scale;

			Rect2D tb = bounds;
			const float yOff = (tb.Height() - textH) * 0.5f;
			tb.min.y += yOff;
			tb.max.y -= yOff;

			r.DrawText(tb, m_LabelText, *m_Font, m_LabelPx, m_LabelColor,
					   UIRenderer::TextAlignH::Center);
		}
	}

} // namespace Onyx::UI
