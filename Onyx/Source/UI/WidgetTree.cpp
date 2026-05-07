#include "WidgetTree.h"

#include "Render/UIRenderer.h"

#include <algorithm>
#include <cmath>

namespace Onyx::UI {

	WidgetTree::WidgetTree() = default;

	WidgetTree::~WidgetTree()
	{
		// Detach pointer fields before the root is destroyed; OnDetach handlers
		// running mid-destruction shouldn't be able to call back into us with
		// dangling pointers.
		m_Hovered = nullptr;
		m_Focused = nullptr;
		m_Capture = nullptr;
		m_PressTarget = nullptr;
	}

	void WidgetTree::SetRoot(std::unique_ptr<Widget> root)
	{
		m_Hovered = nullptr;
		m_Focused = nullptr;
		m_Capture = nullptr;
		m_PressTarget = nullptr;
		m_PressActive = false;
		m_DragActive = false;
		m_HoverIntentFired = false;

		m_Root = std::move(root);
		if (m_Root)
			m_Root->OnAttach();
	}

	std::unique_ptr<Widget> WidgetTree::ReleaseRoot()
	{
		m_Hovered = nullptr;
		m_Focused = nullptr;
		m_Capture = nullptr;
		m_PressTarget = nullptr;
		return std::move(m_Root);
	}

	void WidgetTree::Update(float deltaSeconds)
	{
		m_TimeSeconds += deltaSeconds;

		if (!m_Root)
			return;

		PerformLayout();

		// Walk the tree, calling OnUpdate on every visible widget.
		std::vector<Widget*> stack;
		stack.push_back(m_Root.get());
		while (!stack.empty())
		{
			Widget* w = stack.back();
			stack.pop_back();
			if (!w->IsVisible()) continue;
			w->OnUpdate(deltaSeconds);
			for (const auto& c : w->GetChildren())
				stack.push_back(c.get());
		}

		// Hover-intent: pointer dwelt past threshold without moving.
		if (m_Hovered && !m_HoverIntentFired)
		{
			m_HoverDwell += deltaSeconds;
			if (m_HoverDwell >= m_Thresholds.hoverIntentSeconds)
			{
				InputEvent e{};
				e.type = InputEventType::HoverIntent;
				e.position = m_LastPointer;
				e.mods = m_LastMods;
				DispatchToWidget(m_Hovered, e);
				m_HoverIntentFired = true;
			}
		}

		// Long-press: button held without escalating to drag.
		if (m_PressActive && !m_DragActive && !m_LongPressFired && m_PressTarget)
		{
			m_PressDwell += deltaSeconds;
			if (m_PressDwell >= m_Thresholds.longPressSeconds)
			{
				InputEvent e{};
				e.type = InputEventType::MouseLongPress;
				e.position = m_LastPointer;
				e.button = m_PressButton;
				e.mods = m_LastMods;
				DispatchToWidget(m_PressTarget, e);
				m_LongPressFired = true;
			}
		}
	}

	Widget* WidgetTree::InteractiveAncestor(Widget* w)
	{
		while (w && !w->IsInteractive())
			w = w->GetParent();
		return w;
	}

	void WidgetTree::SetHover(Widget* w, ModifierFlags mods)
	{
		if (w == m_Hovered)
			return;

		if (m_Hovered)
		{
			m_Hovered->m_Hovered = false;
			InputEvent e{};
			e.type = InputEventType::HoverLeave;
			e.position = m_LastPointer;
			e.mods = mods;
			DispatchToWidget(m_Hovered, e);
		}

		m_Hovered = w;
		m_HoverDwell = 0.0f;
		m_HoverIntentFired = false;

		if (m_Hovered)
		{
			m_Hovered->m_Hovered = true;
			InputEvent e{};
			e.type = InputEventType::HoverEnter;
			e.position = m_LastPointer;
			e.mods = mods;
			DispatchToWidget(m_Hovered, e);
		}
	}

	void WidgetTree::SetFocus(Widget* w)
	{
		if (w == m_Focused)
			return;

		if (m_Focused)
		{
			m_Focused->m_Focused = false;
			InputEvent e{};
			e.type = InputEventType::FocusLeave;
			DispatchToWidget(m_Focused, e);
		}

		m_Focused = w;

		if (m_Focused)
		{
			m_Focused->m_Focused = true;
			InputEvent e{};
			e.type = InputEventType::FocusEnter;
			DispatchToWidget(m_Focused, e);
		}
	}

