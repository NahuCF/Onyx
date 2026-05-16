#include "DragDrop.h"

#include "Source/UI/Widgets/DragSlot.h"

#include <algorithm>

namespace Onyx::UI {

	void DragContext::Begin(DragSlot* source, const DragPayload& payload,
							Onyx::Texture* ghostTex, glm::vec2 ghostUvMin,
							glm::vec2 ghostUvMax, glm::vec2 ghostSize)
	{
		m_Active     = true;
		m_Source     = source;
		m_Payload    = payload;
		m_GhostTex   = ghostTex;
		m_GhostUvMin = ghostUvMin;
		m_GhostUvMax = ghostUvMax;
		m_GhostSize  = ghostSize;
	}

	void DragContext::Cancel()
	{
		m_Active   = false;
		m_Source   = nullptr;
		m_GhostTex = nullptr;
	}

	void DragContext::Drop(uint16_t modifiers)
	{
		if (!m_Active)
			return;

		DragSlot* target = nullptr;
		for (DragSlot* s : m_Slots)
		{
			if (!s || s == m_Source)
				continue;
			if (s->GetBounds().Contains(m_Cursor) && s->Accepts(m_Payload))
			{
				target = s;
				break;
			}
		}

		if (target)
			target->ReceiveDrop(m_Payload, modifiers);
		// No target (or none accepted): snap-back — payload not delivered.

		Cancel();
	}

	void DragContext::RegisterSlot(DragSlot* s)
	{
		if (s && std::find(m_Slots.begin(), m_Slots.end(), s) == m_Slots.end())
			m_Slots.push_back(s);
	}

	void DragContext::UnregisterSlot(DragSlot* s)
	{
		m_Slots.erase(std::remove(m_Slots.begin(), m_Slots.end(), s),
					  m_Slots.end());
		// A slot can be destroyed mid-drag; don't leave a dangling source.
		if (m_Source == s)
			Cancel();
	}

} // namespace Onyx::UI
