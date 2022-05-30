#include "Game.h"

int main()
{
	Game* game = new Game();
	game->Update();
	delete game;
}

