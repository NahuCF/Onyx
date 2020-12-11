#include <iostream>

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

	Shader PrimerShader("./shaders/shader.vs", "./shaders/shader.fs");
	Shader SecondShader("./shaders/shader.vs", "./shaders/shader.fs");

	while(!window.Closed())
	{
		window.Vsync("Disable");
		window.Clear();

		//CODE HERE

		PrimerShader.MoveTo(glm::vec3(0.5f, -0.5, 0.0f));
		PrimerShader.UseProgramShader();

		glBegin(GL_TRIANGLES);
		glVertex3f(-1.0f, -1.0f, 0.0f);
		glVertex3f( 1.0f, -1.0f, 0.0f);
		glVertex3f( 0.0f,  1.0f, 0.0f);
		glEnd();

		SecondShader.MoveTo(glm::vec3(-0.5f, -0.5, 0.0f));
		SecondShader.UseProgramShader();

		glBegin(GL_TRIANGLES);
		glVertex3f(-1.0f, -1.0f, 0.0f);
		glVertex3f(1.0f, -1.0f, 0.0f);
		glVertex3f(0.0f, 1.0f, 0.0f);
		glEnd();
		
		//

		window.Update();
	}
}