#include <iostream>

#include "graphics/window.h"
#include "graphics/shader.h"

#include "GLFW/glfw3.h"

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include "stb_image/stb_image.h"

int main()
{
	using namespace se;
	using namespace graphics;

	Window window("SE", 1600, 900);
	glClearColor(0.3f, 0.3f, 1.0f, 1.0f);

	Shader PrimerShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	unsigned char* data = stbi_load("./assets/textures/Mario.png", &width, &height, &nrChannels, 0);

	unsigned int marioTexture;
	glGenTextures(1, &marioTexture);
	glBindTexture(GL_TEXTURE_2D, marioTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	if (!data)
		std::cout << "pene" << std::endl;
	stbi_image_free(data);

	float vertices[] = 
	{
		 0.1f,  0.17f, 0.0f,    1.0f, 1.0f,
		 0.1f, -0.17f, 0.0f,    1.0f, 0.0f,
		-0.1f, -0.17f, 0.0f,    0.0f, 0.0f,
		-0.1f,  0.17f, 0.0f,    0.0f, 1.0f
	};

	unsigned int indices[] = 
	{
		0, 1, 3, 
		1, 2, 3  
	};

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	PrimerShader.UseProgramShader();
	//glUniform1i(glGetUniformLocation(PrimerShader.m_ProgramID, "TextureData"), 0);

	while(!window.Closed())
	{
		window.Vsync("Disable");
		window.Clear();

		//CODE HERE

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, marioTexture);

		PrimerShader.UseProgramShader();
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		//

		window.Update();
	}
}