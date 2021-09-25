#pragma once

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "Maths/Maths.h"

namespace se {

	class Window
	{
	public:
		Window(const char* title = "Temporal name", uint32_t width = 1280, uint32_t height = 720, float aspectRatio = 16.0f / 9.0f);
		~Window();

		bool Closed() const;
		void Update();
		void Clear();

		int Width() const { return m_Width;  }
		int Height() const { return m_Height; }

		float GetSeconds() const { return m_CurrentTime - m_LastTime; }
		float GetMilliseconds() const { return (m_CurrentTime - m_LastTime) * 1000; }
		void ShowFPS(bool value = 0);
		void SetVSync(bool value = 0) const;

		lptm::Vector2D GetWindowSize() const { return lptm::Vector2D(m_Width, m_Height); }
		lptm::Vector2D GetWidowPos() const { return lptm::Vector2D(m_WindowPosX, m_WindowPosY); }

		float GetAspectRatio() const { return m_AspectRatio; }

		void CloseWindow() const;
		GLFWwindow* WindowGUI() { return m_Window; }
	public:
		bool IsKeyPressed(uint32_t keyCode) const;
		bool IsButtomPressed(uint32_t button) const;
		bool IsButtomJustPressed(uint32_t button) const;

		bool IsMouseMoving() const { return m_IsMouseMoving; }
	
		lptm::Vector2D GetMousePos() const { return lptm::Vector2D(m_XMousePos, m_YMousePos); };
		double GetMouseX() const { return m_XMousePos; }
		double GetMouseY() const { return m_YMousePos; }
	private:
		const char* m_Title;
		int m_Height, m_Width;
		float m_AspectRatio;
		float m_LastTime = 0.0f, m_CurrentTime = 0.0f;
		int m_FPS = 0;
		bool m_ShowFPS = false;
		bool m_Closed;

		float curtime, lasttime;
		GLFWwindow* m_Window;
	private:
		bool Init();
		void HandleResize();

		friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		friend void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
		friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
		friend void window_pos_callback(GLFWwindow* window, int xpos, int ypos);

		bool m_Keys[1024];
		bool m_MouseButtons[32];
		bool m_MouseButtonsJustPressed[32];
		bool m_IsMouseMoving;

		uint32_t m_WindowPosX, m_WindowPosY;
		double m_XMousePos, m_YMousePos;
	};

} 