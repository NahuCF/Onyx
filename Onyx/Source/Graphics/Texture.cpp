#include "pch.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_STATIC
#include <stb_image.h>

#include "Texture.h"

namespace Onyx {

	Texture::Texture()
		: m_TextureID(0), m_TextureData(nullptr), m_TextureWidth(0), m_TextureHeight(0), m_NChannels(0)
	{
	}

	Texture::Texture(const char* texturePath)
	{
		InitTexture(texturePath, false, 0, 0, 0);
	}

	Texture::Texture(const char* texturePath, uint8_t keyR, uint8_t keyG, uint8_t keyB)
	{
		InitTexture(texturePath, true, keyR, keyG, keyB);
	}

	std::unique_ptr<Texture> Texture::CreateSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		auto texture = std::unique_ptr<Texture>(new Texture());
		texture->InitSolidColor(r, g, b, a);
		return texture;
	}

	void Texture::InitSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		m_TextureWidth = 1;
		m_TextureHeight = 1;
		m_NChannels = 4;

		glGenTextures(1, &m_TextureID);
		Bind();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		uint8_t data[4] = { r, g, b, a };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		UnBind();
	}

	void Texture::InitTexture(const char* texturePath, bool useColorKey, uint8_t keyR, uint8_t keyG, uint8_t keyB)
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
			UnBind();
			return;
		}

		// Apply color key (convert specified color to transparent)
		unsigned char* finalData = m_TextureData;
		unsigned char* rgbaData = nullptr;

		if (useColorKey) {
			// Always convert to RGBA for color keying
			int pixelCount = m_TextureWidth * m_TextureHeight;
			rgbaData = new unsigned char[pixelCount * 4];

			for (int i = 0; i < pixelCount; ++i) {
				uint8_t r, g, b, a;

				if (m_NChannels >= 3) {
					r = m_TextureData[i * m_NChannels + 0];
					g = m_TextureData[i * m_NChannels + 1];
					b = m_TextureData[i * m_NChannels + 2];
					a = (m_NChannels == 4) ? m_TextureData[i * m_NChannels + 3] : 255;
				} else {
					r = g = b = m_TextureData[i * m_NChannels];
					a = 255;
				}

				// Check if pixel matches color key
				if (r == keyR && g == keyG && b == keyB) {
					a = 0; // Make transparent
				}

				rgbaData[i * 4 + 0] = r;
				rgbaData[i * 4 + 1] = g;
				rgbaData[i * 4 + 2] = b;
				rgbaData[i * 4 + 3] = a;
			}

			finalData = rgbaData;
			m_NChannels = 4;
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
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_TextureWidth, m_TextureHeight, 0, dataFormat, GL_UNSIGNED_BYTE, finalData);

		stbi_image_free(m_TextureData);
		if (rgbaData) {
			delete[] rgbaData;
		}
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

	void Texture::Bind(uint32_t slot) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
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