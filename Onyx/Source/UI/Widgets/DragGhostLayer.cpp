#include "DragGhostLayer.h"

#include "Source/UI/DragDrop.h"
#include "Source/UI/Render/UIRenderer.h"

namespace Onyx::UI {

	DragGhostLayer::DragGhostLayer(DragContext* ctx)
		: m_Ctx(ctx)
	{
		FillSize(); // span the viewport; never interactive
	}

	void DragGhostLayer::OnDraw(UIRenderer& r)
	{
		if (!m_Ctx || !m_Ctx->IsDragging())
			return;

		const glm::vec2 c    = m_Ctx->Cursor();
		const glm::vec2 size = m_Ctx->GhostSize();
		const Rect2D rect{c - size * 0.5f, c + size * 0.5f};

		Color tint = Color::White();
		tint.a = m_Opacity;

		if (m_Ctx->GhostTexture())
			r.DrawTextureRegion(rect, m_Ctx->GhostTexture(),
								m_Ctx->GhostUvMin(), m_Ctx->GhostUvMax(), tint);
		else
			r.DrawRoundedRect(rect, Color{1.0f, 1.0f, 1.0f, 0.35f * m_Opacity},
							  4.0f);
	}

} // namespace Onyx::UI
