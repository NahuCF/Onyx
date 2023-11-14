#pragma once

#include "Physics/Precision.h"

namespace Onyx {

	struct Vector3D
	{
		real x, y, z;

		Vector3D();
		Vector3D(float x, float y, float z);

        real Magnitude() const;
        real SquareMagnitude() const;
        void Normalize();

		Vector3D operator-(const Vector3D& other);
		void operator-=(const Vector3D& other);

		Vector3D operator+(const Vector3D& other);
		void operator+=(const Vector3D& other);

        Vector3D operator*(const real other);
		void operator*=(real other);

		bool operator==(const Vector3D& other) const;
		void operator/=(real other);
	};

}