#include "Widget.h"

#include <algorithm>

namespace Onyx::UI {

	void Widget::AddChild(std::unique_ptr<Widget> child)
	{
		if (!child)
			return;
		child->SetParent(this);
		m_Children.push_back(std::move(child));
		m_Children.back()->OnAttach();
	}

	std::unique_ptr<Widget> Widget::RemoveChild(Widget* child)
	{
		if (!child)
			return nullptr;
		auto it = std::find_if(m_Children.begin(), m_Children.end(),
							   [child](const std::unique_ptr<Widget>& c) { return c.get() == child; });
		if (it == m_Children.end())
			return nullptr;
		std::unique_ptr<Widget> detached = std::move(*it);
		m_Children.erase(it);
		detached->OnDetach();
		detached->SetParent(nullptr);
		return detached;
	}

	Widget* Widget::FindById(const std::string& id)
	{
		if (m_Id == id)
			return this;
		for (const auto& c : m_Children)
		{
			if (Widget* hit = c->FindById(id))
				return hit;
		}
		return nullptr;
	}

	Widget* Widget::HitTest(glm::vec2 pos)
	{
		if (!m_Visible || !m_Bounds.Contains(pos))
			return nullptr;

		// Iterate children in reverse z-order so topmost wins. Pure tree
		// order is the default (later siblings on top); explicit z-order
		// values override. We build a transient sorted index per call —
		// children counts are typically small enough that this is cheap and
		// avoids any per-tree-mutation invalidation bookkeeping.
		struct Indexed
		{
			Widget* w;
			int z;
			size_t order;
		};
		std::vector<Indexed> sorted;
		sorted.reserve(m_Children.size());
		for (size_t i = 0; i < m_Children.size(); ++i)
		{
			Widget* c = m_Children[i].get();
			if (!c->IsVisible()) continue;
			if (c->ClipEscape()) continue; // tested separately by the screen stack
			sorted.push_back({c, c->GetZOrder(), i});
		}
		std::stable_sort(sorted.begin(), sorted.end(),
						 [](const Indexed& a, const Indexed& b) {
							 if (a.z != b.z) return a.z < b.z;
							 return a.order < b.order;
						 });

		// Topmost first.
		for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
		{
			Widget* c = it->w;
			// If parent clips children, skip children outside parent bounds.
			if (m_ClipsChildren && !m_Bounds.Intersects(c->GetBounds()))
				continue;
			if (Widget* hit = c->HitTest(pos))
				return hit;
		}

		return this;
	}

} // namespace Onyx::UI
