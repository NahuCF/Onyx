#include <iostream>
#include <cmath>
#include <SE.h>

#include "glm.hpp"

#include "Serializer/Serializer.h";

int main()
{
	se::Window window("test", 800, 800);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	const int TILE_WIDTH = 8;
	const int TILE_HEIGHT = 8;

	//se::Serializer tileMapSerializer;

	//int tileMap[10][10];

	while(!window.Closed())
	{
		window.Clear();

		if(window.IsButtomJustPressed(GLFW_MOUSE_BUTTON_LEFT))
		{
			std::cout << "YES" << std::endl;
		}
		//std::cout << "Tile: (" << (int)(window.GetMousePos().x / TILE_WIDTH) << ", " << (int)(window.GetMousePos().y / TILE_HEIGHT) << ")" << std::endl;

		window.Update();
	}
}

