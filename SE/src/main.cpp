#include <iostream>

#include "graphics/window.h"
#include "maths/vec2.h"

int main()
{
	int a;

	using namespace se;
	using namespace graphics;
	using namespace maths;

	Window window("SE", 800, 600);
	glClearColor(0.2f, 0.3f, 0.8f, 1.0f);

	vec2 vector(1.0f, 2.0f);
	vector.add(vec2(2.0f, 1.0f));
	vector.add(vec2(2.0f, 2.0f)).multiply(vec2(2.0f, 2.0f));


	std::cout << vector.x << ", " << vector.y << std::endl;

	while(!window.Closed())
	{
		window.Clear();
		//std::cout << window.Width() << ", " << window.Height() << std::endl;
		window.Update();
	}
}