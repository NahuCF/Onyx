#include <iostream>
#include <cmath>
#include <SE.h>

#include "glm.hpp"

int main()
{
	se::Window window("test", 1000, 1000);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	window.SetVSync(0);

	float textureWidth = 0.2f;
	const int TILE_WIDTH = 100, TILE_HEIGHT = 100;
	int tileX, tileY;

	se::Texture whiteTile("Assets/Textures/grass.png", textureWidth, textureWidth);


	auto tiles = new se::Shader*[10][10];

	for(int y = 0; y < 10; y++)
	{
		for(int x = 0; x < 10; x++)
		{
			tiles[y][x] = nullptr;
		}
	}

	while(!window.Closed())
	{
		window.Clear();

		tileX = int(window.GetMousePos().x / TILE_WIDTH);
		tileY = int(window.GetMousePos().y / TILE_HEIGHT);

		// Create tile
		if(window.IsButtomPressed(GLFW_MOUSE_BUTTON_LEFT))
		{
			if(tiles[tileX][tileY] == nullptr)
			{
				se::Shader* nuevo = new se::Shader("Assets/Shaders/TextureShader.vs", "Assets/Shaders/TextureShader.fs");
				nuevo->SetPos(glm::vec3(-1.0f + textureWidth / 2 + tileX * textureWidth, 1.0f - textureWidth / 2 - tileY * textureWidth, 0.2f));

				tiles[tileX][tileY] = nuevo;
			}
		}

		// Destroy tile
		if(window.IsButtomPressed(GLFW_MOUSE_BUTTON_RIGHT))
		{
			if(tiles[tileX][tileY] != nullptr)
			{
				delete tiles[tileX][tileY];
				tiles[tileX][tileY] = nullptr;
			}
		}

		for (int y = 0; y < 10; y++)
		{
			for (int x = 0; x < 10; x++)
			{
				if(tiles[y][x] != nullptr)
				{
					tiles[y][x]->UseProgramShader();
					whiteTile.UseTexture();
				}
			}
		}

		window.Update();
	} 
}

