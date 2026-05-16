#include "Nameplate.h"

#include "Source/UI/Render/UIRenderer.h"
#include "Source/UI/Text/FontAtlas.h"

#include <algorithm>

namespace Onyx::UI {

	Nameplate::Nameplate(const FontAtlas* font, float pxSize)
		: m_Font(font), m_PxSize(pxSize)
	{
	}

	void Nameplate::SetHealthFraction(float f01)
	{
		m_Hp = std::clamp(f01, 0.0f, 1.0f);
	}

	void Nameplate::OnUpdate(float dt)
	{
		WorldAnchoredWidget::OnUpdate(dt);

		if (!m_OnScreen || !m_Font)
		{
			SetBounds(Rect2D{});
			return;
		}

		const FontMetrics& fm = m_Font->GetMetrics();
		const float scale  = m_PxSize / fm.bakeSize;
		const float nameH  = fm.lineHeight * scale;
		const float barH   = m_ShowHealth ? 6.0f : 0.0f;
		const float gap    = m_ShowHealth ? 3.0f : 0.0f;
		const float padX   = 8.0f;
		const float padY   = 4.0f;

		const float w = m_BarWidth + padX * 2.0f;
		const float h = nameH + gap + barH + padY * 2.0f;

		// Sit above the anchor point with a small clearance.
		const float cx = m_ScreenPos.x;
		const float bottom = m_ScreenPos.y - 6.0f;
		SetBounds(Rect2D{glm::vec2(cx - w * 0.5f, bottom - h),
						 glm::vec2(cx + w * 0.5f, bottom)});
	}

	void Nameplate::OnDraw(UIRenderer& r)
	{
		if (!m_OnScreen || GetBounds().Empty() || !m_Font || !m_Font->IsLoaded())
			return;

		const Rect2D b = GetBounds();
		r.DrawRoundedRect(b, m_Background, 4.0f);

		const FontMetrics& fm = m_Font->GetMetrics();
		const float scale = m_PxSize / fm.bakeSize;
		const float nameH = fm.lineHeight * scale;
		const float padX  = 8.0f;
		const float padY  = 4.0f;

		Rect2D nameRect{glm::vec2(b.min.x + padX, b.min.y + padY),
						glm::vec2(b.max.x - padX, b.min.y + padY + nameH)};
		r.DrawText(nameRect, m_Name, *m_Font, m_PxSize, m_NameColor,
				   UIRenderer::TextAlignH::Center);

		if (m_ShowHealth)
		{
			const float barH = 6.0f;
			Rect2D track{glm::vec2(b.min.x + padX, nameRect.max.y + 3.0f),
						 glm::vec2(b.max.x - padX, nameRect.max.y + 3.0f + barH)};
			r.DrawRect(track, m_HpTrack);
			Rect2D fill = track;
			fill.max.x = track.min.x + track.Width() * m_Hp;
			r.DrawRect(fill, m_HpFill);
		}
	}

} // namespace Onyx::UI
