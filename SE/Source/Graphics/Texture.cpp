#include <iostream>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#define STB_IMAGE_STATIC
#include "Source/Vendor/stb_image/stb_image.h"

#include "Texture.h"

namespace se {

	Texture::Texture(const char* texturePath, float Width, float Height)
	{
		stbi_set_flip_vertically_on_load(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID); 

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		m_TextureData = stbi_load(texturePath, &m_TextureWidth, &m_TextureHeight, &m_NChannels, 0);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_TextureWidth, m_TextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_TextureData);

		if(!m_TextureData)	
			std::cout << "FAILED TO LOAD TEXTURE" << std::endl;

		stbi_image_free(m_TextureData);
		glBindTexture(GL_TEXTURE_2D, 0);

		//Create VAO

		float vertices[20] =
		{
			Width / 2,  Height / 2, 0.0f,  1.0f, 1.0f,  //Top Right
			Width / 2, -Height / 2, 0.0f,  1.0f, 0.0f,  //Bottom Right
		   -Width / 2, -Height / 2, 0.0f,  0.0f, 0.0f,  //Bottom Left
		   -Width / 2,  Height / 2, 0.0f,  0.0f, 1.0f	//Top Left
		};

		uint32_t indices[6] =
		{
			0, 1, 3,
			1, 2, 3
		};

		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);
		glGenBuffers(1, &m_EBO);

		glBindVertexArray(m_VAO);

		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_VERTEX_ARRAY, 0);
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	Texture::~Texture()
	{
		glDeleteVertexArrays(1, &m_VAO);
		glDeleteBuffers(1, &m_VBO);
		glDeleteBuffers(1, &m_EBO);
	}

	void Texture::UseTexture() const 
	{
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glBindVertexArray(m_VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

}