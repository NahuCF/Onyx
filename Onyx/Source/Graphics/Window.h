#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Maths/Maths.h"

namespace Onyx {

	class Window
	{
	public:
		Window(const char* title = "Temporal name", uint32_t width = 900, uint32_t height = 900, float aspectRatio = 1.0f, bool fullscreen = false);
		~Window();

		bool ShouldClose() const;
		void Update();
		void Clear();

		int Width() const { return m_Width;  }
		int Height() const { return m_Height; }

		float GetSeconds() const { return m_CurrentTime - m_LastTime; }
		float GetMilliseconds() const { return (m_CurrentTime - m_LastTime) * 1000; }
		void ShowFPS(bool value = 0);
		void SetVSync(bool value = 0) const;

        int GetFramerate() const { return m_LastFPS; }

		Onyx::Vector2D GetWindowSize() const { return Onyx::Vector2D(m_Width, m_Height); }
		Onyx::Vector2D GetWidowPos() const { return Onyx::Vector2D(m_WindowPosX, m_WindowPosY); }

		void SetWindowColor(Onyx::Vector4D color) const { glClearColor(color.x, color.y, color.z, color.w); }

		float GetAspectRatio() const { return m_AspectRatio; }
        //Onyx::Vector2D GetMainMonitorSize() 
        //{ 
            //const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);
            //return { mode->width, mode->height };
        //}

		void CloseWindow() const;
		GLFWwindow* GetWindow() { return m_Window; }

        void MakeFullScreen();
        void MakeWindowed() { m_Monitor = nullptr; }
	public:
		bool IsKeyPressed(uint32_t keyCode) const;
		bool IsButtomPressed(uint32_t button) const;
		bool IsButtomJustPressed(uint32_t button) const;
		bool IsButtomReleased(uint32_t button) const;

		bool IsKeyJustPressed(uint32_t button) const;

		bool IsMouseMoving() const { return m_IsMouseMoving; }
	
		Onyx::Vector2D GetMousePos();
		Onyx::Vector2D GetMousePosInPixels() const { return Onyx::Vector2D(m_XMousePos, m_YMousePos); };
		double GetMouseX() const { return m_XMousePos; }
		double GetMouseY() const { return m_YMousePos; }
	private:
		const char* m_Title;
		int m_Height, m_Width;
        int m_PrimaryMonitorWidth, m_PrimaryMonitorHeight;
		float m_AspectRatio;
		float m_LastTime = 0.0f, m_CurrentTime = 0.0f;
		int m_FPS = 0;
        int m_LastFPS = 0;
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
		bool m_KeysJustPressed[1024];

		bool m_MouseButtons[32];
		bool m_MouseButtonsJustPressed[32];
		bool m_MouseButtonsReleased[32];
		bool m_IsMouseMoving;

		int m_WindowPosX, m_WindowPosY;
		double m_XMousePos, m_YMousePos;

        bool m_FullScreen;

        GLFWmonitor* m_Monitor = nullptr;
	};

} 