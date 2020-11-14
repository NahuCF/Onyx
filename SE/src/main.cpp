#include <iostream>

#include "graphics/window.h"
#include "maths/vec2.h"

int main()
{
	using namespace se;
	using namespace graphics;
	using namespace maths;

	Window window("SE", 800, 600);
	glClearColor(0.2f, 0.3f, 0.8f, 1.0f);

	while(!window.Closed())
	{
		window.Clear();
		window.FPS();
		window.Vsync("Enable");
		window.Update();
	}
}