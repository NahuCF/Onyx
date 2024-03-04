#pragma once

#include "Maths/Maths.h"
#include "Graphics/Shader.h"
#include "Graphics/Window.h"

namespace Onyx {

    class Camera
    {
    public:
        Camera(int width, int height, glm::vec3 position);

        void Matrix(float FOVdeg, float nearPlane, float farPlane, Shader& shader, const char* uniform);

        void Inputs(Onyx::Window& window);

        glm::vec3 const GetPosition() const { return m_Position; }
    private:
        glm::vec3 m_Position;
        glm::vec3 m_Orientation = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 m_Up = glm::vec3(0.0f, 1.0f, 0.0f);

        int m_Width, m_Height;

        bool m_FirstClick = true;

        float m_Speed = 0.1f;
        float m_Sensitivity = 0.1f;

        float m_LastY, m_LastX;

        float m_Yaw = -90.0f;
        float m_Pitch = 0.0f;
    };
    
}