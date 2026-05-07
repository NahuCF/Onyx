#include "UILayer.h"

#include "Source/Graphics/Window.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace Onyx::UI {

	UILayer::UILayer(Manager& manager, Onyx::Window& window)
		: m_Manager(manager), m_Window(window)
	{
	}

	void UILayer::OnAttach()
	{
		m_Manager.SetViewport(glm::ivec2(m_Window.Width(), m_Window.Height()));
	}

	float UILayer::ConsumeFrameSeconds()
	{
		const double now = glfwGetTime();
		if (!m_PrevFrameTimeValid)
		{
			m_PrevFrameTime = now;
			m_PrevFrameTimeValid = true;
			return 0.0f;
		}
		const float dt = static_cast<float>(now - m_PrevFrameTime);
		m_PrevFrameTime = now;
		return dt;
	}

	void UILayer::UpdateImGuiCaptureFlags()
	{
		// ImGui sets these during the previous frame. Reading them here gives
		// us last-frame's state, which is the standard pattern: a click that
		// lands inside an ImGui window will be captured by ImGui this frame
		// because last frame ImGui already declared that intent.
		ImGuiIO& io = ImGui::GetIO();
		m_Manager.SetImGuiHasMouse(io.WantCaptureMouse);
		m_Manager.SetImGuiHasKeyboard(io.WantCaptureKeyboard);
	}

	static ModifierFlags ReadModifierFlags(Onyx::Window& window)
	{
		ModifierFlags flags = 0;
		if (window.IsKeyPressed(GLFW_KEY_LEFT_SHIFT) || window.IsKeyPressed(GLFW_KEY_RIGHT_SHIFT))
			flags |= static_cast<uint16_t>(ModifierFlag::Shift);
		if (window.IsKeyPressed(GLFW_KEY_LEFT_CONTROL) || window.IsKeyPressed(GLFW_KEY_RIGHT_CONTROL))
			flags |= static_cast<uint16_t>(ModifierFlag::Ctrl);
		if (window.IsKeyPressed(GLFW_KEY_LEFT_ALT) || window.IsKeyPressed(GLFW_KEY_RIGHT_ALT))
			flags |= static_cast<uint16_t>(ModifierFlag::Alt);
		if (window.IsKeyPressed(GLFW_KEY_LEFT_SUPER) || window.IsKeyPressed(GLFW_KEY_RIGHT_SUPER))
			flags |= static_cast<uint16_t>(ModifierFlag::Super);
		return flags;
	}

	void UILayer::PollAndDispatch()
	{
		const ModifierFlags mods = ReadModifierFlags(m_Window);
		const glm::vec2 mouse(static_cast<float>(m_Window.GetMouseX()),
							  static_cast<float>(m_Window.GetMouseY()));

		// Mouse move (only if changed; matches the design's event-driven
		// model — we never synthesize MouseMove for a stationary cursor).
		if (m_PrevMouseValid && mouse != m_PrevMouse)
		{
			InputEvent e{};
			e.type = InputEventType::MouseMove;
			e.position = mouse;
			e.delta = mouse - m_PrevMouse;
			e.mods = mods;
			m_Manager.DispatchInput(e);
		}
		m_PrevMouse = mouse;
		m_PrevMouseValid = true;

		// Mouse buttons. GLFW's mouse button codes map directly to our
		// MouseButton enum (Left=0, Right=1, Middle=2, X1=3, X2=4).
		for (int b = 0; b < 5; ++b)
		{
			const bool now = m_Window.IsButtomPressed(b);
			if (now != m_PrevButtons[b])
			{
				InputEvent e{};
				e.type = now ? InputEventType::MouseDown : InputEventType::MouseUp;
				e.position = mouse;
				e.button = static_cast<MouseButton>(b);
				e.mods = mods;
				m_Manager.DispatchInput(e);
				m_PrevButtons[b] = now;
			}
		}

		// Wheel + char + key events: not implemented via polling. The GLFW
		// callback hookup lands when widgets that need them ship (M5/M9).
	}

	void UILayer::OnUpdate()
	{
		// Refresh viewport in case the window resized.
		m_Manager.SetViewport(glm::ivec2(m_Window.Width(), m_Window.Height()));

		UpdateImGuiCaptureFlags();
		PollAndDispatch();

		const float dt = ConsumeFrameSeconds();
		m_Manager.Update(dt);
		m_Manager.Render();
	}

	void UILayer::OnImGui()
	{
		// No-op for M1. Future debug overlays (UIEditor preview, hover-target
		// outlines, perf counters) can render their ImGui windows here.
	}

} // namespace Onyx::UI
