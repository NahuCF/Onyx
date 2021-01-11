#include <iostream>

#include "graphics/window.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "src/collision/ColliderBox.h"

#include "GLFW/glfw3.h"

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include "../maths.h"
int main()
{
	using namespace se;
	using namespace graphics;
	using namespace collision;

	Window window("SE", 1600, 900);
	glClearColor(0.3f, 0.3f, 1.0f, 1.0f);

	Shader MarioShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Shader PandaShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	ColliderBox PandaCollider(0.2f, 0.35f, 0.0f, 0.0f);
	Shader PandaShader2("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Shader PandaShader3("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	PandaShader2.SetPos(glm::vec3(-0.5f, -0.5f, 0.0f));
	PandaShader3.SetPos(glm::vec3(0.5f, -0.5f, 0.0f));
	Texture Mario("./assets/textures/Mario.png", 0.2f, 0.35f);
	Texture Panda("./assets/textures/Panda.png", 0.2f, 0.35f);
	Texture Panda2("./assets/textures/Panda.png", 0.2f, 0.35f);
	Texture Panda3("./assets/textures/Panda.png", 0.2f, 0.35f);

	Shader* contenedor[3];
	ColliderBox* boxContenedor[1];

	while (!window.Closed())
	{
		window.Vsync("Enable");
		window.Clear();

		//CODE HERE
		PandaShader.UseProgramShader();
		PandaShader.A�adir(contenedor, 0);
		PandaCollider.AddCollider(boxContenedor, 0);
		Panda.UseTexture();

		PandaShader2.UseProgramShader();
		PandaShader2.A�adir(contenedor, 1);
		Panda2.UseTexture();
	
		PandaShader3.UseProgramShader();
		PandaShader3.A�adir(contenedor, 2);
		Panda3.UseTexture();

		MarioShader.UseProgramShader();
		Mario.UseTexture();
		if(window.IsKeyPressed(GLFW_KEY_W))
		{
			MarioShader.MoveUp(contenedor, 0.01f, sizeof(contenedor));
			MoveBoxsColliderUp(boxContenedor, 0.01f, 1);
		}
		if(window.IsKeyPressed(GLFW_KEY_S))
		{
			MarioShader.MoveDown(contenedor, 0.01f, sizeof(contenedor));
			MoveBoxsColliderDown(boxContenedor, 0.01f, 1);
		}
		if(window.IsKeyPressed(GLFW_KEY_D))
		{
			MarioShader.MoveRight(contenedor, 0.01f, sizeof(contenedor));
			MoveBoxsColliderRight(boxContenedor, 0.01f, 1);
		}
		if(window.IsKeyPressed(GLFW_KEY_A))
		{
			MarioShader.MoveLeft(contenedor, 0.01f, sizeof(contenedor));
			MoveBoxsColliderLeft(boxContenedor, 0.01f, 1);
		}
		//

		std::cout << boxContenedor[0]->m_Max.x << "  " << boxContenedor[0]->m_Max.y << std::endl;

		window.Update();
	}
}