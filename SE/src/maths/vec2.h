#pragma once

#define Vec2Log(vector) std::cout << "<" << vector.x << ", " << vector.y << ">" << std::endl;

namespace se { namespace maths {

	struct vec2
	{
		float x, y;

		vec2();
		vec2(float x, float y);
		vec2& add(vec2 other);
		vec2& substract(vec2 other);
		vec2& multiply(vec2 other);
		vec2& divide(vec2 other);
	};
} }