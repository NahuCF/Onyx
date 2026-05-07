#pragma once

#include "InputEvent.h"
#include "Layout.h"
#include "Rect2D.h"
#include <memory>
#include <string>
#include <vector>

namespace Onyx::UI {

	class WidgetTree;
	class UIRenderer;

	// Base class for every UI widget. Owns its children; tree shape is mutable
	// (Add/Remove/Reparent). Widgets have stable string ids for hot-reload
	// preservation — a widget with id "foo" keeps focus / scroll / animation
	// state across `.ui` reloads; widgets without ids are torn down and rebuilt.
	//
	// Hit testing returns the deepest visible widget at a point; the input
	// dispatcher then routes the event to the topmost interactive ancestor
	// per Q15 — children below an interactive ancestor don't independently
	// receive hover/press/focus state.
	class Widget
	{
	public:
		Widget() = default;
		virtual ~Widget() = default;

		Widget(const Widget&) = delete;
		Widget& operator=(const Widget&) = delete;

		// Lifecycle. Default no-ops; subclasses override what they need.
		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(float deltaSeconds) { (void)deltaSeconds; }
		// Subclasses emit geometry through `r`. The default no-op makes
		// containers transparent — they recurse into children automatically
		// via WidgetTree::Draw.
		virtual void OnDraw(UIRenderer& r) { (void)r; }
		// Subclasses handle dispatched events here. Routing happens at the
		// interactive-ancestor level per Q15 — a non-interactive widget never
		// receives input events under default routing.
		virtual void OnInput(const InputEvent& e) { (void)e; }
		// Fired when the tree revokes input capture (e.g. drag ghost is
		// dismissed by hot-reload, modal closes mid-drag). Always paired with
		// a previous capture grant.
		virtual void OnCaptureLost() {}

		// Tree management
		void AddChild(std::unique_ptr<Widget> child);
		std::unique_ptr<Widget> RemoveChild(Widget* child);
		Widget* GetParent() const { return m_Parent; }
		const std::vector<std::unique_ptr<Widget>>& GetChildren() const { return m_Children; }

		// Find a descendant by id (depth-first). Returns nullptr if not found.
		Widget* FindById(const std::string& id);

		// Identity. Setting an id on a widget already in the tree is allowed.
		const std::string& GetId() const { return m_Id; }
		void SetId(std::string id) { m_Id = std::move(id); }

		// Layout-output bounds in screen space. The layout pass sets these;
		// the input dispatcher and renderer read them.
		const Rect2D& GetBounds() const { return m_Bounds; }
		void SetBounds(const Rect2D& r) { m_Bounds = r; }

		// Per-widget layout configuration. Containers consult their children's
		// LayoutSpec to compute child bounds in PerformLayout. Subclasses can
		// override the spec freely — Stack widget, for example, configures its
		// own m_LayoutMode in its constructor.
		LayoutSpec& GetLayoutSpec() { return m_LayoutSpec; }
		const LayoutSpec& GetLayoutSpec() const { return m_LayoutSpec; }
		void SetLayoutSpec(const LayoutSpec& s) { m_LayoutSpec = s; }

		// Container layout mode. Free is the default — children must place
		// themselves via anchor + offset (or have explicit bounds). Stack
		// containers arrange children along an axis.
		ContainerMode GetContainerMode() const { return m_ContainerMode; }
		void SetContainerMode(ContainerMode m) { m_ContainerMode = m; }

		// Convenience setters that compose readably at the call site.
		Widget& Bounds(const Rect2D& r) { m_Bounds = r; return *this; }
		Widget& FixedSize(float w, float h)
		{
			m_LayoutSpec.widthMode = SizeMode::Fixed;
			m_LayoutSpec.width = w;
			m_LayoutSpec.heightMode = SizeMode::Fixed;
			m_LayoutSpec.height = h;
			return *this;
		}
		Widget& FillSize() { m_LayoutSpec.widthMode = SizeMode::Fill;
							 m_LayoutSpec.heightMode = SizeMode::Fill;
							 return *this; }
		Widget& Anchor(AnchorEdge a, glm::vec2 offset = {0.0f, 0.0f})
		{
			m_LayoutSpec.anchor = a;
			m_LayoutSpec.anchorOffset = offset;
			return *this;
		}
		Widget& Padding(float p)
		{
			m_LayoutSpec.padding = glm::vec4(p);
			return *this;
		}
		Widget& Gap(float g) { m_LayoutSpec.gap = g; return *this; }
		Widget& FlexGrow(float g) { m_LayoutSpec.flexGrow = g; return *this; }

		// Visibility hides this widget and its subtree from layout, draw, and
		// hit testing. Cheap toggle, no tree mutation.
		bool IsVisible() const { return m_Visible; }
		void SetVisible(bool v) { m_Visible = v; }

		// Children clipped to this widget's bounds (scrollview, masked panels).
		bool ClipsChildren() const { return m_ClipsChildren; }
		void SetClipsChildren(bool c) { m_ClipsChildren = c; }

		// Tooltip / Popup / Toast / ContextMenu set this so they render in the
		// topmost layer regardless of where they sit in the tree. The screen
		// stack collects clip-escape widgets for the appropriate layer.
		bool ClipEscape() const { return m_ClipEscape; }
		void SetClipEscape(bool c) { m_ClipEscape = c; }

		// Z-order override within the parent. Higher draws later. Default 0
		// means "follow tree order" (later siblings draw above earlier).
		int GetZOrder() const { return m_ZOrder; }
		void SetZOrder(int z) { m_ZOrder = z; }

		// Interactivity: widgets that handle events themselves (`onclick`,
		// `draggable`, etc.) set this so the input dispatcher routes hover /
		// press state to them. Per Q15, a child below an interactive ancestor
		// doesn't independently get hover/press.
		bool IsInteractive() const { return m_Interactive; }
		void SetInteractive(bool i) { m_Interactive = i; }

		// Focusable widgets participate in Tab navigation. Typically text
		// inputs, sliders, buttons. Implies Interactive.
		bool IsFocusable() const { return m_Focusable; }
		void SetFocusable(bool f)
		{
			m_Focusable = f;
			if (f) m_Interactive = true;
		}

		// State flags — read-only externally; managed by WidgetTree.
		bool IsHovered() const { return m_Hovered; }
		bool IsPressed() const { return m_Pressed; }
		bool IsFocused() const { return m_Focused; }
		bool HasCapture() const { return m_Capture; }

		// Hit test against this widget's subtree. Returns the deepest visible
		// widget whose bounds contain `pos`, or nullptr. Respects ClipsChildren
		// (children outside parent bounds are skipped) and visibility. Does
		// NOT walk into ClipEscape children — those are tested by the screen
		// stack's topmost-layer pass first.
		Widget* HitTest(glm::vec2 pos);

	private:
		void SetParent(Widget* p) { m_Parent = p; }

		Widget* m_Parent = nullptr;
		std::vector<std::unique_ptr<Widget>> m_Children;

		std::string m_Id;
		Rect2D m_Bounds;
		LayoutSpec m_LayoutSpec;
		ContainerMode m_ContainerMode = ContainerMode::Free;

		int  m_ZOrder = 0;
		bool m_Visible = true;
		bool m_ClipsChildren = false;
		bool m_ClipEscape = false;

		bool m_Interactive = false;
		bool m_Focusable = false;

		// Tree-managed state. friend grants the tree write access without
		// exposing setters that subclasses could call inappropriately.
		bool m_Hovered = false;
		bool m_Pressed = false;
		bool m_Focused = false;
		bool m_Capture = false;

		friend class WidgetTree;
	};

} // namespace Onyx::UI
