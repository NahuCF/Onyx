#pragma once

#include "Maths/Maths.h"

namespace lptm {

	float RadianToDegree(float radian);
	float DegreeToRadian(float degree);

	float VectorModule(Vector2D vector);
	float VectorAngle(Vector2D vector);

	lptm::Vector2D RotateVector(Vector2D point, float degRotation);

	float lerp(float a, float b, float t);
	lptm::Vector4D lerp4D(lptm::Vector4D a, lptm::Vector4D b, float t);
	lptm::Vector2D lerp2D(lptm::Vector2D a, lptm::Vector2D b, float t);

	float ramdomInRange(float min, float max);

}