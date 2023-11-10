#pragma once

#include "Maths/Maths.h"

namespace Velvet {
    class Camera
    {
    public:
        Camera();

        void SetPosition(lptm::Vector3D vector) { m_Position = { -vector.x, -vector.y, -vector.z }; }
        void MoveCamera(lptm::Vector3D vector) { m_Position += { vector.x, vector.y, vector.z }; }

        lptm::Vector3D GetPosition() const { return m_Position; }
    private:
        lptm::Vector3D m_Position;
    };
}