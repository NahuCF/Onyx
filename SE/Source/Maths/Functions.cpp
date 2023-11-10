#include "pch.h"

#include <cmath>
#include <iostream>

#include "Functions.h"
#include "glm.hpp"

#define PI 3.14159265358979323846

namespace Velvet {

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

    Velvet::Vector2D Normalize(Velvet::Vector2D vector)
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

	Velvet::Vector2D RotateVector(Vector2D vector, float degRotation)
	{
		float module = VectorModule(vector);
		return Velvet::Vector2D(std::cos(DegreeToRadian(degRotation)) * module, std::sin(DegreeToRadian(degRotation)) * module);
	}

	float lerp(float a, float b, float t)
	{
		return a + t * (b - a);
	}

	Velvet::Vector4D lerp4D(Velvet::Vector4D a, Velvet::Vector4D b, float t)
	{
		return Velvet::Vector4D(
			Velvet::lerp(a.x, b.x, t),
			Velvet::lerp(a.y, b.y, t),
			Velvet::lerp(a.z, b.z, t),
			Velvet::lerp(a.w, b.w, t)
		);
	}

	Velvet::Vector2D lerp2D(Velvet::Vector2D a, Velvet::Vector2D b, float t)
	{
		return Velvet::Vector2D(
			Velvet::lerp(a.x, b.x, t),
			Velvet::lerp(a.y, b.y, t)
		);
	}

	float ramdomInRange(float min, float max)
	{
		return ((float(rand()) / float(RAND_MAX)) * (max - min)) + min;;
	}

}