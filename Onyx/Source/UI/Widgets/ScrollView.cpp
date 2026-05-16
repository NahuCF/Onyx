#include "ScrollView.h"

#include "Source/UI/Render/UIRenderer.h"

#include <algorithm>
#include <cmath>

namespace Onyx::UI {

	namespace { constexpr float kBarW = 6.0f; }

	ScrollView::ScrollView()
	{
		SetClipsChildren(true);  // tree pushes a scissor to our bounds
		SetInteractive(true);    // so the wheel routes here
		SetContainerMode(ContainerMode::Free);

		auto content = std::make_unique<Widget>();
		content->SetContainerMode(ContainerMode::StackVertical);
		LayoutSpec& cs = content->GetLayoutSpec();
		cs.widthMode  = SizeMode::Fill;
		cs.heightMode = SizeMode::Fixed;
		cs.height     = 0.0f;
		cs.anchor     = AnchorEdge::TopLeft;
		m_Content = content.get();
		AddChild(std::move(content));
	}

	void ScrollView::AddItem(std::unique_ptr<Widget> item)
	{
		m_Content->AddChild(std::move(item));
	}

	float ScrollView::ContentHeight() const
	{
		const auto& kids = m_Content->GetChildren();
		float total = 0.0f;
		int   n = 0;
		for (const auto& c : kids)
		{
			if (!c->IsVisible()) continue;
			const LayoutSpec& s = c->GetLayoutSpec();
			total += (s.heightMode == SizeMode::Fixed) ? s.height
													   : c->GetBounds().Height();
			++n;
		}
		if (n > 1) total += m_Gap * static_cast<float>(n - 1);
		return total;
	}

	void ScrollView::OnUpdate(float /*dt*/)
	{
		const float viewH    = GetBounds().Height();
		const float contentH = ContentHeight();
		m_MaxScroll = std::max(0.0f, contentH - viewH);

		if (m_PinBottom)
			m_ScrollY = m_MaxScroll;
		m_ScrollY = std::clamp(m_ScrollY, 0.0f, m_MaxScroll);

		// Reserve a right strip for the scrollbar so content doesn't draw under
		// it. padding is {top, right, bottom, left}.
		const float pad = (m_ShowScrollbar && m_MaxScroll > 0.0f) ? kBarW : 0.0f;
		GetLayoutSpec().padding = glm::vec4(0.0f, pad, 0.0f, 0.0f);

		LayoutSpec& cs = m_Content->GetLayoutSpec();
		cs.heightMode   = SizeMode::Fixed;
		cs.height       = std::max(contentH, viewH);
		cs.gap          = m_Gap;
		cs.anchor       = AnchorEdge::TopLeft;
		cs.anchorOffset = glm::vec2(0.0f, -std::round(m_ScrollY));
	}

	void ScrollView::OnInput(const InputEvent& e)
	{
		if (e.type == InputEventType::MouseWheel)
		{
			m_PinBottom = false;
			m_ScrollY = std::clamp(m_ScrollY - e.delta.y * m_WheelStep,
								   0.0f, m_MaxScroll);
		}
	}

	void ScrollView::OnDraw(UIRenderer& r)
	{
		if (!m_ShowScrollbar || m_MaxScroll <= 0.0f)
			return;

		const Rect2D b = GetBounds();
		const float trackH = b.Height();
		const Rect2D track{glm::vec2(b.max.x - kBarW, b.min.y),
						   glm::vec2(b.max.x, b.max.y)};
		r.DrawRect(track, m_BarTrack);

		const float contentH = m_MaxScroll + trackH;
		float thumbH = trackH * (trackH / contentH);
		thumbH = std::max(thumbH, 24.0f);
		const float frac = m_MaxScroll > 0.0f ? m_ScrollY / m_MaxScroll : 0.0f;
		const float thumbY = b.min.y + (trackH - thumbH) * frac;
		const Rect2D thumb{glm::vec2(b.max.x - kBarW, thumbY),
						   glm::vec2(b.max.x, thumbY + thumbH)};
		r.DrawRoundedRect(thumb, m_BarThumb, kBarW * 0.5f);
	}

} // namespace Onyx::UI
