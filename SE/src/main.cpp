#include <iostream>
#include <vector>

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

	Window window("SE", 1280, 720);
	glClearColor(0.3f, 0.3f, 1.0f, 1.0f);

	std::vector<Shader*> contenedorr;

	std::vector<Shader> tuputamadre;

	Shader MarioShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Texture Mario("./assets/textures/Mario.png", 0.2f, 0.35f);
	ColliderBox MarioCollider(0.2f, 0.35f, 0.0f, 0.0f);

	Shader PandaShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	PandaShader.Añadir(contenedorr);
	Texture Panda("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider(0.2f, 0.35f, 0.0f, 0.0f);

	Shader PandaShader2("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	PandaShader2.Añadir(contenedorr);
	Texture Panda2("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider2(0.2f, 0.35f, -0.5f, -0.5f);

	Shader PandaShader3("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	PandaShader3.Añadir(contenedorr);
	Texture Panda3("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider3(0.2f, 0.35f, 0.5f, -0.5f);

	Shader ShaderPandaTemplate("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Texture TexturePandaTemplate("./assets/textures/Panda.png", 0.2f, 0.35f);

	PandaShader2.SetPos(glm::vec3(-0.5f, -0.5f, 0.0f));
	PandaShader3.SetPos(glm::vec3(0.5f, -0.5f, 0.0f));

	const int boxColliderContenedorLength = 3;

	ColliderBox* boxContenedor[boxColliderContenedorLength];

	while (!window.Closed())
	{
		window.Vsync("Disable");
		window.Clear();
		std::cout << sizeof(Shader) << std::endl;
		for(int i = 0; i < contenedorr.size(); i++)
		{
			if(MarioShader.GetPosX() - contenedorr[i]->GetPosX() < 1)
			{
				contenedorr[i]->UseProgramShader();
				TexturePandaTemplate.UseTexture();
			}
		}
		if(window.IsButtomJustPressed(GLFW_MOUSE_BUTTON_RIGHT))
		{
			Shader* ShaderPandaTemplatee = new Shader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
			ShaderPandaTemplatee->SetPos(glm::vec3((float)window.GetMousePosX(), (float)window.GetMousePosY(), 0.0f));
			contenedorr.push_back(ShaderPandaTemplatee);
		}

		//CODE HERE
		PandaShader.UseProgramShader();
		PandaCollider.AddCollider(boxContenedor, 0);
		Panda.UseTexture();

		PandaShader2.UseProgramShader();
		PandaCollider2.AddCollider(boxContenedor, 1);
		Panda2.UseTexture();

		PandaShader3.UseProgramShader();
		PandaCollider3.AddCollider(boxContenedor, 2);
		Panda3.UseTexture();


		MarioShader.UseProgramShader();
		Mario.UseTexture();
		if(window.IsKeyPressed(GLFW_KEY_W))
		{
			std::cout << contenedorr.size() << std::endl;
			MarioShader.MoveUp(contenedorr, 0.017f);
			//MoveBoxsColliderUp(boxContenedor, 0.017f, boxColliderContenedorLength);
		}
		if(window.IsKeyPressed(GLFW_KEY_S))
		{
			MarioShader.MoveDown(contenedorr, 0.017f);
			//MoveBoxsColliderDown(boxContenedor, 0.017f, boxColliderContenedorLength);
		}
		if(window.IsKeyPressed(GLFW_KEY_D))
		{
			MarioShader.MoveRight(contenedorr, 0.01f);
			//MoveBoxsColliderRight(boxContenedor, 0.01f, boxColliderContenedorLength);
		}
		if(window.IsKeyPressed(GLFW_KEY_A))
		{
			MarioShader.MoveLeft(contenedorr, 0.01f);
			//MoveBoxsColliderLeft(boxContenedor, 0.01f, boxColliderContenedorLength);
		}

		if(IsColliding(boxContenedor, MarioCollider, boxColliderContenedorLength))
		{
			//std::cout << "colliding..." << std::endl;
			//ActivateCollition(boxContenedor, MarioCollider, contenedor, 3, 4);	
		}
		//
		//std::cout << boxContenedor[0]->m_Min.y << " " << MarioCollider.m_Min.y << std::endl;

		window.Update();
	}
}