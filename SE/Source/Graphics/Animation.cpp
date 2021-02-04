#include <iostream>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#define STB_IMAGE_STATIC
#include "Source/Vendor/stb_image/stb_image.h"

#include "Animation.h"

namespace se {

	Animation::Animation(const char* filePath, uint32_t width, uint32_t height, uint32_t spriteWidth, uint32_t spriteHeight)
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

		m_TextureData = stbi_load(filePath, &m_TextureWidth, &m_TextureHeight, &m_NChannels, 0);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_TextureWidth, m_TextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_TextureData);

		if (!m_TextureData)
			std::cout << "FAILED TO LOAD TEXTURE" << std::endl;

		stbi_image_free(m_TextureData);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

}