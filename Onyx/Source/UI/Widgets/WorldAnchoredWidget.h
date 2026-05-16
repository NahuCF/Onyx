#pragma once

#include "Source/UI/Widget.h"

#include <cstdint>
#include <glm/glm.hpp>

namespace Onyx { class WorldUIAnchorSystem; }

namespace Onyx::UI {

	// Base for HUD widgets pinned to a world position (damage numbers,
	// nameplates, quest markers). The game feeds a world position each frame —
	// the library stays decoupled from entity/bone/socket systems; it only
	// needs the engine's WorldUIAnchorSystem to project + frustum/distance cull.
	//
	// Subclasses are self-drawing leaves (no child widgets): call the base
	// OnUpdate to refresh projection, then size their own bounds around
	// m_ScreenPos and paint in OnDraw. IsVisible() stays true so OnUpdate keeps
	// running; off-screen frames collapse bounds to empty so hit-testing and
	// drawing skip them without freezing the update loop.
	class WorldAnchoredWidget : public Widget
	{
	public:
		void SetAnchorSystem(Onyx::WorldUIAnchorSystem* s) { m_Anchor = s; }
		void SetWorldPosition(const glm::vec3& p)          { m_WorldPos = p; }
		// Extra world-space offset (e.g. +Z to sit above a creature's head).
		void SetWorldOffset(const glm::vec3& o)            { m_WorldOffset = o; }
		// Optional stable entity id so the projection cache can hit.
		void SetEntityId(uint64_t id)                      { m_EntityId = id; }

		bool IsOnScreen() const { return m_OnScreen; }

		void OnUpdate(float dt) override;

	protected:
		Onyx::WorldUIAnchorSystem* m_Anchor = nullptr;
		glm::vec3 m_WorldPos{0.0f};
		glm::vec3 m_WorldOffset{0.0f};
		uint64_t  m_EntityId = 0;

		bool      m_OnScreen = false;
		glm::vec2 m_ScreenPos{0.0f};
		float     m_Depth = 0.0f;
	};

} // namespace Onyx::UI
