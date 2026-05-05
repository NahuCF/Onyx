#include "pch.h"

#include <GL/glew.h>

#define STB_IMAGE_STATIC
#include <stb_image.h>

#include "TextureArray.h"

namespace Onyx {

	TextureArray::TextureArray()
		: m_TextureID(0), m_Width(0), m_Height(0), m_Layers(0), m_Channels(4)
	{
	}

	TextureArray::~TextureArray()
	{
		if (m_TextureID)
		{
			glDeleteTextures(1, &m_TextureID);
		}
	}

	void TextureArray::Create(int width, int height, int layers, int channels)
	{
		if (m_TextureID)
		{
			glDeleteTextures(1, &m_TextureID);
		}

		m_Width = width;
		m_Height = height;
		m_Layers = layers;
		m_Channels = channels;

		GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_TextureID);

		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat,
					 width, height, layers, 0,
					 (channels == 4) ? GL_RGBA : GL_RGB,
					 GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	void TextureArray::SetLayerData(int layer, const void* data)
	{
		if (!m_TextureID || layer < 0 || layer >= m_Layers)
			return;

		GLenum format = (m_Channels == 4) ? GL_RGBA : GL_RGB;

		glBindTexture(GL_TEXTURE_2D_ARRAY, m_TextureID);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
						0, 0, layer,
						m_Width, m_Height, 1,
						format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	bool TextureArray::LoadLayerFromFile(const std::string& path, int layer)
	{
		if (!m_TextureID || layer < 0 || layer >= m_Layers)
			return false;

		stbi_set_flip_vertically_on_load(true);

		int w, h, ch;
		unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, m_Channels);
		if (!data)
		{
			std::cerr << "[TextureArray] Failed to load: " << path << '\n';
			return false;
		}

		if (w == m_Width && h == m_Height)
		{
			SetLayerData(layer, data);
			stbi_image_free(data);
			return true;
		}

		std::vector<uint8_t> resized(m_Width * m_Height * m_Channels);
		for (int y = 0; y < m_Height; y++)
		{
			for (int x = 0; x < m_Width; x++)
			{
				int srcX = x * w / m_Width;
				int srcY = y * h / m_Height;
				srcX = (std::min)(srcX, w - 1);
				srcY = (std::min)(srcY, h - 1);
				for (int c = 0; c < m_Channels; c++)
				{
					resized[(y * m_Width + x) * m_Channels + c] =
						data[(srcY * w + srcX) * m_Channels + c];
				}
			}
		}

		SetLayerData(layer, resized.data());
		stbi_image_free(data);
		return true;
	}

	void TextureArray::SetLayerSolidColor(int layer, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		if (!m_TextureID || layer < 0 || layer >= m_Layers)
			return;

		std::vector<uint8_t> pixels(m_Width * m_Height * m_Channels);
		for (int i = 0; i < m_Width * m_Height; i++)
		{
			pixels[i * m_Channels + 0] = r;
			pixels[i * m_Channels + 1] = g;
			pixels[i * m_Channels + 2] = b;
			if (m_Channels == 4)
			{
				pixels[i * m_Channels + 3] = a;
			}
		}

		SetLayerData(layer, pixels.data());
	}

	void TextureArray::Bind(uint32_t slot) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_TextureID);
	}

	void TextureArray::UnBind() const
	{
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

} // namespace Onyx
