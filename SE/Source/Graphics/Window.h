#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace se {

	void Print();

	class Window
	{
	public:
		Window(const char* title = "Engine", uint32_t width = 1280, uint32_t height = 720);
		~Window();

		void Update();
		bool Closed() const;
		void Clear() const;

		int Width() const { return m_Width;  }
		int Height() const { return m_Height; }

		void FPS();
		void SetVSync(bool value = 0) const;
		GLFWwindow* WindowGUI() { return m_Window; }
	public:
		bool IsKeyPressed(uint32_t keyCode) const;
		bool IsButtomPressed(uint32_t button) const;
		bool IsButtomJustPressed(uint32_t button) const;

		double GetMouseX() const { return m_XMousePos; }
		double GetMouseY() const { return m_YMousePos; }
	private:
		const char* m_Title;
		int m_Height, m_Width;
		int m_LastTime, m_CurrentTime, m_FPS;
		bool m_Closed;
		GLFWwindow* m_Window;
	private:
		bool Init();
		friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		friend void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
		friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
		bool m_Keys[1024];
		bool m_MouseButtons[32];
		bool m_MouseButtonsJustPressed[32];

		double m_XMousePos, m_YMousePos;
	};

} 