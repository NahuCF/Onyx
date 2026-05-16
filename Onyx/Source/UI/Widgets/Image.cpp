#include "Image.h"

#include "Source/Graphics/Texture.h"
#include "Source/UI/Render/UIRenderer.h"

#include <algorithm>

namespace Onyx::UI {

	void Image::OnDraw(UIRenderer& r)
	{
		if (!m_Texture)
			return;

		const Rect2D bounds = GetBounds();
		if (bounds.Empty())
			return;

		if (m_ScaleMode == ScaleMode::Stretch)
		{
			r.DrawTexture(bounds, m_Texture, m_Tint);
			return;
		}

		const auto sz = m_Texture->GetTextureSize();
		const float texW = static_cast<float>(sz.x);
		const float texH = static_cast<float>(sz.y);
		if (texW <= 0.0f || texH <= 0.0f)
		{
			r.DrawTexture(bounds, m_Texture, m_Tint);
			return;
		}

		const float sx = bounds.Width() / texW;
		const float sy = bounds.Height() / texH;
		// Fit: largest scale that still fits (min). Fill: smallest scale that
		// covers (max), then clip the overflow to bounds.
		const float scale = (m_ScaleMode == ScaleMode::Fit)
								 ? std::min(sx, sy)
								 : std::max(sx, sy);

		const glm::vec2 dstSize{texW * scale, texH * scale};
		const glm::vec2 center = bounds.Center();
		const Rect2D dst{center - dstSize * 0.5f, center + dstSize * 0.5f};

		if (m_ScaleMode == ScaleMode::Fill)
		{
			// Cover mode overflows bounds — clip via the renderer's scissor
			// stack so neighbours aren't painted over.
			r.PushScissor(bounds);
			r.DrawTexture(dst, m_Texture, m_Tint);
			r.PopScissor();
		}
		else
		{
			r.DrawTexture(dst, m_Texture, m_Tint);
		}
	}

} // namespace Onyx::UI
