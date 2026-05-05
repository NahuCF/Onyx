#include <fstream>
#include <iostream>
#include <string>

#include "Viewport.h"
#include <SE.h>

int main()
{

	se::Window window("Tile Editor", 1000, 1000);
	glClearColor(1.0f, 0.3f, 0.4f, 1.0f);
	window.SetVSync(0);

	Viewport viewport;
	ImGuiIO& io = viewport.Init(&window);

	while (!window.Closed())
	{
		window.Clear();

		viewport.Loop(io);

		window.Update();
	}
}