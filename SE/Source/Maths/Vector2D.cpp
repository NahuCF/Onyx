#include "pch.h"
#include "Vector2D.h"

namespace lptm {

	Vector2D::Vector2D()
	{
		x = 0.0f;
		y = 0.0f;
	};

	Vector2D::Vector2D(float x, float y)
	{
		this->x = x;
		this->y = y;
	}

	Vector2D& Vector2D::Add(const Vector2D& other) 
	{  
		x += other.x;
		y += other.y;

		return *this;
	}

	Vector2D& Vector2D::Multiply(const Vector2D& other) 
	{
		return *this;
	}	 

	Vector2D Vector2D::operator+(const Vector2D& other)
	{
		return Vector2D(x + other.x, y + other.y);
	}

	Vector2D& Vector2D::operator+=(const Vector2D& other)
	{
		x += other.x;
		y += other.y;

		return *this;
	}

	Vector2D Vector2D::operator*(const Vector2D& other)
	{
		return Multiply(other);
	}

	Vector2D& Vector2D::operator*=(const Vector2D& other) 
	{ 
		return Multiply(other);
	}

	bool Vector2D::operator==(const Vector2D& other) const
	{
		return x == other.x && y == other.y;
	}

	std::ostream& operator<<(std::ostream& stream, lptm::Vector2D& vector)
	{
		stream << "(" << vector.x << ", " << vector.y << ")";
		return stream;
	}
}