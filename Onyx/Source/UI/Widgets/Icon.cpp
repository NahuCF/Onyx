#include "Icon.h"

#include "Source/UI/Render/UIRenderer.h"

namespace Onyx::UI {

	void Icon::OnDraw(UIRenderer& r)
	{
		if (!m_Texture)
			return;

		const Rect2D bounds = GetBounds();
		if (bounds.Empty())
			return;

		// Disabled icons darken but keep their alpha so the slot frame still
		// reads through.
		Color tint = m_Tint;
		if (!m_Enabled)
		{
			tint.r *= 0.35f;
			tint.g *= 0.35f;
			tint.b *= 0.35f;
		}

		r.DrawTextureRegion(bounds, m_Texture, m_UvMin, m_UvMax, tint);
	}

} // namespace Onyx::UI
