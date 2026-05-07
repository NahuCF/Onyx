#include "DebugRect.h"

#include "Source/UI/Render/UIRenderer.h"

#include <algorithm>
#include <iostream>

namespace Onyx::UI {

	DebugRect::DebugRect(const Color& base, const std::string& label)
		: m_Base(base), m_Label(label)
	{
		SetInteractive(true);
		SetFocusable(true);
	}

	void DebugRect::OnDraw(UIRenderer& r)
	{
		Color c = m_Base;
		if (IsPressed())
		{
			// Mute toward black on press for a clear visual.
			c.r *= 0.55f;
			c.g *= 0.55f;
			c.b *= 0.55f;
		}
		else if (IsHovered())
		{
			// Brighten on hover.
			c.r = std::min(1.0f, c.r + 0.15f);
			c.g = std::min(1.0f, c.g + 0.15f);
			c.b = std::min(1.0f, c.b + 0.15f);
		}

		r.DrawRoundedRect(GetBounds(), c, 6.0f);

		// Outline. 1px insets emit four tiny rects forming a frame; not the
		// most efficient but it's debug-only and validates clipping behavior.
		const Color outline = IsFocused() ? Color{1.0f, 1.0f, 0.4f, 1.0f}
										  : Color{0.0f, 0.0f, 0.0f, 0.6f};
		const Rect2D b = GetBounds();
		r.DrawRect(Rect2D{{b.min.x, b.min.y}, {b.max.x, b.min.y + 1.0f}}, outline);
		r.DrawRect(Rect2D{{b.min.x, b.max.y - 1.0f}, {b.max.x, b.max.y}}, outline);
		r.DrawRect(Rect2D{{b.min.x, b.min.y}, {b.min.x + 1.0f, b.max.y}}, outline);
		r.DrawRect(Rect2D{{b.max.x - 1.0f, b.min.y}, {b.max.x, b.max.y}}, outline);
	}

	void DebugRect::OnInput(const InputEvent& e)
	{
		if (e.type == InputEventType::MouseUp && e.button == MouseButton::Left && IsHovered())
		{
			++m_Clicks;
			std::cout << "[UI] " << m_Label << " click " << m_Clicks
					  << " mods=" << e.mods << '\n';
		}
		else if (e.type == InputEventType::MouseDoubleClick && e.button == MouseButton::Left)
		{
			std::cout << "[UI] " << m_Label << " double-click\n";
		}
	}

} // namespace Onyx::UI
