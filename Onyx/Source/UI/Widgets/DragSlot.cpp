#include "DragSlot.h"

#include "Source/UI/Render/UIRenderer.h"

#include <algorithm>

namespace Onyx::UI {

	DragSlot::DragSlot()
	{
		SetInteractive(true); // receive press/drag events from the tree
	}

	void DragSlot::SetContext(DragContext* c)
	{
		if (m_Ctx == c)
			return;
		if (m_Ctx)
			m_Ctx->UnregisterSlot(this);
		m_Ctx = c;
		if (m_Ctx)
			m_Ctx->RegisterSlot(this);
	}

	void DragSlot::OnDetach()
	{
		if (m_Ctx)
			m_Ctx->UnregisterSlot(this);
	}

	void DragSlot::SetItem(const DragPayload& p, Onyx::Texture* tex,
						   glm::vec2 uvMin, glm::vec2 uvMax)
	{
		m_Payload = p;
		m_Tex     = tex;
		m_UvMin   = uvMin;
		m_UvMax   = uvMax;
		m_Empty   = false;
	}

	void DragSlot::Clear()
	{
		m_Empty = true;
		m_Tex   = nullptr;
		m_Payload = DragPayload{};
	}

	bool DragSlot::Accepts(const DragPayload& p) const
	{
		return m_Accept ? m_Accept(p) : true;
	}

	void DragSlot::ReceiveDrop(const DragPayload& p, uint16_t mods)
	{
		if (m_OnDrop)
			m_OnDrop(p, mods);
	}

	bool DragSlot::IsDragSource() const
	{
		return m_Ctx && m_Ctx->IsDragging() && m_Ctx->Source() == this;
	}

	bool DragSlot::IsHotTarget() const
	{
		return m_Ctx && m_Ctx->IsDragging() && m_Ctx->Source() != this &&
			   Accepts(m_Ctx->Payload()) &&
			   GetBounds().Contains(m_Ctx->Cursor());
	}

	void DragSlot::OnInput(const InputEvent& e)
	{
		if (!m_Ctx)
			return;

		switch (e.type)
		{
			case InputEventType::MouseDragBegin:
				if (!m_Empty)
				{
					const glm::vec2 ghostSize{GetBounds().Width(),
											  GetBounds().Height()};
					m_Ctx->Begin(this, m_Payload, m_Tex, m_UvMin, m_UvMax,
								 ghostSize);
				}
				break;

			case InputEventType::MouseDrag:
				if (IsDragSource())
					m_Ctx->UpdateCursor(e.position);
				break;

			case InputEventType::MouseDragEnd:
				if (IsDragSource())
					m_Ctx->Drop(e.mods);
				break;

			default:
				break;
		}
	}

	void DragSlot::OnDraw(UIRenderer& r)
	{
		const Rect2D b = GetBounds();
		if (b.Empty())
			return;

		// Frame: border ring + inset background.
		r.DrawRoundedRect(b, m_Border, m_Radius);
		r.DrawRoundedRect(b.Inset(1.0f), m_Bg, std::max(0.0f, m_Radius - 1.0f));

		// Icon — hidden while this slot is the active drag source (it's
		// "picked up" and follows the cursor as the ghost).
		if (!m_Empty && m_Tex && !IsDragSource())
			r.DrawTextureRegion(b.Inset(3.0f), m_Tex, m_UvMin, m_UvMax);

		// Compatible-drop affordance.
		if (IsHotTarget())
		{
			const float t = 2.0f;
			r.DrawRect(Rect2D{{b.min.x, b.min.y}, {b.max.x, b.min.y + t}}, m_Highlight);
			r.DrawRect(Rect2D{{b.min.x, b.max.y - t}, {b.max.x, b.max.y}}, m_Highlight);
			r.DrawRect(Rect2D{{b.min.x, b.min.y}, {b.min.x + t, b.max.y}}, m_Highlight);
			r.DrawRect(Rect2D{{b.max.x - t, b.min.y}, {b.max.x, b.max.y}}, m_Highlight);
		}
	}

} // namespace Onyx::UI
