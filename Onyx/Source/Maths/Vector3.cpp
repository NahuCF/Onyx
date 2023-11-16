#include "pch.h"
#include "Vector3.h"

namespace Onyx {

	Vector3::Vector3()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	};

    Vector3::Vector3(real x, real y, real z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

    real Vector3::Magnitude() const
    {
        return sqrt(SquareMagnitude());
    }

    real Vector3::SquareMagnitude() const
    {
        return x * x + y * y + z * z;
    }

    void Vector3::Normalize()
    {
        real l = Magnitude();
        if(l > 0)
        {
            (*this) /= l;
        }
    }

    void Vector3::AddScaledVector(const Vector3 &other, real scale)
    {
        x += other.x * scale;
        y += other.y * scale;
        z += other.z * scale;
    }

    Vector3 Vector3::ComponentProduct(const Vector3 &vector) const
    {
        return Vector3(x * vector.x, y * vector.y, z * vector.z);
    }

    void Vector3::ComponentProductUpdate(const Vector3 &vector)
    {
        x *= vector.x;
        y *= vector.y;
        z *= vector.z;
    }

    Vector3 Vector3::operator-(const Vector3 &other)
    {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    void Vector3::operator-=(const Vector3 &other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
    }

    Vector3 Vector3::operator+(const Vector3& other)
	{
		return Vector3(x + other.x, y + other.y, z + other.z);
	}

	void Vector3::operator+=(const Vector3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
	}

	Vector3 Vector3::operator*(const real other)
	{
        return Vector3(x * other, y * other, z * other);
	}

    real Vector3::ScalarProduct(const Vector3 &vector) const
    {
        return x * vector.x + y * vector.y + z * vector.z;
    }

    real Vector3::operator*(const Vector3 &vector) const
    {
        return ScalarProduct(vector);
    }

    void Vector3::operator*=(real other) 
	{ 
		x *= other;
		y *= other;
		z *= other;
	}

    void Vector3::operator/=(real other)
    {
        x = x / other;
        y = y / other;
        z = z / other;
    }

    bool Vector3::operator==(const Vector3& other) const
	{
		return x == other.x && y == other.y;
	}
    
}