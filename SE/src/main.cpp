#include <iostream>
#include <string>

#include "graphics/window.h"
#include "maths/maths.h"
#include "graphics/shader.h"

int main()
{
	using namespace se;
	using namespace graphics;
	using namespace maths;

	Window window("SE", 800, 600);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	while(!window.Closed())
	{
		window.Clear();
		window.Vsync("Enable");
		window.Update();	
	}
}