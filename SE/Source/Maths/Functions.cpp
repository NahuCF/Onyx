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

}