#pragma once

#include "Maths/Maths.h"
#include "Graphics/Shader.h"

namespace Onyx {

    class Camera
    {
    public:
        Camera();

        void Matrix(float FOVdeg, float nearPlane, float farPlane, Shader& shader, const char* uniform);
    private:
        glm::vec3 m_Position;
        glm::vec3 m_Orientation = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 m_Up = glm::vec3(0.0f, 1.0f, 0.0f);

        int m_Width, m_Height;

        float m_Speed = 0.1f;
        float m_Sensitivity = 100.0f;
    };
    
}