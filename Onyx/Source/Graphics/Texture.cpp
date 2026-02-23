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

	std::unique_ptr<Texture> Texture::CreateFromData(const void* data, int width, int height, int channels)
	{
		auto texture = std::unique_ptr<Texture>(new Texture());
		texture->InitFromData(data, width, height, channels);
		return texture;
	}

	void Texture::InitFromData(const void* data, int width, int height, int channels)
	{
		m_TextureWidth = width;
		m_TextureHeight = height;
		m_NChannels = channels;
		m_TextureData = nullptr;

		glGenTextures(1, &m_TextureID);
		Bind();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLenum internalFormat, dataFormat;
		if (channels == 4) {
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		} else if (channels == 3) {
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		} else if (channels == 1) {
			internalFormat = GL_R8;
			dataFormat = GL_RED;
		} else {
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

		UnBind();
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		m_TextureData = stbi_load(texturePath, &m_TextureWidth, &m_TextureHeight, &m_NChannels, 0);

		if (!m_TextureData) {
			std::cout << "ERROR::TEXTURE::FILE_NOT_FOUND: " << texturePath << std::endl;
			UnBind();
			return;
		}

		unsigned char* finalData = m_TextureData;
		unsigned char* rgbaData = nullptr;

		if (useColorKey) {
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

				if (r == keyR && g == keyG && b == keyB) {
					a = 0;
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
		glGenerateMipmap(GL_TEXTURE_2D);

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

std::unique_ptr<Texture> Texture::CreateFloatTexture(const float* data, int width, int height)
	{
		auto texture = std::unique_ptr<Texture>(new Texture());
		texture->m_TextureWidth = width;
		texture->m_TextureHeight = height;
		texture->m_NChannels = 1;
		texture->m_TextureData = nullptr;

		glGenTextures(1, &texture->m_TextureID);
		glBindTexture(GL_TEXTURE_2D, texture->m_TextureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data);
		glBindTexture(GL_TEXTURE_2D, 0);

		return texture;
	}

std::unique_ptr<Texture> Texture::CreateNoiseTexture(const float* data, int width, int height)
	{
		auto texture = std::unique_ptr<Texture>(new Texture());
		texture->m_TextureWidth = width;
		texture->m_TextureHeight = height;
		texture->m_NChannels = 3;
		texture->m_TextureData = nullptr;

		glGenTextures(1, &texture->m_TextureID);
		glBindTexture(GL_TEXTURE_2D, texture->m_TextureID);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);

		return texture;
	}

	void Texture::SetFloatData(const float* data)
	{
		glTextureSubImage2D(m_TextureID, 0, 0, 0, m_TextureWidth, m_TextureHeight, GL_RED, GL_FLOAT, data);
	}
}