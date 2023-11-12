#pragma once

namespace Onyx {

	struct Vector4D
	{
		float x, y, z, w;

		Vector4D();
		Vector4D(float x, float y, float z, float w);
		Vector4D& Add(const Vector4D& other);
		Vector4D& Multiply(const Vector4D& other);

		Vector4D operator+(const Vector4D& other);
		Vector4D operator*(const Vector4D& other);

		Vector4D& operator+=(const Vector4D& other);
		Vector4D& operator*=(const Vector4D& other);

		bool operator==(const Vector4D& other) const;
	};

	//std::ostream& operator<<(std::ostream& stream, Onyx::Vector4D& vector);
}