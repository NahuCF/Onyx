#include <iostream>

#include "window.h"

int main()
{
	using namespace se;
	using namespace graphics;

	Window window("Pija", 800, 600);
	while(!window.Closed())
	{
		window.Update();
	}
}