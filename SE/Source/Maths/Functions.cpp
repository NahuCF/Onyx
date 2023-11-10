#include "pch.h"

#include <cmath>
#include <iostream>

#include "Functions.h"
#include "glm.hpp"

#define PI 3.14159265358979323846

namespace Onyx {

	float RadianToDegree(float radian)
	{
		return (radian * 180.0f) / PI;
	}

	float DegreeToRadian(float degree)
	{
		return (degree * PI) / 180.0f;
	}

	float VectorModule(Vector2D vector)
	{
		return sqrt(vector.x * vector.x + vector.y * vector.y);
	}

    Onyx::Vector2D Normalize(Onyx::Vector2D vector)
    {
        float module = VectorModule(vector);

        return {
            vector.x / module,
            vector.y / module
        };
    }

	float VectorAngle(Vector2D vector)
	{
		return std::atan(vector.y / vector.x);
	}

	Onyx::Vector2D RotateVector(Vector2D vector, float degRotation)
	{
		float module = VectorModule(vector);
		return Onyx::Vector2D(std::cos(DegreeToRadian(degRotation)) * module, std::sin(DegreeToRadian(degRotation)) * module);
	}

	float lerp(float a, float b, float t)
	{
		return a + t * (b - a);
	}

	Onyx::Vector4D lerp4D(Onyx::Vector4D a, Onyx::Vector4D b, float t)
	{
		return Onyx::Vector4D(
			Onyx::lerp(a.x, b.x, t),
			Onyx::lerp(a.y, b.y, t),
			Onyx::lerp(a.z, b.z, t),
			Onyx::lerp(a.w, b.w, t)
		);
	}

	Onyx::Vector2D lerp2D(Onyx::Vector2D a, Onyx::Vector2D b, float t)
	{
		return Onyx::Vector2D(
			Onyx::lerp(a.x, b.x, t),
			Onyx::lerp(a.y, b.y, t)
		);
	}

	float ramdomInRange(float min, float max)
	{
		return ((float(rand()) / float(RAND_MAX)) * (max - min)) + min;;
	}

}