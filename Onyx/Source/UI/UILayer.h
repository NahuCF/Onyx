#pragma once

#include "Source/Core/Layer.h"
#include "Source/UI/Manager.h"
#include <glm/glm.hpp>

namespace Onyx {
	class Window;
}

namespace Onyx::UI {

	// The Onyx::Layer that drives the UI manager. Pulls input from the active
	// Window via polling each frame, synthesizes InputEvents, and feeds them
	// to Manager. Calls Manager::Update + Render in OnUpdate.
	//
	// Frame ordering (Application::Run):
	//   1. user layers' OnUpdate (3D rendering)
	//   2. UILayer::OnUpdate                <-- this layer
	//   3. ImGuiLayer::Begin / OnImGui / End
	//   4. Window swap
	//
	// Polling is fine for M1 — mouse position, button state, key state. Char
	// events / wheel events / repeat keys need the GLFW callback path; that
	// hookup is deferred to M5/M9 when text input + scrollwheel widgets land.
	class UILayer : public Onyx::Layer
	{
	public:
		UILayer(Manager& manager, Onyx::Window& window);
		~UILayer() override = default;

		void OnAttach() override;
		void OnUpdate() override;
		void OnImGui() override;

	private:
		void PollAndDispatch();
		void UpdateImGuiCaptureFlags();
		float ConsumeFrameSeconds();

		Manager& m_Manager;
		Onyx::Window& m_Window;

		// Polling state — used to synthesize edge events (Down/Up/Move/etc.)
		// from the polled Window state.
		glm::vec2 m_PrevMouse{0.0f};
		bool m_PrevMouseValid = false;
		bool m_PrevButtons[5] = {false, false, false, false, false};

		double m_PrevFrameTime = 0.0;
		bool   m_PrevFrameTimeValid = false;
	};

} // namespace Onyx::UI
