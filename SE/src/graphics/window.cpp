#include <iostream>
#include <GLFW/glfw3.h>

#include "window.h"

namespace se {namespace graphics {

	Window::Window(const char* title, unsigned int height, unsigned int width)
	{
		m_Title = title;
		m_Height = height;
		m_Width = width;
		if(!Init())
			glfwTerminate();
	};
	
	Window::~Window()
	{
		glfwTerminate();
	}

	bool Window::Init()
	{
		if (!glfwInit())
		{
			std::cout << "Failed to load GLFW :c" << std::endl;
			return false;
		}
		m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, NULL, NULL);
		if(!m_Window)
		{
			glfwTerminate();
			std::cout << "Fail to create GLFW wondow :c" << std::endl;
			return false;
		}
		glfwMakeContextCurrent(m_Window);
		return true;
	}

	void Window::Update() const
	{
		glfwSwapBuffers(m_Window);
		glfwPollEvents();
	}

	bool Window::Closed() const
	{
		return glfwWindowShouldClose(m_Window);
	}
}}