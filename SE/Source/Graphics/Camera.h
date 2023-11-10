#pragma once

#include "Maths/Maths.h"

namespace Onyx {
    class Camera
    {
    public:
        Camera();

        void SetPosition(Onyx::Vector3D vector) { m_Position = { -vector.x, -vector.y, -vector.z }; }
        void MoveCamera(Onyx::Vector3D vector) { m_Position += { vector.x, vector.y, vector.z }; }

        Onyx::Vector3D GetPosition() const { return m_Position; }
    private:
        Onyx::Vector3D m_Position;
    };
}