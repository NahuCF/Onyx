#include "WorldAnchoredWidget.h"

#include "Source/Graphics/WorldUIAnchorSystem.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Onyx::UI {

	void WorldAnchoredWidget::OnUpdate(float /*dt*/)
	{
		if (!m_Anchor)
		{
			m_OnScreen = false;
			return;
		}

		// Anchor at an arbitrary world point: pass a translation matrix as the
		// entity transform with no socket/attachments — projects the origin.
		const glm::mat4 model =
			glm::translate(glm::mat4(1.0f), m_WorldPos + m_WorldOffset);

		Onyx::WorldUIAnchorSystem::AnchorRequest req;
		req.entityId    = m_EntityId; // 0 = anonymous, never cached
		req.socketSlot  = 0xFF;
		req.modelMatrix = &model;     // valid for the synchronous Project call
		req.mode        = Onyx::WorldUIAnchorSystem::AnchorMode::Track;

		const Onyx::WorldUIAnchorSystem::Sample s = m_Anchor->Project(req);
		m_OnScreen  = s.visible;
		m_ScreenPos = s.screenPos;
		m_Depth     = s.depth;
	}

} // namespace Onyx::UI
