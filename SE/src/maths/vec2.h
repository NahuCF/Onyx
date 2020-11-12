#pragma once

namespace se {	namespace maths {

	struct vec2
	{
		float x, y;

		vec2();
		vec2(const float& x, const float& y);

		vec2& add(vec2 other);
		vec2& substract(vec2 other);
		vec2& multiply(vec2 other);
		vec2& divide(vec2 other);
	};
} }