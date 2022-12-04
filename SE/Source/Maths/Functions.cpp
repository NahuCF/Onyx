#include "pch.h"

#include <cmath>
#include <iostream>

#include "Functions.h"
#include "glm.hpp"

#define PI 3.14159265358979323846

namespace lptm {

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

	float VectorAngle(Vector2D vector)
	{
		return std::atan(vector.y / vector.x);
	}

	lptm::Vector2D RotateVector(Vector2D vector, float degRotation)
	{
		float module = VectorModule(vector);
		return lptm::Vector2D(std::cos(DegreeToRadian(degRotation)) * module, std::sin(DegreeToRadian(degRotation)) * module);
	}

	float lerp(float a, float b, float t)
	{
		return a + t * (b - a);
	}

	lptm::Vector4D lerp4D(lptm::Vector4D a, lptm::Vector4D b, float t)
	{
		return lptm::Vector4D(
			lptm::lerp(a.x, b.x, t),
			lptm::lerp(a.y, b.y, t),
			lptm::lerp(a.z, b.z, t),
			lptm::lerp(a.w, b.w, t)
		);
	}

	lptm::Vector2D lerp2D(lptm::Vector2D a, lptm::Vector2D b, float t)
	{
		return lptm::Vector2D(
			lptm::lerp(a.x, b.x, t),
			lptm::lerp(a.y, b.y, t)
		);
	}

	float ramdomInRange(float min, float max)
	{
		return ((float(rand()) / float(RAND_MAX)) * (max - min)) + min;;
	}

}