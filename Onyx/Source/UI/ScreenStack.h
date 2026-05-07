#pragma once

#include "WidgetTree.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace Onyx::UI {

	class UIRenderer;

	// Compositing layers, back to front. Drawing iterates low → high; input
	// iterates high → low so the topmost layer gets first chance to consume.
	// Tooltip / Popup / Toast / Drag are reserved for clip-escape widgets the
	// runtime promotes to a top layer regardless of where they sit in the tree.
	enum class ScreenLayer : uint8_t
	{
		World	= 0, // world-attached widgets sit under HUD
		HUD		= 1, // anchored screen elements
		Modal	= 2, // PushOverlay default
		Popup	= 3, // dropdowns / context menus
		Tooltip	= 4, // always topmost short of toasts and drag ghost
		Toast	= 5, // ephemeral notifications
		Drag	= 6, // drag ghost — above everything during a drag
		Count
	};

	struct ScreenEntry
	{
		std::unique_ptr<WidgetTree> tree;
		ScreenLayer layer = ScreenLayer::HUD;
	};

	// Stack of widget trees. The bottom slot is the active "screen" (HUD layer
	// by default); PushOverlay pushes onto the top with a chosen layer.
	// Compositing order is by layer enum then by push order within a layer.
	class ScreenStack
	{
	public:
		ScreenStack();
		~ScreenStack();

		// Replace the active screen. Discards everything above it (matching
		// the design's "screen replace + overlay stack" model).
		void SetScreen(std::unique_ptr<WidgetTree> screen);

		// Push an overlay tree on a chosen layer. Returns a non-owning handle
		// so the caller can pop it later (PopOverlay) or query state.
		WidgetTree* PushOverlay(std::unique_ptr<WidgetTree> overlay, ScreenLayer layer = ScreenLayer::Modal);

		// Pop a specific overlay tree. Silently no-op if the pointer isn't
		// in the stack. Pops the topmost overlay if `overlay == nullptr`.
		void PopOverlay(WidgetTree* overlay = nullptr);

		// Update every tree, top-down (overlays update before screen so timer
		// state stays consistent with input order).
		void Update(float deltaSeconds);

		// Push the current viewport size to every tree. Called by Manager
		// each frame so window resizes propagate before the next layout pass.
		void SetViewport(glm::vec2 viewport);

		// Render trees back-to-front (composition order). Each tree handles
		// its own clipping via WidgetTree::Draw.
		void Draw(UIRenderer& renderer);

		// Dispatch an input event to the top-most tree first; if it doesn't
		// consume (no interactive target hit and no capture), the event falls
		// through. Capture short-circuits — whichever tree owns capture takes
		// every event until it releases.
		void DispatchInput(const InputEvent& e);

		// Iterate trees in render order (back to front) for the renderer.
		// Visitor signature: (const WidgetTree& tree, ScreenLayer layer).
		template <typename Fn>
		void ForEachInRenderOrder(Fn&& fn) const
		{
			for (const auto& s : m_Screens)
			{
				if (s.tree)
					fn(*s.tree, s.layer);
			}
		}

		// Convenience for tests / debug.
		size_t OverlayCount() const { return m_Screens.empty() ? 0 : m_Screens.size() - 1; }
		WidgetTree* GetActiveScreen() { return m_Screens.empty() ? nullptr : m_Screens.front().tree.get(); }

		// Whichever tree currently owns capture (any tree). nullptr if none.
		WidgetTree* GetCaptureTree() const { return m_CaptureTree; }
		void NotifyCaptureGrabbed(WidgetTree* t) { m_CaptureTree = t; }
		void NotifyCaptureReleased(WidgetTree* t)
		{
			if (m_CaptureTree == t) m_CaptureTree = nullptr;
		}

	private:
		// Stable order: index 0 = active screen, 1+ = overlays in push order.
		// Sorting for compositing happens in ForEachInRenderOrder via stable
		// re-sort by layer enum.
		std::vector<ScreenEntry> m_Screens;
		WidgetTree* m_CaptureTree = nullptr;

		void Resort();
	};

} // namespace Onyx::UI
