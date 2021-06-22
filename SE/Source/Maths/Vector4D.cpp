#include "pch.h"
#include "Vector4D.h"

namespace lptm {

	Vector4D::Vector4D()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 0.0f;
	};

	Vector4D::Vector4D(float x, float y, float z, float w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	Vector4D& Vector4D::Add(const Vector4D& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;

		return *this;
	}

	Vector4D& Vector4D::Multiply(const Vector4D& other)
	{
		return *this;
	}

	Vector4D Vector4D::operator+(const Vector4D& other)
	{
		return Vector4D(x + other.x, y + other.y, z + other.z, w + other.w);
	}

	Vector4D& Vector4D::operator+=(const Vector4D& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;

		return *this;
	}

	Vector4D Vector4D::operator*(const Vector4D& other)
	{
		return Multiply(other);
	}

	Vector4D& Vector4D::operator*=(const Vector4D& other)
	{
		return Multiply(other);
	}

}