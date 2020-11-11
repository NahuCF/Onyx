#pragma once

#include <GLFW/glfw3.h>

namespace se {namespace graphics {

	class Window
	{
	public:
		Window(const char* title, unsigned int height, unsigned int width);
		~Window();
		void Update() const;
		bool Closed() const;
	private:
		const char* m_Title;
		unsigned int m_Height, m_Width;
		bool m_Closed;
		GLFWwindow* m_Window;
	private:
		bool Init();
	};
} }