#pragma once

#include "Render/UIRenderer.h"
#include "ScreenStack.h"
#include "Text/FontAtlas.h"
#include <glm/glm.hpp>
#include <memory>

namespace Onyx::UI {

	// Top-level facade for the UI library. Owned by Application. Sites that
	// need to drive UI grab `Application::GetUI()` and call SetScreen /
	// PushOverlay / Update / DispatchInput.
	//
	// Render path lives on UIRenderer (M2). M3 will plug the text system in;
	// M5 wires the file watcher; M7 adds binding context.
	class Manager
	{
	public:
		Manager();
		~Manager();

		UIRenderer& GetRenderer() { return *m_Renderer; }

		// Default font atlas. Lazy-loaded on first Render() — the atlas binary
		// is cached at Resources/Fonts/Roboto-Medium.atlas.bin. The first run
		// bakes from Resources/Fonts/Roboto-Medium.ttf using msdfgen + FreeType
		// (a couple seconds); subsequent runs just load the binary.
		FontAtlas& GetDefaultFont() { return *m_DefaultFont; }
		bool DefaultFontLoaded() const { return m_DefaultFont && m_DefaultFont->IsLoaded(); }

		// Viewport size in pixels. Layout and projection use this. Update
		// when the window resizes.
		void SetViewport(glm::ivec2 viewport) { m_Viewport = viewport; }
		glm::ivec2 GetViewport() const { return m_Viewport; }

		// Coordination with ImGui: the UILayer sets these per frame from
		// ImGui's WantCaptureMouse / WantCaptureKeyboard. When set, mouse /
		// keyboard events are dropped before reaching widgets.
		void SetImGuiHasMouse(bool yes) { m_ImGuiHasMouse = yes; }
		void SetImGuiHasKeyboard(bool yes) { m_ImGuiHasKeyboard = yes; }
		bool ImGuiHasMouse() const { return m_ImGuiHasMouse; }
		bool ImGuiHasKeyboard() const { return m_ImGuiHasKeyboard; }

		// Screen-stack passthrough. Widget trees are constructed by callers
		// (data-driven loader in M5; programmatic in C++ today).
		void SetScreen(std::unique_ptr<WidgetTree> screen) { m_Stack.SetScreen(std::move(screen)); }
		WidgetTree* PushOverlay(std::unique_ptr<WidgetTree> overlay, ScreenLayer layer = ScreenLayer::Modal)
		{
			return m_Stack.PushOverlay(std::move(overlay), layer);
		}
		void PopOverlay(WidgetTree* overlay = nullptr) { m_Stack.PopOverlay(overlay); }

		ScreenStack& GetScreenStack() { return m_Stack; }
		const ScreenStack& GetScreenStack() const { return m_Stack; }

		// Frame phases — UILayer drives these. Call order per frame:
		//   1. Network / sim / animation (outside the UI library)
		//   2. Update(dt)
		//   3. DispatchInput(events...) — one call per platform event
		//   4. Render() (no-op in M1; real renderer wires up in M2)
		void Update(float deltaSeconds);
		void DispatchInput(const InputEvent& e);
		void Render(); // M1: no-op. M2: drives UIRenderer.

	private:
		ScreenStack m_Stack;
		std::unique_ptr<UIRenderer> m_Renderer;
		std::unique_ptr<FontAtlas>  m_DefaultFont;
		glm::ivec2 m_Viewport{0, 0};
		bool m_ImGuiHasMouse = false;
		bool m_ImGuiHasKeyboard = false;
		bool m_RendererInitialized = false;
		bool m_FontInitialized = false;
	};

} // namespace Onyx::UI
