#include "pch.h"

#include "Window.h"

namespace Onyx {

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void window_pos_callback(GLFWwindow* window, int xpos, int ypos);
    void window_resize_callback(GLFWwindow* window, int cx, int cy);

	Window::Window(const char* title, uint32_t width, uint32_t height, float aspectRatio, bool fullscreen)
		: m_Title(title)
		, m_Height(height)
		, m_Width(width)
		, m_AspectRatio(aspectRatio)
        , m_FullScreen(fullscreen)
	{
		if(!Init())
			glfwTerminate();

		SetVSync(1);

		for(int i = 0; i < 1024; i++)
			m_Keys[i] = false;

		for (int i = 0; i < 1024; i++)
			m_KeysJustPressed[i] = false;

		for(int i = 0; i < 32; i++)
			m_MouseButtons[i] = false;

		for(int i = 0; i < 32; i++)
			m_MouseButtonsJustPressed[i] = false;

		for (int i = 0; i < 32; i++)
			m_MouseButtonsReleased[i] = false;
	};
	
	Window::~Window()
	{
		glfwTerminate();
	}

	bool Window::Init()
	{
		if(!glfwInit())
		{
			std::cout << "Failed to load GLFW :c" << std::endl;
			return false;
		}
		curtime = (float)glfwGetTime();
		lasttime = (float)glfwGetTime();

		m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, NULL, NULL);

        m_Monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);
        if (!mode) {
            glfwTerminate();
            return -1;
        }
        m_PrimaryMonitorWidth = mode->width;
        m_PrimaryMonitorHeight = mode->height;

        // Center window
        int posX = (m_PrimaryMonitorWidth - m_Width) / 2;
        int posY = (m_PrimaryMonitorHeight - m_Height) / 2;
        glfwSetWindowPos(m_Window, posX, posY);

        if(m_FullScreen)
            MakeFullScreen();
        
		if(!m_Window)
		{
			glfwTerminate();
			std::cout << "Fail to create GLFW window :c" << std::endl;
			return false;
		}

		glfwMakeContextCurrent(m_Window);
		glfwSetWindowUserPointer(m_Window, this);
		glfwSetKeyCallback(m_Window, key_callback);
		glfwSetCursorPosCallback(m_Window, cursor_position_callback);
		glfwSetMouseButtonCallback(m_Window, mouse_button_callback);
		glfwSetWindowPosCallback(m_Window, window_pos_callback);
        glfwSetWindowSizeCallback(m_Window, window_resize_callback);


		if(glewInit() != GLEW_OK)
		{
			std::cout << "Error to initializate GLEW :c" << std::endl;
			return false;
		}

		glEnable(GL_DEPTH_TEST);

		return true;
	}

	void Window::HandleResize()
	{	
		int aspectWidth = m_Width;
		int aspectHeight = (int)((float)aspectWidth / m_AspectRatio);
		if (aspectHeight > m_Height)
		{
			aspectHeight = m_Height;
			aspectWidth = (int)((float)aspectHeight * m_AspectRatio);
		}
	
		int vpX = (int)(((float)m_Width / 2.0f) - ((float)aspectWidth / 2.0f));
		int vpY = (int)(((float)m_Height / 2.0f) - ((float)aspectHeight / 2.0f));
		glViewport(vpX, vpY, aspectWidth, aspectHeight);
	}

	void Window::Update() 
	{
        //glfwSetWindowMonitor(m_Window, m_Monitor, m_WindowPosX, m_WindowPosY, m_Width, m_Height, GLFW_DONT_CARE);
		m_LastTime = (float)glfwGetTime();
		m_IsMouseMoving = false;
	
		m_FPS++;
		
		for (int i = 0; i < 1024; i++)
			m_KeysJustPressed[i] = false;

		for(int i = 0; i < 32; i++)
			m_MouseButtonsJustPressed[i] = false;

		for (int i = 0; i < 32; i++)
			m_MouseButtonsReleased[i] = false;

		glfwGetFramebufferSize(m_Window, &m_Width, &m_Height);
		HandleResize();

		glfwSwapBuffers(m_Window);
		glfwPollEvents();
	}

	bool Window::ShouldClose() const
	{
		return glfwWindowShouldClose(m_Window);
	}

    void Window::MakeFullScreen()
    {
        glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0, m_Width, m_Height, 0 );
    }

	void Window::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_CurrentTime = (float)glfwGetTime();
		curtime = (float)glfwGetTime();
		if (curtime - lasttime > 1)
		{
            m_LastFPS = m_FPS;
			std::cout << "FPS: " << m_FPS << std::endl;
			m_FPS = 0;

			curtime = (float)glfwGetTime();
			lasttime = (float)glfwGetTime();
		}
	}

	Onyx::Vector2D Window::GetMousePos() 
	{
		Onyx::Vector2D posInPx = GetMousePosInPixels();
		Onyx::Vector2D windowSize = GetWindowSize();

		float x = posInPx.x / windowSize.x;
		float y = posInPx.y / windowSize.y;

		return Onyx::Vector2D(x, y);
	}

	void Window::ShowFPS(bool value)
	{
		m_ShowFPS = value;
	}

	void Window::SetVSync(bool value) const
	{
		glfwSwapInterval(value);
	}

	void Window::CloseWindow() const
	{
		glfwTerminate();
	}

	bool Window::IsKeyPressed(uint32_t keyCode) const
	{
		return m_Keys[keyCode];
	}

	bool Window::IsKeyJustPressed(uint32_t key) const
	{
		return m_KeysJustPressed[key];
	}

	bool Window::IsButtomPressed(uint32_t button) const
	{
		return m_MouseButtons[button];
	}

	bool Window::IsButtomJustPressed(uint32_t button) const
	{
		return m_MouseButtonsJustPressed[button];
	}

	bool Window::IsButtomReleased(uint32_t button) const
	{
		return m_MouseButtonsReleased[button];
	}

	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		glfwGetCursorPos(window, &win->m_XMousePos, &win->m_YMousePos);
		win->m_IsMouseMoving = true;
	}

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);

		win->m_MouseButtons[button] = action == GLFW_PRESS;
		win->m_MouseButtonsJustPressed[button] = action == GLFW_PRESS;
		win->m_MouseButtonsReleased[button] = action == GLFW_RELEASE;
	}

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);

		win->m_Keys[key] = action != GLFW_RELEASE;
		win->m_KeysJustPressed[key] = action == GLFW_PRESS;
	}

	void window_pos_callback(GLFWwindow* window, int xpos, int ypos)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);

		win->m_WindowPosX = xpos;
		win->m_WindowPosY = ypos;
	}

    void window_resize_callback(GLFWwindow* window, int cx, int cy)
    {

    }

}