#include "pch.h"
#include "Vector3D.h"

namespace Onyx {

	Vector3D::Vector3D()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	};

	Vector3D::Vector3D(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

    real Vector3D::Magnitude() const
    {
        return sqrt(x * x + y * y + z * z);
    }

    void Vector3D::Normalize()
    {
        real l = Magnitude();
        if(l > 0)
        {
            (*this) /= l;
        }
    }

    Vector3D Vector3D::operator+(const Vector3D& other)
	{
		return Vector3D(x + other.x, y + other.y, z + other.z);
	}

	Vector3D& Vector3D::operator+=(const Vector3D& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}

	Vector3D Vector3D::operator*(const real other)
	{
		return {
            (*this).x * other,
            (*this).y * other,
            (*this).z * other,
        };
	}

	void Vector3D::operator*=(real other) 
	{ 
		x *= other;
		y *= other;
		z *= other;
	}

    void Vector3D::operator/=(real other)
    {
        x = x / other;
        y = y / other;
        z = z / other;
    }

    bool Vector3D::operator==(const Vector3D& other) const
	{
		return x == other.x && y == other.y;
	}
    
}