	void WidgetTree::CollectFocusables(Widget* root, std::vector<Widget*>& out) const
	{
		if (!root || !root->IsVisible())
			return;
		if (root->IsFocusable())
			out.push_back(root);
		for (const auto& c : root->GetChildren())
			CollectFocusables(c.get(), out);
	}

	void WidgetTree::MoveFocus(int direction)
	{
		if (!m_Root)
			return;

		std::vector<Widget*> focusables;
		CollectFocusables(m_Root.get(), focusables);
		if (focusables.empty())
		{
			SetFocus(nullptr);
			return;
		}

		int idx = -1;
		for (int i = 0; i < (int)focusables.size(); ++i)
		{
			if (focusables[i] == m_Focused)
			{
				idx = i;
				break;
			}
		}

		const int n = static_cast<int>(focusables.size());
		if (idx < 0)
		{
			// No prior focus: pick first (forward) or last (backward).
			SetFocus(direction >= 0 ? focusables.front() : focusables.back());
			return;
		}
		idx = (idx + direction + n) % n;
		SetFocus(focusables[idx]);
	}

	void WidgetTree::SetCapture(Widget* w)
	{
		if (w == m_Capture)
			return;
		ReleaseCapture(false);
		m_Capture = w;
		if (m_Capture)
			m_Capture->m_Capture = true;
	}

	void WidgetTree::ReleaseCapture(bool silent)
	{
		if (!m_Capture)
			return;
		Widget* prev = m_Capture;
		m_Capture = nullptr;
		prev->m_Capture = false;
		if (!silent)
			prev->OnCaptureLost();
	}

	void WidgetTree::DispatchToWidget(Widget* target, const InputEvent& e)
	{
		if (!target)
			return;
		target->OnInput(e);
	}

	void WidgetTree::DispatchInput(const InputEvent& e)
	{
		if (!m_Root)
			return;

		switch (e.type)
		{
			case InputEventType::MouseMove:
				DispatchMouseMove(e);
				break;
			case InputEventType::MouseDown:
				DispatchMouseDown(e);
				break;
			case InputEventType::MouseUp:
				DispatchMouseUp(e);
				break;
			case InputEventType::MouseWheel:
				DispatchMouseWheel(e);
				break;
			case InputEventType::KeyDown:
			case InputEventType::KeyUp:
				DispatchKey(e);
				break;
			case InputEventType::Char:
				DispatchChar(e);
				break;
			default:
				// Synthesized events (HoverEnter/Leave/Intent, DoubleClick,
				// LongPress, Drag*, FocusEnter/Leave) are produced by the
				// tree itself, not pushed in. Ignore if pushed externally.
				break;
		}
	}

	void WidgetTree::DispatchMouseMove(const InputEvent& e)
	{
		m_LastPointer = e.position;
		m_LastMods = e.mods;

		// Capture short-circuits routing — captured widget gets every move.
		if (m_Capture)
		{
			DispatchToWidget(m_Capture, e);

			// Drag escalation/continuation while captured (e.g. button held
			// throughout). The drag begins once the press has crossed the
			// threshold; from then on every move generates MouseDrag.
			if (m_PressActive && !m_DragActive)
			{
				const float dx = e.position.x - m_PressPos.x;
				const float dy = e.position.y - m_PressPos.y;
				if (std::sqrt(dx * dx + dy * dy) >= m_Thresholds.dragPx)
				{
					m_DragActive = true;
					InputEvent be = e;
					be.type = InputEventType::MouseDragBegin;
					be.button = m_PressButton;
					DispatchToWidget(m_Capture, be);
					InputEvent de = e;
					de.type = InputEventType::MouseDrag;
					de.button = m_PressButton;
					DispatchToWidget(m_Capture, de);
				}
			}
			else if (m_DragActive)
			{
				InputEvent de = e;
				de.type = InputEventType::MouseDrag;
				de.button = m_PressButton;
				DispatchToWidget(m_Capture, de);
			}
			return;
		}

		Widget* hit = m_Root->HitTest(e.position);
		Widget* target = InteractiveAncestor(hit);
		SetHover(target, e.mods);

		if (target)
			DispatchToWidget(target, e);

		// Drag detection on non-captured press.
		if (m_PressActive && !m_DragActive && m_PressTarget)
		{
			const float dx = e.position.x - m_PressPos.x;
			const float dy = e.position.y - m_PressPos.y;
			if (std::sqrt(dx * dx + dy * dy) >= m_Thresholds.dragPx)
			{
				m_DragActive = true;
				InputEvent be = e;
				be.type = InputEventType::MouseDragBegin;
				be.button = m_PressButton;
				DispatchToWidget(m_PressTarget, be);
				InputEvent de = e;
				de.type = InputEventType::MouseDrag;
				de.button = m_PressButton;
				DispatchToWidget(m_PressTarget, de);
			}
		}
		else if (m_DragActive && m_PressTarget)
		{
			InputEvent de = e;
			de.type = InputEventType::MouseDrag;
			de.button = m_PressButton;
			DispatchToWidget(m_PressTarget, de);
		}
	}

