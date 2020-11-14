#pragma once

#define Vec3Log(vector) std::cout << "<" << vector.x << ", " << vector.y << ", " << vector.z << ">" << std::endl;

namespace se { namespace maths {

	struct vec3
	{
		float x, y, z;

		vec3();
		vec3(float x, float y, float z);

		vec3& add(vec3 other);
		vec3& substract(vec3 other);
		vec3& multiply(vec3 other);
		vec3& divide(vec3 other);
	};
} }