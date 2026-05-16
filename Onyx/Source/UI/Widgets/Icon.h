#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widget.h"

#include <glm/glm.hpp>

namespace Onyx { class Texture; }

namespace Onyx::UI {

	// Atlas sub-region image: an action-bar slot icon, item icon, ability
	// glyph. UVs select the sprite within a sheet. Non-owning texture. When
	// disabled (ability on cooldown / no resource) it dims; pair with the
	// cooldown radial overlay for the full action-slot look.
	class Icon : public Widget
	{
	public:
		Icon() = default;
		Icon(Onyx::Texture* tex, glm::vec2 uvMin = {0.0f, 0.0f},
			 glm::vec2 uvMax = {1.0f, 1.0f})
			: m_Texture(tex), m_UvMin(uvMin), m_UvMax(uvMax) {}

		void SetTexture(Onyx::Texture* t)        { m_Texture = t; }
		void SetUV(glm::vec2 mn, glm::vec2 mx)   { m_UvMin = mn; m_UvMax = mx; }
		void SetTint(const Color& c)             { m_Tint = c; }

		// Dim when the action is unavailable. Visual only — input gating is the
		// owner's concern.
		void SetEnabled(bool e) { m_Enabled = e; }
		bool IsEnabled() const  { return m_Enabled; }

		void OnDraw(UIRenderer& r) override;

	private:
		Onyx::Texture* m_Texture = nullptr;
		glm::vec2 m_UvMin{0.0f, 0.0f};
		glm::vec2 m_UvMax{1.0f, 1.0f};
		Color m_Tint = Color::White();
		bool  m_Enabled = true;
	};

} // namespace Onyx::UI
