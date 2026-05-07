#include "ScreenStack.h"

#include <algorithm>

namespace Onyx::UI {

	ScreenStack::ScreenStack() = default;

	ScreenStack::~ScreenStack() = default;

	void ScreenStack::SetScreen(std::unique_ptr<WidgetTree> screen)
	{
		// Discard overlays — replacing the screen invalidates their stack
		// context. Releases capture on any tree that held it.
		m_Screens.clear();
		m_CaptureTree = nullptr;

		ScreenEntry entry;
		entry.tree = std::move(screen);
		entry.layer = ScreenLayer::HUD;
		m_Screens.push_back(std::move(entry));
	}

	WidgetTree* ScreenStack::PushOverlay(std::unique_ptr<WidgetTree> overlay, ScreenLayer layer)
	{
		ScreenEntry entry;
		entry.tree = std::move(overlay);
		entry.layer = layer;
		WidgetTree* raw = entry.tree.get();
		m_Screens.push_back(std::move(entry));
		Resort();
		return raw;
	}

	void ScreenStack::PopOverlay(WidgetTree* overlay)
	{
		if (m_Screens.size() <= 1)
			return;

		if (!overlay)
		{
			// Pop the topmost overlay (last entry, but only if it's not the
			// screen at index 0).
			ScreenEntry& last = m_Screens.back();
			if (last.tree.get() == m_CaptureTree)
				m_CaptureTree = nullptr;
			m_Screens.pop_back();
			Resort();
			return;
		}

		auto it = std::find_if(m_Screens.begin() + 1, m_Screens.end(),
							   [overlay](const ScreenEntry& e) { return e.tree.get() == overlay; });
		if (it == m_Screens.end())
			return;

		if (it->tree.get() == m_CaptureTree)
			m_CaptureTree = nullptr;
		m_Screens.erase(it);
		Resort();
	}

	void ScreenStack::Resort()
	{
		// Keep the active screen at index 0; sort overlays by (layer, push
		// order). std::stable_sort preserves push order within a layer.
		if (m_Screens.size() <= 2)
			return;
		std::stable_sort(m_Screens.begin() + 1, m_Screens.end(),
						 [](const ScreenEntry& a, const ScreenEntry& b) {
							 return static_cast<uint8_t>(a.layer) < static_cast<uint8_t>(b.layer);
						 });
	}

	void ScreenStack::Update(float deltaSeconds)
	{
		// Update top-down so timer state advances in the same order input
		// dispatch will visit them. Tick all trees regardless of visibility.
		for (auto it = m_Screens.rbegin(); it != m_Screens.rend(); ++it)
		{
			if (it->tree)
				it->tree->Update(deltaSeconds);
		}
	}

	void ScreenStack::SetViewport(glm::vec2 viewport)
	{
		for (auto& s : m_Screens)
		{
			if (s.tree)
				s.tree->SetViewport(viewport);
		}
	}

	void ScreenStack::Draw(UIRenderer& renderer)
	{
		// Back-to-front: index 0 (active screen) first, then overlays in
		// stack order. Resort() keeps overlays sorted by layer enum so
		// drawing in vector order is the correct compositing order.
		for (auto& s : m_Screens)
		{
			if (s.tree)
				s.tree->Draw(renderer);
		}
	}

	void ScreenStack::DispatchInput(const InputEvent& e)
	{
		// Capture wins absolutely.
		if (m_CaptureTree)
		{
			m_CaptureTree->DispatchInput(e);
			return;
		}

		// Top-down. Any tree with a non-empty hit chain consumes the event.
		// Mouse events fall through to the next layer if the topmost layer
		// has no interactive target under the pointer.
		for (auto it = m_Screens.rbegin(); it != m_Screens.rend(); ++it)
		{
			WidgetTree* tree = it->tree.get();
			if (!tree || !tree->GetRoot())
				continue;
			tree->DispatchInput(e);

			// For mouse events, "consumed" means an interactive target was
			// hit (hover changed or capture grabbed). For keyboard / char,
			// the focused tree consumes; if no tree has focus, the event is
			// dropped after iterating.
			const bool isMouse =
				e.type == InputEventType::MouseMove ||
				e.type == InputEventType::MouseDown ||
				e.type == InputEventType::MouseUp ||
				e.type == InputEventType::MouseWheel;
			const bool consumed = isMouse ? (tree->GetHover() != nullptr) : (tree->GetFocus() != nullptr);
			if (consumed)
				return;
		}
	}

} // namespace Onyx::UI
