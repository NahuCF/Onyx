#include "Manager.h"

namespace Onyx::UI {

	Manager::Manager()
	{
		m_Renderer = std::make_unique<UIRenderer>();
		m_DefaultFont = std::make_unique<FontAtlas>();
	}

	Manager::~Manager() = default;

	void Manager::Update(float deltaSeconds)
	{
		m_Stack.SetViewport(glm::vec2(m_Viewport));
		m_Stack.Update(deltaSeconds);
	}

	void Manager::DispatchInput(const InputEvent& e)
	{
		// ImGui-capture coordination: if ImGui's WantCaptureMouse/Keyboard is
		// set this frame, drop the corresponding events before they hit
		// widgets. Synthetic events still propagate normally — they're
		// produced by widget timers, not platform input.
		const bool isMouse =
			e.type == InputEventType::MouseMove ||
			e.type == InputEventType::MouseDown ||
			e.type == InputEventType::MouseUp ||
			e.type == InputEventType::MouseWheel;
		const bool isKey =
			e.type == InputEventType::KeyDown ||
			e.type == InputEventType::KeyUp ||
			e.type == InputEventType::Char;

		if (isMouse && m_ImGuiHasMouse)
			return;
		if (isKey && m_ImGuiHasKeyboard)
			return;

		m_Stack.DispatchInput(e);
	}

	void Manager::Render()
	{
		if (!m_Renderer)
			return;

		// First-frame init: GL context isn't valid in Manager's ctor (it
		// runs before Window::Init in some paths and definitely before any
		// editor framebuffer setup). Lazy-init guarantees a valid context.
		if (!m_RendererInitialized)
		{
			m_Renderer->Init();
			m_RendererInitialized = true;
		}
		if (!m_FontInitialized)
		{
			// First-run cost: ~1-2 s to bake the Latin atlas. Subsequent runs
			// load the cached .atlas.bin in <50 ms.
			m_DefaultFont->LoadOrBake("Resources/Fonts/Roboto-Medium.ttf",
									  "Resources/Fonts/Roboto-Medium.atlas.bin",
									  32.0f, DefaultLatinCharset());
			m_FontInitialized = true;
		}

		if (m_Viewport.x <= 0 || m_Viewport.y <= 0)
			return;

		m_Renderer->BeginFrame(m_Viewport);
		m_Stack.Draw(*m_Renderer);
		m_Renderer->EndFrame();
	}

} // namespace Onyx::UI
