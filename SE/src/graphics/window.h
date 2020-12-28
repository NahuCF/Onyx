#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace se {namespace graphics {

	class Window
	{
	public:
		Window(const char* title, unsigned int width, unsigned int height);
		~Window();
		void Update();
		bool Closed() const;
		void Clear() const;
		int Width() const { return m_Width;  }
		int Height() const { return m_Height; }
		void FPS();
		void Vsync(const char *state);
	public:
		bool IsKeyPressed(unsigned int keyCode);
	private:
		const char* m_Title;
		int m_Height, m_Width;
		int m_LastTime, m_CurrentTime, m_FPS;
		bool m_Closed;
		GLFWwindow* m_Window;
	private:
		bool Init();
		friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		bool m_Keys[1024];
	};

} }