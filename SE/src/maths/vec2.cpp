#include "vec2.h"

namespace se { namespace maths {

	vec2::vec2()
	{
		x = 0.0f;
		y = 0.0f;
	}

	vec2::vec2(const float& x, const float& y)
	{
		this->x = x;
		this->y = y;
	}

	vec2 &vec2::add(vec2 other)
	{
		x += other.x;
		y += other.y;

		return *this;
	}

	vec2& vec2::substract(vec2 other)
	{
		x -= other.x;
		y -= other.y;

		return *this;
	}

	vec2& vec2::multiply(vec2 other)
	{
		x *= other.x;
		y *= other.y;

		return *this;
	}

	vec2& vec2::divide(vec2 other)
	{
		x /= other.x;
		y /= other.y;

		return *this;
	}
}}