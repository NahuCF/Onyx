#include <iostream>

#include "graphics/window.h"

int main()
{
	using namespace se;
	using namespace graphics;

	Window window("SE", 800, 600);
	glClearColor(0.2f, 0.3f, 0.8f, 1.0f);

	while(!window.Closed())
	{
		window.Clear();
		window.Vsync("Enable");
		window.Update();
	}
}