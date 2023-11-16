#pragma once

#include "Physics/Precision.h"

namespace Onyx {

	struct Vector3
	{
		real x, y, z;

		Vector3();
		Vector3(real x, real y, real z);

        real Magnitude() const;
        real SquareMagnitude() const;
        void Normalize();

        void AddScaledVector(const Vector3& other, real scale);
        
        real ScalarProduct(const Vector3& vector) const;
        real operator*(const Vector3& vector) const;

        Vector3 ComponentProduct(const Vector3& vector) const;
        void ComponentProductUpdate(const Vector3& vector);

		Vector3 operator-(const Vector3& other);
		void operator-=(const Vector3& other);

		Vector3 operator+(const Vector3& other);
		void operator+=(const Vector3& other);

        Vector3 operator*(const real other);
		void operator*=(real other);

		bool operator==(const Vector3& other) const;
		void operator/=(real other);
	};

}