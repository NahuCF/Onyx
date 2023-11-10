#pragma once

namespace Velvet {

	struct Vector2D
	{
		float x, y;

		Vector2D();
		Vector2D(float x, float y);

		Vector2D& Add(const Vector2D& other);
		Vector2D& Multiply(const Vector2D& other);
        Vector2D& NormalizeValues();

		Vector2D operator+(const Vector2D& other);
		Vector2D operator-(const Vector2D& other);
		Vector2D operator*(const Vector2D& other);
		Vector2D operator*(const float other);
		Vector2D operator/(const float other);

		Vector2D& operator+=(const Vector2D& other);
		Vector2D& operator*=(const Vector2D& other);

		Vector2D Rotate(float degRotation);
		float GetVectorAngle() const ;

		bool operator==(const Vector2D& other) const;
	};

	//std::ostream& operator<<(std::ostream& stream, Velvet::Vector2D& vector);
}