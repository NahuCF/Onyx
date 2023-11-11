#pragma once

#include "Physics/Precision.h"

namespace Onyx {

	struct Vector3D
	{
		real x, y, z;

		Vector3D();
		Vector3D(float x, float y, float z);
		Vector3D& Add(const Vector3D& other);
		Vector3D& Multiply(const Vector3D& other);

		Vector3D operator+(const Vector3D& other);
		Vector3D operator*(const Vector3D& other);
		Vector3D operator*(const float other);

		Vector3D& operator+=(const Vector3D& other);
		Vector3D& operator*=(const Vector3D& other);

		bool operator==(const Vector3D& other) const;
	};

}