#pragma once

#include "Maths/Maths.h"
#include <memory>
#include <vector>

namespace Onyx {

	struct PreloadedImage
	{
		std::vector<unsigned char> pixels;
		int width = 0, height = 0, channels = 0;
		bool Valid() const { return !pixels.empty(); }
		void Free()
		{
			pixels.clear();
			pixels.shrink_to_fit();
		}
	};

	class Texture
	{
	public:
		Texture(const char* texturePath);
		Texture(const char* texturePath, uint8_t keyR, uint8_t keyG, uint8_t keyB);
		~Texture();

		static std::unique_ptr<Texture> CreateSolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
		static std::unique_ptr<Texture> CreateFromData(const void* data, int width, int height, int channels);
		static std::unique_ptr<Texture> CreateWithMipmaps(const void* data, int width, int height, int channels);
		static std::unique_ptr<Texture> CreateFloatTexture(const float* data, int width, int height);
		static std::unique_ptr<Texture> CreateNoiseTexture(const float* data, int width, int height);

		static PreloadedImage PreloadFromFile(const char* path);

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

		uint32_t m_TextureID = 0;
		unsigned char* m_TextureData = nullptr;
		int m_TextureWidth = 0, m_TextureHeight = 0, m_NChannels = 0;
	};

} // namespace Onyx
