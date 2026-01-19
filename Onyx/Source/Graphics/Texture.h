#pragma once

#include "Maths/Maths.h"
#include <memory>

namespace Onyx {

	class Texture
	{
	public:
		Texture(const char* texturePath);
		// Constructor with color key support (magenta transparency)
		Texture(const char* texturePath, uint8_t keyR, uint8_t keyG, uint8_t keyB);
		~Texture();

		// Factory method for creating a solid color texture (1x1 pixel)
		static std::unique_ptr<Texture> CreateSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

		void Bind() const;
		void Bind(uint32_t slot) const;
		void UnBind() const;

        void SetData(void* data);

		uint32_t GetTextureID() const { return m_TextureID; }
		Onyx::Vector2D GetTextureSize() const { return Onyx::Vector2D(m_TextureWidth, m_TextureHeight); }
	private:
		// Private constructor for factory methods
		Texture();
		void InitTexture(const char* texturePath, bool useColorKey, uint8_t keyR, uint8_t keyG, uint8_t keyB);
		void InitSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

		uint32_t m_TextureID;
		unsigned char* m_TextureData;
		int m_TextureWidth, m_TextureHeight, m_NChannels;
	};

}