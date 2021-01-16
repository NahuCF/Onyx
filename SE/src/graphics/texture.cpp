#include <iostream>

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "src/stb_image/stb_image.h"

#include "texture.h"


namespace se { namespace graphics {

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

		for(int i = 0; i < 20; i++)
		{
			if( i == 3 || i == 4 || i == 8 || i == 19)
			{
				vertices[i] = 1.0f;
			}
			else if(i == 2 || i == 7 || i == 9 || i == 13 || i == 18 || i == 13 || i == 14 || i == 18)
			{
				vertices[i] = 0.0f;
			}
			else if(i == 0 || i == 5)
			{
				vertices[i] = Width / 2.0f;
			}
			else if(i == 10 || i == 15)
			{
				vertices[i] = -(Width / 2.0f);
			}
			else if(i == 1 || i == 16)
			{
				vertices[i] = (Height / 2.0f);
			}
			else if (i == 6 || i == 11)
			{
				vertices[i] = -(Height / 2.0f);
			}
		}

		indices[0] = { 0 };
		indices[1] = { 1 };
		indices[2] = { 3 };
		indices[3] = { 1 };
		indices[4] = { 2 };
		indices[5] = { 3 };

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

}}