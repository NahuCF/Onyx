#pragma once

namespace lptm {

	struct Vector2D
	{
		float x, y;

		Vector2D();
		Vector2D(float x, float y);
		Vector2D Add(const Vector2D& other) const;
		Vector2D Multiply(const Vector2D& other) const;

		Vector2D operator+(const Vector2D& other) const;
		Vector2D operator*(const Vector2D& other) const;

		bool operator==(const Vector2D& other) const;
	};

}