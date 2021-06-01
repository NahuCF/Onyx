#include "pch.h"
#include <time.h>

#include "Window.h"

namespace se {

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

	Window::Window(const char* title, uint32_t width, uint32_t height)
		: m_Title(title), m_Height(height), m_Width(width)
	{
		if(!Init())
			glfwTerminate();

		SetVSync(1); 

		for(int i = 0; i < 1024; i++)
			m_Keys[i] = false;

		for(int i = 0; i < 32; i++)
			m_MouseButtons[i] = false;

		for(int i = 0; i < 32; i++)
			m_MouseButtonsJustPressed[i] = false;
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
		glfwSetCursorPosCallback(m_Window, cursor_position_callback);
		glfwSetMouseButtonCallback(m_Window, mouse_button_callback);

		if(glewInit() != GLEW_OK)
		{
			std::cout << "Error to initializate GLEW :c" << std::endl;
			return false;
		}

		glEnable(GL_DEPTH_TEST);

		return true;
	}

	void Window::Update() 
	{
		for(int i = 0; i < 32; i++)
			m_MouseButtonsJustPressed[i] = false;

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

	void Window::SetVSync(bool value) const
	{
		glfwSwapInterval(value);
	}

	bool Window::IsButtomPressed(uint32_t button) const
	{
		return m_MouseButtons[button];
	}

	bool Window::IsButtomJustPressed(uint32_t button) const
	{
		return m_MouseButtonsJustPressed[button];
	}

	bool Window::IsKeyPressed(uint32_t keyCode) const
	{
		return m_Keys[keyCode];
	}
	
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		glfwGetCursorPos(window, &win->m_XMousePos, &win->m_YMousePos);

		//if(win->m_XMousePos < win->m_Width / 2) //means that is in the left side
		//{
		//	win->m_XMousePos = (-((win->m_Width / 2 - win->m_XMousePos) / win->m_Width)) * 2;
		//}
		//else if(win->m_XMousePos > win->m_Width / 2)
		//{
		//	win->m_XMousePos = (-((win->m_Width / 2 - win->m_XMousePos) / win->m_Width)) * 2;
		//}
		//else
		//{
		//	win->m_XMousePos = 0;
		//}

		//if(win->m_YMousePos < win->m_Height / 2) //means that is in the top side
		//{
		//	win->m_YMousePos = ((win->m_Height / 2 - win->m_YMousePos) / win->m_Height) * 2;
		//}
		//else if(win->m_YMousePos > win->m_Height / 2)
		//{
		//	win->m_YMousePos = ((win->m_Height / 2 - win->m_YMousePos) / win->m_Height) * 2;
		//}
		//else
		//{
		//	win->m_YMousePos = 0;
		//}
	}

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);

		win->m_MouseButtons[button] = action == GLFW_PRESS;
		win->m_MouseButtonsJustPressed[button] = action == GLFW_PRESS;
	}

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		win->m_Keys[key] = action != GLFW_RELEASE;
	}

}