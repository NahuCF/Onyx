#include "pch.h"

#include "Camera.h"

namespace Onyx {

    Camera::Camera(int width, int height, glm::vec3 position)
        : m_Width(width)
        , m_Height(height)
        , m_Position(position)
    {
    }

    void Camera::Matrix(float FOVdeg, float nearPlane, float farPlane, Shader &shader, const char *uniform)
    {
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        view = glm::lookAt(m_Position, m_Position + m_Orientation, m_Up);
        projection = glm::perspective(glm::radians(FOVdeg),(float)(m_Width / m_Height), nearPlane, farPlane); 

        glUniformMatrix4fv(glGetUniformLocation(shader.GetProgramID(), uniform), 1, GL_FALSE, glm::value_ptr(projection));
    }

}