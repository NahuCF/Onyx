#include <iostream>
#include <cmath>
#include <SE.h>

#include "glm.hpp"

int main()
{
	se::Window window("test", 800, 800);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	while(!window.Closed())
	{
		window.Clear();

		window.Update();
	} 
}

