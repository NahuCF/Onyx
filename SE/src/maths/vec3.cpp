#include "vec3.h"

namespace se { namespace maths {

	vec3::vec3()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	vec3::vec3(float x,  float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	vec3& vec3::add(vec3 other)
	{
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}

	vec3& vec3::substract(vec3 other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;

		return *this;
	}

	vec3& vec3::multiply(vec3 other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;

		return *this;
	}

	vec3& vec3::divide(vec3 other)
	{
		x /= other.x;
		y /= other.y;
		z /= other.z;

		return *this;
	}
}}