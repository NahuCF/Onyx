#include "CooldownOverlay.h"

#include "Source/UI/Render/UIRenderer.h"
#include "Source/UI/Text/FontAtlas.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Onyx::UI {

	namespace { constexpr float kPi = 3.14159265358979323846f; }

	void CooldownOverlay::SetRemaining(float f01)
	{
		m_Remaining = std::clamp(f01, 0.0f, 1.0f);
	}

	void CooldownOverlay::OnDraw(UIRenderer& r)
	{
		if (m_Remaining <= 0.0f)
			return; // ability ready — nothing to mask

		const Rect2D b = GetBounds();
		if (b.Empty())
			return;

		const glm::vec2 center = b.Center();
		// Half-diagonal fully covers a square slot; clip to bounds so the
		// overflow never paints neighbouring slots.
		const float radius = 0.5f * std::sqrt(b.Width() * b.Width() +
											  b.Height() * b.Height());

		const float startRad = -kPi * 0.5f;             // 12 o'clock
		const float sweepRad = m_Remaining * 2.0f * kPi; // clockwise wipe

		r.PushScissor(b);
		r.DrawPie(center, radius, startRad, sweepRad, m_MaskColor);
		r.PopScissor();

		if (m_Font && m_Font->IsLoaded() && m_Duration > 0.0f)
		{
			const float secs = m_Remaining * m_Duration;
			char buf[16];
			if (secs >= 10.0f)
				std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(std::ceil(secs)));
			else
				std::snprintf(buf, sizeof(buf), "%.1f", secs);

			const FontMetrics& fm = m_Font->GetMetrics();
			const float scale = m_LabelPx / fm.bakeSize;
			const float textH = fm.lineHeight * scale;
			Rect2D tb = b;
			const float yOff = (tb.Height() - textH) * 0.5f;
			tb.min.y += yOff;
			tb.max.y -= yOff;
			r.DrawText(tb, buf, *m_Font, m_LabelPx, m_LabelColor,
					   UIRenderer::TextAlignH::Center);
		}
	}

} // namespace Onyx::UI
