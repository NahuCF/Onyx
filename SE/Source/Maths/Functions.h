#pragma once

#include "Maths/Maths.h"

namespace Onyx {

	float RadianToDegree(float radian);
	float DegreeToRadian(float degree);

	float VectorModule(Vector2D vector);
	float VectorAngle(Vector2D vector);

    Onyx::Vector2D Normalize(Onyx::Vector2D vector);

	Onyx::Vector2D RotateVector(Vector2D point, float degRotation);

	float lerp(float a, float b, float t);
	Onyx::Vector4D lerp4D(Onyx::Vector4D a, Onyx::Vector4D b, float t);
	Onyx::Vector2D lerp2D(Onyx::Vector2D a, Onyx::Vector2D b, float t);

	float ramdomInRange(float min, float max);

}