#include <iostream>

#include "window.h"

int main()
{
	using namespace se;
	using namespace graphics;

	Window window("Pija", 800, 600);
	glClearColor(0.2f, 0.3f, 0.8f, 1.0f);

	while(!window.Closed())
	{
		window.Clear();
		std::cout << window.Width() << ", " << window.Height() << std::endl;
		glBegin(GL_TRIANGLES);
		glVertex2f(-0.5f, -0.5f);
		glVertex2f( 0.0f,  0.5f);
		glVertex2f( 0.5f, -0.5f);
		glEnd();
		window.Update();
	}
}