#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widgets/WorldAnchoredWidget.h"

#include <glm/glm.hpp>

namespace Onyx { class Texture; }

namespace Onyx::UI {

	// World-anchored sprite/badge: quest "!" / "?" over an NPC, ping, objective
	// marker. Uses a texture region when set; falls back to a solid rounded
	// badge so it's functional before art exists (a real marker, not a hack —
	// swap in the sprite later via SetTexture). Gentle vertical bob optional.
	class WorldMarker : public WorldAnchoredWidget
	{
	public:
		WorldMarker() = default;

		void SetTexture(Onyx::Texture* t)        { m_Texture = t; }
		void SetUV(glm::vec2 mn, glm::vec2 mx)   { m_UvMin = mn; m_UvMax = mx; }
		void SetSize(float w, float h)           { m_Size = {w, h}; }
		void SetColor(const Color& c)            { m_Color = c; }
		void SetBob(float amplitudePx, float speed)
		{ m_BobAmp = amplitudePx; m_BobSpeed = speed; }

		void OnUpdate(float dt) override;
		void OnDraw(UIRenderer& r) override;

	private:
		Onyx::Texture* m_Texture = nullptr;
		glm::vec2 m_UvMin{0.0f, 0.0f};
		glm::vec2 m_UvMax{1.0f, 1.0f};
		glm::vec2 m_Size{28.0f, 28.0f};
		Color m_Color = Color::FromRGBA8(0xFFD24AFF);

		float m_BobAmp   = 4.0f;
		float m_BobSpeed = 3.0f;
		float m_Clock    = 0.0f;
	};

} // namespace Onyx::UI
