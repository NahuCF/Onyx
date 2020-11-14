#pragma once

#define Vec4Log(vector) std::cout << "<" << vector.x << ", " << vector.y << ", " << vector.z << ", " << vector.w << ">" << std::endl;

namespace se { namespace maths {

	struct vec4
	{
		float x, y, z, w;

		vec4();
		vec4(float x, float y, float z, float w);

		vec4& add(vec4 other);
		vec4& substract(vec4 other);
		vec4& multiply(vec4 other);
		vec4& divide(vec4 other);
	};
} }