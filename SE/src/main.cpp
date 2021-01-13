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
	MarioShader.SetPos(glm::vec3(-0.5f, 0.5f, 0.0f));
	Texture Mario("./assets/textures/Mario.png", 0.2f, 0.35f);
	ColliderBox MarioCollider(0.2f, 0.35f, -0.5f, 0.5f);

	Shader PandaShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Texture Panda("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider(0.2f, 0.35f, 0.0f, 0.0f);

	Shader PandaShader2("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Texture Panda2("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider2(0.2f, 0.35f, -0.5f, -0.5f);

	Shader PandaShader3("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Texture Panda3("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider3(0.2f, 0.35f, 0.5f, -0.5f);

	Shader PandaShader4("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Texture Panda4("./assets/textures/Panda.png", 0.2f, 0.35f);

	PandaShader2.SetPos(glm::vec3(-0.5f, -0.5f, 0.0f));
	PandaShader3.SetPos(glm::vec3(0.5f, -0.5f, 0.0f));
	PandaShader4.SetPos(glm::vec3(0.5f, 0.5f, 0.0f));

	Shader* contenedor[4];
	ColliderBox* boxContenedor[3];

	while (!window.Closed())
	{
		window.Vsync("Enable");
		window.Clear();

		//CODE HERE
		PandaShader.UseProgramShader();
		PandaShader.Añadir(contenedor, 0);
		PandaCollider.AddCollider(boxContenedor, 0);
		Panda.UseTexture();

		PandaShader2.UseProgramShader();
		PandaShader2.Añadir(contenedor, 1);
		PandaCollider2.AddCollider(boxContenedor, 1);
		Panda2.UseTexture();
	
		PandaShader3.UseProgramShader();
		PandaShader3.Añadir(contenedor, 2);
		PandaCollider3.AddCollider(boxContenedor, 2);
		Panda3.UseTexture();

		PandaShader4.UseProgramShader();
		PandaShader4.Añadir(contenedor, 3);
		Panda4.UseTexture();

		MarioShader.UseProgramShader();
		Mario.UseTexture();
		if(window.IsKeyPressed(GLFW_KEY_W) )
		{
			MarioShader.MoveUp(contenedor, 0.017f, sizeof(contenedor));
			MoveBoxsColliderUp(boxContenedor, 0.017f, 3);
		}
		if(window.IsKeyPressed(GLFW_KEY_S) )
		{
			MarioShader.MoveDown(contenedor, 0.017f, sizeof(contenedor));
			MoveBoxsColliderDown(boxContenedor, 0.017f, 3);
		}
		if(window.IsKeyPressed(GLFW_KEY_D) && !IsGointToCollide(boxContenedor, MarioCollider, 3, 0.01f))
		{
			MarioShader.MoveRight(contenedor, 0.01f, sizeof(contenedor));
			MoveBoxsColliderRight(boxContenedor, 0.01f, 3);
		}
		if(window.IsKeyPressed(GLFW_KEY_A) && !IsGointToCollide(boxContenedor, MarioCollider, 3, 0.01f))
		{
			MarioShader.MoveLeft(contenedor, 0.01f, sizeof(contenedor));
			MoveBoxsColliderLeft(boxContenedor, 0.01f, 3);
		}

		if(IsColliding(boxContenedor, MarioCollider, 3))
		{
			ActivateCollition(boxContenedor, MarioCollider, contenedor, 3, 4);	
		}
		//
		//std::cout << boxContenedor[0]->m_Min.y << " " << MarioCollider.m_Min.y << std::endl;
		
		window.Update();
	}
}