#include <iostream>
#include <cmath>
#include <array>

#include <SE.h>

#include "glm.hpp"

lptm::Vector2D PixelToNDC(lptm::Vector2D mousePos, lptm::Vector2D windowSize)
{
	return lptm::Vector2D(2 / (windowSize.x / mousePos.x), 2 / (windowSize.y / mousePos.y));
}

int main()
{
	se::Window window("Game", 1600, 900);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	window.SetVSync(0);

	se::Shader baseShader("Assets/Shaders/Shader.vs", "Assets/Shaders/Shader.fs");
	se::Shader textureShader("Assets/Shaders/TextureBatch.vs", "Assets/Shaders/TextureBatch.fs");

	se::Texture ball("Assets/Textures/nezuko-ball.png");
	se::Texture grass("Assets/Textures/grass.png");
	
	se::Renderer2D renderer;
	//se::ParticleSystem particle(20);

	while(!window.Closed())
	{
		window.Clear();

		lptm::Vector2D pos = PixelToNDC(window.GetMousePos(), { 1000, 1000 });

		textureShader.Bind();
		renderer.RenderQuad({ 0.1f, 0.1f }, { 1.0f, 1.0f, 0.0f }, grass, &textureShader);
				//else
					//renderer.RenderQuad({ 0.2f, 0.2f }, { 1.0f - (1.0f - ((float)x * 0.2f)), 1.0f - (1.0f - ((float)y * 0.2f)), 0.1f }, grass, &textureShader);
		//renderer.RenderQuad({ 0.25f, 0.25f }, { 0.25f, 1.0f, 0.0f }, mario, &textureShader);
		//renderer.RenderQuad({ 0.25f, 0.25f }, { 1.0f, 1.0f, 0.0f }, grass, &textureShader);
		
		//if(window.IsButtomPressed(GLFW_MOUSE_BUTTON_LEFT))
		/*particle.Emiter(
			{ pos.x, pos.y, 0.0f }, 
			{ 0.5f, 1.0f, 1.0f, 1.0f }, 
			{ 0.0f, -1.0f * window.GetSeconds() }, 
			{ 0.025f, 0.025f },
			{ 0.0125f, 0.0125f },
			1.0f, 0.4f, window.GetSeconds()
		);*/
		//else
			//particle.Update({ pos.x, pos.y, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -0.0f }, 1, 0.1f, window.GetSeconds());

		/*for(uint32_t y = 0; y < 400; y++)
			for (uint32_t x = 0; x < 400; x++)
				if((x + y) % 2 == 0)
					renderer.RenderQuad({ 0.005f,  0.005f }, { 1.0f - (1.0f - ((float)x * 0.005f)), 1.0f - (1.0f - ((float)y * 0.005f)), 0.1f }, { 1.0f, 0.0f, 0.0f, 1.0f });
				else
					renderer.RenderQuad({ 0.005f,  0.005f }, { 1.0f - (1.0f - ((float)x * 0.005f)), 1.0f - (1.0f - ((float)y * 0.005f)), 0.1f }, { 0.0f, 1.0f, 0.0f, 1.0f });*/

		renderer.Flush();

		window.Update();
	} 
}

