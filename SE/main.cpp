#include <iostream>

#include "graphics/Window.h"
#include "graphics/Shader.h"
#include "graphics/Texture.h"
#include "src/collision/ColliderBox.h"

#include "GLFW/glfw3.h"

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include "src/Vendor/ImGui/imgui.h"
#include "src/Vendor/ImGui/imgui_impl_opengl3.h"
#include "src/Vendor/ImGui/imgui_impl_glfw.h"

int main()
{
	using namespace se;
	using namespace graphics;
	using namespace collision;

	Window Window("SE", 1024, 672);
	glClearColor(0.364f, 0.580f, 0.984f, 1.0f);

	std::vector<Shader*> shaderContenedor;
	std::vector<ColliderBox*> colliderContenedor;

	const char* vShaderPath = "./shaders/textureShader.vs";
	const char* fShaderPath = "./shaders/textureShader.fs";

	Shader MarioShader(vShaderPath, fShaderPath);
	Texture Mario("./assets/textures/Mario.png", 0.12f, 0.13f);
	ColliderBox MarioCollider(0.12f, 0.15f, 0.0f, 0.0f);

	Shader oneWord(vShaderPath, fShaderPath);
	oneWord.SetPos(glm::vec3(0.0f, 0.0f, 0.0f));
	oneWord.AddShader(shaderContenedor);
	Texture oneWordTexture("./assets/textures/1.png", 2.0f, 2.0f);
	ColliderBox oneWordCollider(2.0f, 0.4f, 0.0f, -1.0f);
	oneWordCollider.AddCollider(colliderContenedor);

	Shader twoWord(vShaderPath, fShaderPath);
	twoWord.SetPos(glm::vec3(2.0f, 0.0f, 0.0f));
	twoWord.AddShader(shaderContenedor);
	Texture twoWordTexture("./assets/textures/2.png", 2.0f, 2.0f);
	ColliderBox twoWordCollider(2.0f, 0.4f, 2.0f, -1.0f);
	twoWordCollider.AddCollider(colliderContenedor);


	ColliderBox FirstTube(0.22f, 0.24f, 2.62f, -0.64f);
	FirstTube.AddCollider(colliderContenedor);

	Window.SetVSync(0);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(Window.WindowGUI(), true);
	ImGui_ImplOpenGL3_Init((char*)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));

	char palabra[20] = {0};
	float f = 0.0f;
	bool my_tool_active = true;
	while(!Window.Closed())
	{
		Window.Clear();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("My First Tool", &my_tool_active, ImGuiWindowFlags_MenuBar);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
				if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
				if (ImGui::MenuItem("Close", "Ctrl+W")) { my_tool_active = false; }
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Text("Que onda wacho");
		ImGui::InputText("string", palabra, IM_ARRAYSIZE(palabra));
		if (ImGui::Button("Print"))
			std::cout << f << std::endl;
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
		ImGui::End();


		oneWord.UseProgramShader();
		oneWordTexture.UseTexture();

		twoWord.UseProgramShader();
		twoWordTexture.UseTexture();

		MarioShader.UseProgramShader();
		Mario.UseTexture();
		if(Window.IsKeyPressed(GLFW_KEY_W) && !IsGointToCollide(colliderContenedor, MarioCollider, 0.0f, 0.085f))
		{
			MarioShader.MoveShaderAxisY(0.085f);
			MarioCollider.MoveCollider(-0.0f, 0.085f);
		}
		if(Window.IsKeyPressed(GLFW_KEY_D) && !IsGointToCollide(colliderContenedor, MarioCollider, 0.01f, 0.0f))
		{
			if(MarioShader.GetRealPosX() > 0.2f)
			{
				MarioShader.MoveRight(shaderContenedor, 0.01f);
				MarioCollider.MoveCollider(0.01f, 0.0f);
			}
			else
			{
				MarioShader.MoveShaderRight(0.01f);
				MarioCollider.MoveCollider(0.01f, 0.0f);
			}
		}
		if(Window.IsKeyPressed(GLFW_KEY_A) && !IsGointToCollide(colliderContenedor, MarioCollider, -0.01f, 0.0f))
		{
			MarioShader.MoveShaderLeft(0.01f);
			MarioCollider.MoveCollider(-0.01f, 0.0f);
		}

		//Gravity
		if(!IsGointToCollide(colliderContenedor, MarioCollider, 0, -0.01f))
		{
			MarioShader.MoveShaderAxisY(-0.01f);
			MarioCollider.MoveCollider(0.0f, -0.01f);;
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		Window.Update();
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}