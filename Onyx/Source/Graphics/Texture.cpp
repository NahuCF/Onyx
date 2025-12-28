#include "pch.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_STATIC
#include <stb_image.h>

#include "Texture.h"

namespace Onyx {

	Texture::Texture(const char* texturePath)
	{
		stbi_set_flip_vertically_on_load(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glGenTextures(1, &m_TextureID);
		Bind();
		 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		m_TextureData = stbi_load(texturePath, &m_TextureWidth, &m_TextureHeight, &m_NChannels, 0);

		if (!m_TextureData) {
			std::cout << "ERROR::TEXTURE::FILE_NOT_FOUND: " << texturePath << std::endl;
		}

		GLenum internalFormat, dataFormat;
		if (m_NChannels == 4) {
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		} else if (m_NChannels == 3) {
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		} else if (m_NChannels == 1) {
			internalFormat = GL_R8;
			dataFormat = GL_RED;
		} else {
			// Default to RGBA
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_TextureWidth, m_TextureHeight, 0, dataFormat, GL_UNSIGNED_BYTE, m_TextureData);

		if(!m_TextureData)	
			std::cout << "FAILED TO LOAD TEXTURE" << std::endl;

		stbi_image_free(m_TextureData);
		UnBind();
	}

	Texture::~Texture()
	{
		glDeleteTextures(1, &m_TextureID);
	}

	void Texture::Bind() const
	{
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
	}

	void Texture::UnBind() const
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

    void Texture::SetData(void* data)
    {
        glTextureSubImage2D(this->m_TextureID, 0, 0, 0, m_TextureWidth, m_TextureHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
}