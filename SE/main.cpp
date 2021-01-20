#include <iostream>
#include <cmath>

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

	std::vector<Shader*> shaderContenedor;
	std::vector<ColliderBox*> colliderContenedor;

	const char* vShaderPath = "./shaders/textureShader.vs";
	const char* fShaderPath = "./shaders/textureShader.fs";

	Shader MarioShader(vShaderPath, fShaderPath);
	Texture Mario("./assets/textures/Mario.png", 0.2f, 0.35f);
	ColliderBox MarioCollider(0.2f, 0.35f, 0.0f, 0.0f);

	Shader PandaShader(vShaderPath, fShaderPath);
	PandaShader.SetPos(glm::vec3(-0.5f, 0.5f, 0.0f));
	PandaShader.AddShader(shaderContenedor);
	Texture Panda("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider(0.2f, 0.35f, -0.5f, 0.5f);
	PandaCollider.AddCollider(colliderContenedor);

	Shader PandaShader2(vShaderPath, fShaderPath);
	PandaShader2.SetPos(glm::vec3(-0.5f, -0.5f, 0.0f));
	PandaShader2.AddShader(shaderContenedor);
	Texture Panda2("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider2(0.2f, 0.35f, -0.5f, -0.5f);
	PandaCollider2.AddCollider(colliderContenedor);

	Shader PandaShader3(vShaderPath, fShaderPath);
	PandaShader3.AddShader(shaderContenedor);
	PandaShader3.SetPos(glm::vec3(0.5f, -0.5f, 0.0f));
	Texture Panda3("./assets/textures/Panda.png", 0.2f, 0.35f);
	ColliderBox PandaCollider3(0.2f, 0.35f, 0.5f, -0.5f);
	PandaCollider3.AddCollider(colliderContenedor);

	Shader ShaderPandaTemplate(vShaderPath, fShaderPath);
	Texture TexturePandaTemplate("./assets/textures/Panda.png", 0.2f, 0.35f);

	window.Vsync("Disable");
	
	while(!window.Closed())
	{
		window.Clear();

		for(int i = 0; i < shaderContenedor.size(); i++)
		{
			shaderContenedor[i]->UseProgramShader();
			TexturePandaTemplate.UseTexture();
		}

		MarioShader.UseProgramShader();
		Mario.UseTexture();
		if(window.IsKeyPressed(GLFW_KEY_W) && !IsGointToCollide(colliderContenedor, MarioCollider, 0, 0.017f))
		{
			MarioShader.MoveUp(shaderContenedor, 0.017f);
			MoveBoxsColliderUp(colliderContenedor, 0.017f);
		}
		if(window.IsKeyPressed(GLFW_KEY_S) && !IsGointToCollide(colliderContenedor, MarioCollider, 0, -0.017f))
		{
			MarioShader.MoveDown(shaderContenedor, 0.017f);
			MoveBoxsColliderDown(colliderContenedor, 0.017f);
		}
		if(window.IsKeyPressed(GLFW_KEY_D) && !IsGointToCollide(colliderContenedor, MarioCollider, 0.01f, 0))
		{
			MarioShader.MoveRight(shaderContenedor, 0.01f);
			MoveBoxsColliderRight(colliderContenedor, 0.01f);
		}
		if(window.IsKeyPressed(GLFW_KEY_A) && !IsGointToCollide(colliderContenedor, MarioCollider, -0.01f, 0))
		{
			MarioShader.MoveLeft(shaderContenedor, 0.01f);
			MoveBoxsColliderLeft(colliderContenedor, 0.01f);
		}

		window.Update();
	}
}