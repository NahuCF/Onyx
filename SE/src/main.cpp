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

	std::vector<Shader*> shaderCotenedor;
	std::vector<ColliderBox*> colliderContenedor;

	const char* vShaderPath = "./shaders/textureShader.vs";
	const char* fShaderPath = "./shaders/textureShader.fs";

	Shader MarioShader(vShaderPath, fShaderPath);
	Texture Mario("./assets/textures/Mario.png", 0.2f, 0.35f);
	ColliderBox MarioCollider(0.2f, 0.35f, 0.0f, 0.0f);

	Shader PandaShader(vShaderPath, fShaderPath);
	PandaShader.Añadir(shaderCotenedor);
	Texture Panda("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider(0.2f, 0.35f, 0.0f, 0.0f);

	Shader PandaShader2(vShaderPath, fShaderPath);
	PandaShader2.Añadir(shaderCotenedor);
	Texture Panda2("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider2(0.2f, 0.35f, -0.5f, -0.5f);

	Shader PandaShader3(vShaderPath, fShaderPath);
	PandaShader3.Añadir(shaderCotenedor);
	Texture Panda3("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider3(0.2f, 0.35f, 0.5f, -0.5f);

	Shader ShaderPandaTemplate(vShaderPath, fShaderPath);
	Texture TexturePandaTemplate("./assets/textures/Panda.png", 0.2f, 0.35f);

	PandaShader2.SetPos(glm::vec3(-0.5f, -0.5f, 0.0f));
	PandaShader3.SetPos(glm::vec3(0.5f, -0.5f, 0.0f));

	while(!window.Closed())
	{
		window.Vsync("Disable");
		window.Clear();

		for(int i = 0; i < shaderCotenedor.size(); i++)
		{
			if(MarioShader.GetPosX() - shaderCotenedor[i]->GetPosX() < 1)
			{
				shaderCotenedor[i]->UseProgramShader();
				TexturePandaTemplate.UseTexture();
			}
		}
		if(window.IsButtomJustPressed(GLFW_MOUSE_BUTTON_RIGHT))
		{
			Shader* ShaderPandaTemplatee = new Shader(vShaderPath, fShaderPath);
			ShaderPandaTemplatee->SetPos(glm::vec3((float)window.GetMousePosX(), (float)window.GetMousePosY(), 0.0f));
			shaderCotenedor.push_back(ShaderPandaTemplatee);
		}

		//CODE HERE
		PandaShader.UseProgramShader();
		PandaCollider.AddCollider(colliderContenedor);
		Panda.UseTexture();

		PandaShader2.UseProgramShader();
		PandaCollider2.AddCollider(colliderContenedor);
		Panda2.UseTexture();

		PandaShader3.UseProgramShader();
		PandaCollider3.AddCollider(colliderContenedor);
		Panda3.UseTexture();


		MarioShader.UseProgramShader();
		Mario.UseTexture();
		if(window.IsKeyPressed(GLFW_KEY_W))
		{
			MarioShader.MoveUp(shaderCotenedor, 0.017f);
			//MoveBoxsColliderUp(shaderCotenedor, 0.017f, boxColliderContenedorLength);
		}
		if(window.IsKeyPressed(GLFW_KEY_S))
		{
			MarioShader.MoveDown(shaderCotenedor, 0.017f);
			//MoveBoxsColliderDown(shaderCotenedor, 0.017f, boxColliderContenedorLength);
		}
		if(window.IsKeyPressed(GLFW_KEY_D))
		{
			MarioShader.MoveRight(shaderCotenedor, 0.01f);
			//MoveBoxsColliderRight(shaderCotenedor, 0.01f, boxColliderContenedorLength);
		}
		if(window.IsKeyPressed(GLFW_KEY_A))
		{
			MarioShader.MoveLeft(shaderCotenedor, 0.01f);
			//MoveBoxsColliderLeft(shaderCotenedor, 0.01f, boxColliderContenedorLength);
		}

		window.Update();
	}
}