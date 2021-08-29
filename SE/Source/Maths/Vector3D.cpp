#include "pch.h"
#include "Vector3D.h"

namespace lptm {

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

	Vector3D& Vector3D::Add(const Vector3D& other) 
	{  
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}

	Vector3D& Vector3D::Multiply(const Vector3D& other) 
	{
		return *this;
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

	Vector3D Vector3D::operator*(const Vector3D& other)
	{
		return Multiply(other);
	}

	Vector3D& Vector3D::operator*=(const Vector3D& other) 
	{ 
		return Multiply(other);
	}

	bool Vector3D::operator==(const Vector3D& other) const
	{
		return x == other.x && y == other.y;
	}

	std::ostream& operator<<(std::ostream& stream, lptm::Vector3D& vector)
	{
		stream << "(" << vector.x << ", " << vector.y << ")";
		return stream;
	}
}