#include <iostream>
#include <string>

#include "graphics/window.h"
#include "graphics/shader.h"

#include "GLFW/glfw3.h"

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

int main()
{
	using namespace se;
	using namespace graphics;

	Window window("SE", 800, 600);
	glClearColor(0.5f, 0.5f, 1.0f, 1.0f);

	while(!window.Closed())
	{
		window.Vsync("Disable");
		window.Clear();

		//CODE HERE


		
		//

		window.Update();
	}
}