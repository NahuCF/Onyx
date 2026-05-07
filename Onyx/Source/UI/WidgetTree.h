#pragma once

#include "InputEvent.h"
#include "Widget.h"
#include <memory>

namespace Onyx::UI {

	class UIRenderer;

	// Manages a single widget hierarchy: input dispatch, hover/focus/capture
	// state machine, drag detection, double-click coalescing, long-press timer,
	// hover-intent timer, and Tab navigation. ScreenStack owns one tree per
	// active screen / overlay.
	class WidgetTree
	{
	public:
		WidgetTree();
		~WidgetTree();

		void SetRoot(std::unique_ptr<Widget> root);
		Widget* GetRoot() const { return m_Root.get(); }
		std::unique_ptr<Widget> ReleaseRoot();

		// Per-frame tick. Drives: layout pass, hover-intent firing, long-press
		// firing, widget OnUpdate. Pass real wall-clock dt — timer thresholds
		// are in seconds.
		void Update(float deltaSeconds);

		// Set the viewport bounds the root widget fills. Required before
		// Update() so PerformLayout has a target rect. Manager passes the
		// platform window's viewport here.
		void SetViewport(glm::vec2 viewport) { m_Viewport = viewport; }
		glm::vec2 GetViewport() const { return m_Viewport; }

		// Run the layout pass without ticking widgets. Useful for warming up
		// initial bounds before the first frame draws. Update() calls this
		// internally.
		void PerformLayout();

		// Walk the visible subtree depth-first, emitting widget geometry via
		// OnDraw. Honors ClipsChildren by pushing a scissor before recursing
		// into children. Clip-escape children (Tooltip / Popup / Toast /
		// ContextMenu) are skipped — ScreenStack collects them and renders
		// them in the topmost layer.
		void Draw(UIRenderer& renderer);

		// Dispatch a single platform input event. Synthesizes higher-level
		// events (HoverEnter/Leave, DoubleClick, DragBegin/Drag/DragEnd) and
		// routes them per Q15 (interactive-ancestor) and the focus rules.
		void DispatchInput(const InputEvent& e);

		// Capture: redirect all subsequent mouse events to `w` regardless of
		// position. Used by drag systems and modal-input grabs. Releasing
		// fires OnCaptureLost on the captured widget unless `silent` is set
		// (e.g. capture released during widget destruction).
		void SetCapture(Widget* w);
		void ReleaseCapture(bool silent = false);
		Widget* GetCapture() const { return m_Capture; }

		// Focus: explicit assignment for keyboard input routing. Fires
		// FocusLeave on the previous widget and FocusEnter on the new.
		void SetFocus(Widget* w);
		Widget* GetFocus() const { return m_Focused; }

		// Tab navigation. +1 = next focusable, -1 = previous (Shift-Tab).
		// Wraps around at the ends.
		void MoveFocus(int direction);

		// Currently hovered interactive widget (the topmost ancestor of the
		// pointer's deepest hit that has IsInteractive() == true).
		Widget* GetHover() const { return m_Hovered; }

		// Tweakable thresholds. Defaults match the design doc.
		InputThresholds& GetThresholds() { return m_Thresholds; }
		const InputThresholds& GetThresholds() const { return m_Thresholds; }

	private:
		// Walk up from `w` until an interactive widget is found, or null.
		static Widget* InteractiveAncestor(Widget* w);

		// Collect all focusable visible widgets in tree-order (depth-first,
		// pre-order, respecting visibility). Used for Tab navigation.
		void CollectFocusables(Widget* root, std::vector<Widget*>& out) const;

		void DispatchToWidget(Widget* target, const InputEvent& e);
		void DispatchMouseMove(const InputEvent& e);
		void DispatchMouseDown(const InputEvent& e);
		void DispatchMouseUp(const InputEvent& e);
		void DispatchMouseWheel(const InputEvent& e);
		void DispatchKey(const InputEvent& e);
		void DispatchChar(const InputEvent& e);

		void SetHover(Widget* w, ModifierFlags mods);

		void DrawWidgetRecursive(Widget* w, UIRenderer& renderer);

		void LayoutChildren(Widget* parent);
		void PlaceFreeChild(Widget* child, const Rect2D& contentArea);
		void LayoutStack(Widget* parent, const Rect2D& contentArea, bool horizontal);

		std::unique_ptr<Widget> m_Root;

		Widget* m_Hovered = nullptr;
		Widget* m_Focused = nullptr;
		Widget* m_Capture = nullptr;
		Widget* m_PressTarget = nullptr; // widget that received the original MouseDown

		InputThresholds m_Thresholds;

		// Pointer state + timers
		glm::vec2 m_LastPointer{0.0f};
		ModifierFlags m_LastMods = 0;

		glm::vec2 m_Viewport{1280.0f, 720.0f};

		float m_HoverDwell = 0.0f;
		bool  m_HoverIntentFired = false;

		// Press / drag state
		MouseButton m_PressButton = MouseButton::Left;
		glm::vec2 m_PressPos{0.0f};
		float m_PressDwell = 0.0f;
		bool  m_PressActive = false;
		bool  m_DragActive = false;
		bool  m_LongPressFired = false;

		// Click coalescing
		float m_TimeSeconds = 0.0f; // monotonically advances with Update()
		float m_LastClickTime = -1.0f;
		glm::vec2 m_LastClickPos{0.0f};
		MouseButton m_LastClickButton = MouseButton::Left;
		uint8_t m_PendingClickCount = 0;
	};

} // namespace Onyx::UI