	void WidgetTree::DispatchMouseDown(const InputEvent& e)
	{
		m_LastPointer = e.position;
		m_LastMods = e.mods;

		Widget* target = m_Capture;
		if (!target)
		{
			Widget* hit = m_Root->HitTest(e.position);
			target = InteractiveAncestor(hit);
		}

		if (target)
		{
			target->m_Pressed = true;
			DispatchToWidget(target, e);

			if (target->IsFocusable())
				SetFocus(target);

			m_PressTarget = target;
		}
		else
		{
			m_PressTarget = nullptr;
			// Click outside any interactive widget releases focus.
			SetFocus(nullptr);
		}

		m_PressButton = e.button;
		m_PressPos = e.position;
		m_PressDwell = 0.0f;
		m_PressActive = true;
		m_DragActive = false;
		m_LongPressFired = false;
	}

	void WidgetTree::DispatchMouseUp(const InputEvent& e)
	{
		m_LastPointer = e.position;
		m_LastMods = e.mods;

		Widget* target = m_Capture ? m_Capture : m_PressTarget;

		if (m_DragActive && target)
		{
			InputEvent de = e;
			de.type = InputEventType::MouseDragEnd;
			de.button = m_PressButton;
			DispatchToWidget(target, de);
		}
		else if (target)
		{
			DispatchToWidget(target, e);

			// Double-click coalescing — only if same button + same target +
			// within threshold and roughly the same position.
			const float gap = m_TimeSeconds - m_LastClickTime;
			const float dx = e.position.x - m_LastClickPos.x;
			const float dy = e.position.y - m_LastClickPos.y;
			const bool nearby = std::sqrt(dx * dx + dy * dy) < 5.0f;
			if (gap <= m_Thresholds.doubleClickSeconds &&
				nearby && m_LastClickButton == e.button)
			{
				m_PendingClickCount++;
				if (m_PendingClickCount >= 2)
				{
					InputEvent dc = e;
					dc.type = InputEventType::MouseDoubleClick;
					dc.clickCount = 2;
					DispatchToWidget(target, dc);
					m_PendingClickCount = 0;
				}
			}
			else
			{
				m_PendingClickCount = 1;
			}
			m_LastClickTime = m_TimeSeconds;
			m_LastClickPos = e.position;
			m_LastClickButton = e.button;
		}

		if (target)
			target->m_Pressed = false;

		m_PressTarget = nullptr;
		m_PressActive = false;
		m_DragActive = false;
		m_LongPressFired = false;
	}

	void WidgetTree::DispatchMouseWheel(const InputEvent& e)
	{
		m_LastMods = e.mods;
		Widget* target = m_Capture;
		if (!target)
		{
			Widget* hit = m_Root->HitTest(e.position);
			target = InteractiveAncestor(hit);
		}
		if (target)
			DispatchToWidget(target, e);
	}

	void WidgetTree::DispatchKey(const InputEvent& e)
	{
		m_LastMods = e.mods;
		// Tab / Shift-Tab handled at the tree level for navigation, even
		// without a focused widget; everything else routes to focus.
		if (e.type == InputEventType::KeyDown && e.keyCode == 258 /* GLFW_KEY_TAB */)
		{
			MoveFocus(HasMod(e.mods, ModifierFlag::Shift) ? -1 : +1);
			return;
		}
		if (m_Focused)
			DispatchToWidget(m_Focused, e);
	}

	void WidgetTree::DispatchChar(const InputEvent& e)
	{
		m_LastMods = e.mods;
		if (m_Focused)
			DispatchToWidget(m_Focused, e);
	}

	void WidgetTree::Draw(UIRenderer& renderer)
	{
		if (!m_Root)
			return;
		DrawWidgetRecursive(m_Root.get(), renderer);
	}

	void WidgetTree::PerformLayout()
	{
		if (!m_Root)
			return;
		// Root always fills the viewport unless its bounds were set explicitly
		// to something non-empty (debug screens, sub-region trees). The "is
		// bounds non-empty" check lets opt-out trees skip the auto-fit.
		if (m_Root->GetBounds().Empty())
		{
			m_Root->SetBounds(Rect2D{glm::vec2(0.0f), m_Viewport});
		}
		LayoutChildren(m_Root.get());
	}

