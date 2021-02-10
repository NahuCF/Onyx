#include <iostream>

#include <SE.h>

int main()
{
	se::Window window("Test", 1600, 800);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	std::vector<se::Shader*> shadersitos;

	se::Shader marioShader("Assets/Shaders/TextureShader.vs", "Assets/Shaders/TextureShader.fs");
	marioShader.AddShader(shadersitos);

	se::Texture mario("Assets/Textures/Hola.png", 1.0f, 1.0f);

	while(!window.Closed())
	{
		window.Clear();

		if(window.IsKeyPressed(GLFW_KEY_A))
		{
			marioShader.MoveLeft(shadersitos, 0.01f);
		}
		if(window.IsKeyPressed(GLFW_KEY_D))
		{
			marioShader.MoveRight(shadersitos, 0.01f);
		}

		mario.UseTexture();

		window.Update();
	}
}
