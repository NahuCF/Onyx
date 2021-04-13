#include <iostream>
#include <cmath>
#include <SE.h>

#include "glm.hpp"

int main()
{
	se::Window window("Test", 1600, 800);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	std::vector<se::Shader*> contenedor;

	se::Shader ea("Assets/Shaders/TextureShader.vs", "Assets/Shaders/TextureShader.fs");
	ea.SetPos(glm::vec3(0.5f, 0.5f, -0.4f));
	ea.AddShader(contenedor);
	se::Texture textura("Assets/Textures/Mario.png", 0.5f, 0.5f);

	se::Shader eafondo("Assets/Shaders/TextureShader.vs", "Assets/Shaders/TextureShader.fs");
	eafondo.SetPos(glm::vec3(0.5f, 0.5f, -0.5f));
	se::Texture fondo("Assets/Textures/1.png", 0.5f, 0.5f);

	window.SetVSync(0);

	while(!window.Closed())
	{
		window.Clear();
	
		ea.UseProgramShader();
		textura.UseTexture();

		if(window.IsKeyPressed(GLFW_KEY_A))
		{
			eafondo.MoveLeft(contenedor, 0.1f);
		}
		if(window.IsKeyPressed(GLFW_KEY_D))
		{
			eafondo.MoveRight(contenedor, 0.1f);
		}

		window.Update();
	}
}

