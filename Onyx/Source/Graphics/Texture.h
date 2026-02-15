#pragma once

#include "Maths/Maths.h"
#include <memory>

namespace Onyx {

	class Texture
	{
	public:
		Texture(const char* texturePath);
		Texture(const char* texturePath, uint8_t keyR, uint8_t keyG, uint8_t keyB);
		~Texture();

		static std::unique_ptr<Texture> CreateSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
		static std::unique_ptr<Texture> CreateFromData(const void* data, int width, int height, int channels);
		static std::unique_ptr<Texture> CreateFloatTexture(const float* data, int width, int height);

		void Bind() const;
		void Bind(uint32_t slot) const;
		void UnBind() const;

        void SetData(void* data);
		void SetFloatData(const float* data);



		uint32_t GetTextureID() const { return m_TextureID; }
		Onyx::Vector2D GetTextureSize() const { return Onyx::Vector2D(m_TextureWidth, m_TextureHeight); }

	private:
		Texture();
		void InitTexture(const char* texturePath, bool useColorKey, uint8_t keyR, uint8_t keyG, uint8_t keyB);
		void InitSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
		void InitFromData(const void* data, int width, int height, int channels);

		uint32_t m_TextureID;
		unsigned char* m_TextureData;
		int m_TextureWidth, m_TextureHeight, m_NChannels;
	};

}
