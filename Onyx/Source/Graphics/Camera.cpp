#include "pch.h"

#include "Camera.h"
#include "Graphics/Window.h"
namespace Onyx {

    Camera::Camera(int width, int height, glm::vec3 position)
        : m_Width(width)
        , m_Height(height)
        , m_Position(position)
    {
        m_LastX = width / 2;
        m_LastY = height / 2;
    }

    void Camera::Matrix(float FOVdeg, float nearPlane, float farPlane, Shader &shader, const char *uniform)
    {
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        view = glm::lookAt(m_Position, m_Position + m_Orientation, m_Up);
        projection = glm::perspective(glm::radians(FOVdeg),(float)(m_Width / m_Height), nearPlane, farPlane); 

        glUniformMatrix4fv(glGetUniformLocation(shader.GetProgramID(), uniform), 1, GL_FALSE, glm::value_ptr(projection * view));
    }

    void Camera::Inputs(Onyx::Window& window)
    {
        if(window.IsKeyPressed(GLFW_KEY_W)) 
        {
            m_Position += m_Speed * m_Orientation;
        }
        if(window.IsKeyPressed(GLFW_KEY_S)) 
        {
            m_Position += m_Speed * -m_Orientation;
        }
        if(window.IsKeyPressed(GLFW_KEY_A)) 
        {
            m_Position -= m_Speed * glm::normalize(glm::cross(m_Orientation, m_Up));
        }
        if(window.IsKeyPressed(GLFW_KEY_D)) 
        {
            m_Position += m_Speed * glm::normalize(glm::cross(m_Orientation, m_Up));
        }
        if(window.IsKeyPressed(GLFW_KEY_Q)) 
        {
            m_Position += m_Speed * m_Up;
        }
        if(window.IsKeyPressed(GLFW_KEY_E)) 
        {
            m_Position -= m_Speed * m_Up;
        }

        if(window.IsButtomPressed(GLFW_MOUSE_BUTTON_LEFT))
        {
            glfwSetInputMode(window.WindowGUI(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            double mouseX;
            double mouseY;
            glfwGetCursorPos(window.WindowGUI(), &mouseX, &mouseY);
            
            float xPos = static_cast<float>(mouseX);
            float yPos = static_cast<float>(mouseY);

            if (m_FirstClick)
            {
                m_LastX = xPos;
                m_LastY = yPos;
                m_FirstClick = false;
            }

            float xOffset = xPos - m_LastX;
            float yOffset = m_LastY - yPos;
            m_LastX = xPos;
            m_LastY = yPos;

            xOffset *= m_Sensitivity;
            yOffset *= m_Sensitivity;

            m_Yaw += xOffset;
            m_Pitch += yOffset;

            if (m_Pitch > 89.0f)
                m_Pitch = 89.0f;
            if (m_Pitch < -89.0f)
                m_Pitch = -89.0f;

            glm::vec3 front;
            front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            front.y = sin(glm::radians(m_Pitch));
            front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            m_Orientation = glm::normalize(front);
        }
        if(window.IsButtomReleased(GLFW_MOUSE_BUTTON_LEFT))
        {
            m_FirstClick = true;
            glfwSetInputMode(window.WindowGUI(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

}