	void WidgetTree::LayoutChildren(Widget* parent)
	{
		if (!parent)
			return;

		const LayoutSpec& pSpec = parent->GetLayoutSpec();
		const Rect2D content = parent->GetBounds().Inset(
			pSpec.padding.x, pSpec.padding.y, pSpec.padding.z, pSpec.padding.w);

		switch (parent->GetContainerMode())
		{
			case ContainerMode::Free:
				for (const auto& c : parent->GetChildren())
				{
					Widget* child = c.get();
					if (!child->IsVisible()) continue;
					PlaceFreeChild(child, content);
					LayoutChildren(child);
				}
				break;

			case ContainerMode::StackHorizontal:
				LayoutStack(parent, content, /*horizontal=*/true);
				for (const auto& c : parent->GetChildren())
					LayoutChildren(c.get());
				break;

			case ContainerMode::StackVertical:
				LayoutStack(parent, content, /*horizontal=*/false);
				for (const auto& c : parent->GetChildren())
					LayoutChildren(c.get());
				break;
		}
	}

	namespace {

		// Resolve one axis given its mode + paired axis size (for Aspect).
		float ResolveAxis(SizeMode mode, float specValue, float parentSize, float pairedSize)
		{
			switch (mode)
			{
				case SizeMode::Fixed:	return specValue;
				case SizeMode::Fill:	return parentSize;
				case SizeMode::Percent: return parentSize * specValue;
				case SizeMode::Aspect:	return pairedSize * specValue;
			}
			return specValue;
		}

		glm::vec2 ResolveSize(const LayoutSpec& spec, glm::vec2 parentSize)
		{
			// Aspect needs a paired axis. Resolve the non-aspect axis first.
			glm::vec2 size{0.0f};
			if (spec.widthMode != SizeMode::Aspect)
				size.x = ResolveAxis(spec.widthMode, spec.width, parentSize.x, 0.0f);
			if (spec.heightMode != SizeMode::Aspect)
				size.y = ResolveAxis(spec.heightMode, spec.height, parentSize.y, 0.0f);
			if (spec.widthMode == SizeMode::Aspect)
				size.x = ResolveAxis(spec.widthMode, spec.width, parentSize.x, size.y);
			if (spec.heightMode == SizeMode::Aspect)
				size.y = ResolveAxis(spec.heightMode, spec.height, parentSize.y, size.x);

			// Apply min/max constraints. min beats max so we never produce a
			// negative-sized rect.
			size = glm::min(size, spec.maxSize);
			size = glm::max(size, spec.minSize);
			size = glm::max(size, glm::vec2(0.0f));
			return size;
		}

		glm::vec2 AnchorPosition(AnchorEdge a, const Rect2D& content, glm::vec2 size)
		{
			const float lx = content.min.x;
			const float cx = content.min.x + (content.Width() - size.x) * 0.5f;
			const float rx = content.max.x - size.x;
			const float ty = content.min.y;
			const float cy = content.min.y + (content.Height() - size.y) * 0.5f;
			const float by = content.max.y - size.y;
			switch (a)
			{
				case AnchorEdge::TopLeft:	  return {lx, ty};
				case AnchorEdge::Top:		  return {cx, ty};
				case AnchorEdge::TopRight:	  return {rx, ty};
				case AnchorEdge::Left:		  return {lx, cy};
				case AnchorEdge::Center:	  return {cx, cy};
				case AnchorEdge::Right:		  return {rx, cy};
				case AnchorEdge::BottomLeft:  return {lx, by};
				case AnchorEdge::Bottom:	  return {cx, by};
				case AnchorEdge::BottomRight: return {rx, by};
			}
			return {lx, ty};
		}

	} // namespace

	void WidgetTree::PlaceFreeChild(Widget* child, const Rect2D& content)
	{
		const LayoutSpec& spec = child->GetLayoutSpec();
		const glm::vec2 size = ResolveSize(spec, content.Size());
		const glm::vec2 base = AnchorPosition(spec.anchor, content, size);
		const glm::vec2 origin = base + spec.anchorOffset;
		child->SetBounds(Rect2D{origin, origin + size});
	}

