#include "WorldMarker.h"

#include "Source/UI/Render/UIRenderer.h"

#include <algorithm>
#include <cmath>

namespace Onyx::UI {

	void WorldMarker::OnUpdate(float dt)
	{
		WorldAnchoredWidget::OnUpdate(dt);
		m_Clock += dt;

		if (!m_OnScreen)
		{
			SetBounds(Rect2D{});
			return;
		}

		const float bob = (m_BobAmp > 0.0f)
							   ? std::sin(m_Clock * m_BobSpeed) * m_BobAmp
							   : 0.0f;
		const glm::vec2 c{m_ScreenPos.x, m_ScreenPos.y - bob};
		SetBounds(Rect2D{c - m_Size * 0.5f, c + m_Size * 0.5f});
	}

	void WorldMarker::OnDraw(UIRenderer& r)
	{
		if (!m_OnScreen || GetBounds().Empty())
			return;

		if (m_Texture)
			r.DrawTextureRegion(GetBounds(), m_Texture, m_UvMin, m_UvMax, m_Color);
		else
			// Functional fallback until art lands.
			r.DrawRoundedRect(GetBounds(), m_Color,
							   std::min(m_Size.x, m_Size.y) * 0.3f);
	}

} // namespace Onyx::UI
