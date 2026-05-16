#pragma once

#include "Source/UI/Color.h"
#include "Source/UI/Widget.h"

namespace Onyx::UI {

	class DragContext;

	// Top-most layer that paints the drag ghost following the cursor while a
	// drag is in flight. Add it last in the HUD tree (it draws after everything
	// else). Non-interactive — it never intercepts input.
	class DragGhostLayer : public Widget
	{
	public:
		explicit DragGhostLayer(DragContext* ctx = nullptr);

		void SetContext(DragContext* c) { m_Ctx = c; }
		void SetGhostOpacity(float a)   { m_Opacity = a; }

		void OnDraw(UIRenderer& r) override;

	private:
		DragContext* m_Ctx = nullptr;
		float m_Opacity = 0.85f;
	};

} // namespace Onyx::UI