	void WidgetTree::LayoutStack(Widget* parent, const Rect2D& content, bool horizontal)
	{
		// Pass 1: gather visible children, sum their fixed sizes, count flex.
		struct Item
		{
			Widget* w = nullptr;
			float preferred = 0.0f;
			float minSide = 0.0f;
			float maxSide = 1e9f;
			float crossSize = 0.0f;
			float flexGrow = 0.0f;
			bool isFlex = false;
		};
		std::vector<Item> items;
		items.reserve(parent->GetChildren().size());

		const float axisSize  = horizontal ? content.Width() : content.Height();
		const float crossSize = horizontal ? content.Height() : content.Width();
		const float gap = parent->GetLayoutSpec().gap;

		float fixedTotal = 0.0f;
		float flexTotal = 0.0f;
		int visibleCount = 0;

		for (const auto& c : parent->GetChildren())
		{
			Widget* child = c.get();
			if (!child->IsVisible()) continue;
			++visibleCount;
			const LayoutSpec& spec = child->GetLayoutSpec();
			const glm::vec2 sz = ResolveSize(spec,
				horizontal ? glm::vec2(axisSize, crossSize) : glm::vec2(crossSize, axisSize));

			Item it;
			it.w = child;
			it.preferred = horizontal ? sz.x : sz.y;
			it.crossSize = horizontal ? sz.y : sz.x;
			it.minSide = horizontal ? spec.minSize.x : spec.minSize.y;
			it.maxSide = horizontal ? spec.maxSize.x : spec.maxSize.y;
			it.flexGrow = spec.flexGrow;
			it.isFlex = spec.flexGrow > 0.0f;

			if (it.isFlex)
				flexTotal += spec.flexGrow;
			else
				fixedTotal += it.preferred;

			items.push_back(it);
		}

		const float gapTotal = gap * std::max(0, visibleCount - 1);
		float slack = axisSize - fixedTotal - gapTotal;
		if (slack < 0.0f) slack = 0.0f; // overflow: items keep their preferred size

		// Pass 2: compute final per-item axis size + place sequentially.
		float cursor = horizontal ? content.min.x : content.min.y;

		for (Item& it : items)
		{
			float axisLen = it.preferred;
			if (it.isFlex && flexTotal > 0.0f)
			{
				// Flex children share slack proportionally to flexGrow.
				axisLen = (slack * it.flexGrow) / flexTotal;
				if (axisLen < it.minSide) axisLen = it.minSide;
				if (axisLen > it.maxSide) axisLen = it.maxSide;
			}

			Rect2D bounds;
			if (horizontal)
			{
				bounds.min.x = cursor;
				bounds.max.x = cursor + axisLen;
				bounds.min.y = content.min.y;
				bounds.max.y = content.min.y + std::min(it.crossSize, content.Height());
				if (it.w->GetLayoutSpec().heightMode == SizeMode::Fill)
					bounds.max.y = content.max.y;
			}
			else
			{
				bounds.min.y = cursor;
				bounds.max.y = cursor + axisLen;
				bounds.min.x = content.min.x;
				bounds.max.x = content.min.x + std::min(it.crossSize, content.Width());
				if (it.w->GetLayoutSpec().widthMode == SizeMode::Fill)
					bounds.max.x = content.max.x;
			}
			it.w->SetBounds(bounds);
			cursor += axisLen + gap;
		}
	}

	void WidgetTree::DrawWidgetRecursive(Widget* w, UIRenderer& renderer)
	{
		if (!w || !w->IsVisible())
			return;

		w->OnDraw(renderer);

		const bool clip = w->ClipsChildren();
		if (clip)
			renderer.PushScissor(w->GetBounds());

		// Sort children by (z-order, tree order) for predictable compositing,
		// matching the hit-test ordering. Skip clip-escape children — those
		// are floated to the topmost layer by the screen stack.
		struct Indexed
		{
			Widget* w;
			int z;
			size_t order;
		};
		std::vector<Indexed> sorted;
		const auto& kids = w->GetChildren();
		sorted.reserve(kids.size());
		for (size_t i = 0; i < kids.size(); ++i)
		{
			Widget* c = kids[i].get();
			if (!c->IsVisible()) continue;
			if (c->ClipEscape()) continue;
			sorted.push_back({c, c->GetZOrder(), i});
		}
		std::stable_sort(sorted.begin(), sorted.end(),
						 [](const Indexed& a, const Indexed& b) {
							 if (a.z != b.z) return a.z < b.z;
							 return a.order < b.order;
						 });
		for (const Indexed& ix : sorted)
			DrawWidgetRecursive(ix.w, renderer);

		if (clip)
			renderer.PopScissor();
	}

} // namespace Onyx::UI
