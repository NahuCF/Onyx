#include <iostream>

#include "graphics/window.h"
#include "graphics/shader.h"
#include "src/graphics/texture.h"

#include "GLFW/glfw3.h"

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

int main()
{
	using namespace se;
	using namespace graphics;

	Window window("SE", 1600, 900);
	glClearColor(0.3f, 0.3f, 1.0f, 1.0f);

	Shader MarioShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Shader PandaShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Shader PandaShader2("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	PandaShader2.SetPos(glm::vec3(-0.5f, -0.5f, 0.0f));
	Texture Mario("./assets/textures/Mario.png", 0.2f, 0.35f);
	Texture Panda("./assets/textures/Panda.png", 0.2f, 0.35f);
	Texture Panda2("./assets/textures/Panda.png", 0.2f, 0.35f);

	Shader* contenedor[2];

	while (!window.Closed())
	{
		window.Vsync("Enable");
		window.Clear();

		//CODE HERE
		PandaShader.UseProgramShader();
		PandaShader.Añadir(contenedor, 0);
		Panda.UseTexture();

		PandaShader2.UseProgramShader();
		PandaShader2.Añadir(contenedor, 1);
		Panda2.UseTexture();

		MarioShader.UseProgramShader();
		Mario.UseTexture();
		if (window.IsKeyPressed(GLFW_KEY_W))
		{
			MarioShader.MoveUp(contenedor, 0.01f);
		}
		if (window.IsKeyPressed(GLFW_KEY_S))
		{
			MarioShader.MoveDown(contenedor, 0.01f);
		}
		if (window.IsKeyPressed(GLFW_KEY_D))
		{
			MarioShader.MoveRight(contenedor, 0.01f);
		}
		if (window.IsKeyPressed(GLFW_KEY_A))
		{
			MarioShader.MoveLeft(contenedor, 0.01f);
		}
		//

		window.Update();
	}
}