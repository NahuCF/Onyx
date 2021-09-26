#pragma once

#include "Maths/Maths.h"

namespace lptm {

	float RadianToDegree(float radian);
	float DegreeToRadian(float degree);

	float VectorModule(Vector2D vector);
	float VectorAngle(Vector2D vector);

	lptm::Vector2D RotateVector(Vector2D point, float degRotation);

}