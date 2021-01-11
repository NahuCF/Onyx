#include <iostream>
#include <time.h>

#include "Window.h"

namespace se { namespace graphics {

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

	Window::Window(const char* title, unsigned int width, unsigned int height)
	{
		m_Title = title;
		m_Height = height;
		m_Width = width;
		if(!Init())
			glfwTerminate();

		for (int i = 0; i < 1024; i++)
			m_Keys[i] = false;
	};
	
	Window::~Window()
	{
		glfwTerminate();
	}

	bool Window::Init()
	{
		m_LastTime = time(NULL);
		if(!glfwInit())
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
		glfwSetWindowUserPointer(m_Window, this);
		glfwSetKeyCallback(m_Window, key_callback);
		if(glewInit() != GLEW_OK)
		{
			std::cout << "Error to initializate GLEW :c" << std::endl;
			return false;
		}
		return true;
	}

	void Window::Update() 
	{
		glfwGetFramebufferSize(m_Window, &m_Width, &m_Height);
		glViewport(0, 0, m_Width, m_Height);
		glfwSwapBuffers(m_Window);
		glfwPollEvents();
		FPS();
	}

	bool Window::Closed() const
	{
		return glfwWindowShouldClose(m_Window);
	}

	void Window::Clear() const
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Window::FPS()
	{
		m_FPS++;
		m_CurrentTime = time(NULL);
		if(m_CurrentTime - m_LastTime >= 1)
		{
			m_LastTime = m_CurrentTime;
			std::cout << "FPS: " << m_FPS << std::endl;
			m_FPS = 0;
		}
	}

	void Window::Vsync(const char* state)
	{
		if(state == "Enable")
		{
			glfwSwapInterval(1);
		}
		else if (state == "Disable")
		{
			glfwSwapInterval(0);
		}
	}

	bool Window::IsKeyPressed(unsigned int keyCode)
	{
		return m_Keys[keyCode];
	}

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		win->m_Keys[key] = action != GLFW_RELEASE;
	}

}}