#pragma once

#include "Maths/Maths.h"

namespace Velvet {

	float RadianToDegree(float radian);
	float DegreeToRadian(float degree);

	float VectorModule(Vector2D vector);
	float VectorAngle(Vector2D vector);

    Velvet::Vector2D Normalize(Velvet::Vector2D vector);

	Velvet::Vector2D RotateVector(Vector2D point, float degRotation);

	float lerp(float a, float b, float t);
	Velvet::Vector4D lerp4D(Velvet::Vector4D a, Velvet::Vector4D b, float t);
	Velvet::Vector2D lerp2D(Velvet::Vector2D a, Velvet::Vector2D b, float t);

	float ramdomInRange(float min, float max);

}