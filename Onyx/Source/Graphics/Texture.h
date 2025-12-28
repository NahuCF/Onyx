#pragma once

#include "Maths/Maths.h"

namespace Onyx {

	class Texture
	{
	public:
		Texture(const char* texturePath);
		~Texture();

		void Bind() const;
		void UnBind() const;
        
        void SetData(void* data);

		uint32_t GetTextureID() const { return m_TextureID; }
		Onyx::Vector2D GetTextureSize() const { return Onyx::Vector2D(m_TextureWidth, m_TextureHeight); }
	private:
		uint32_t m_TextureID;
		unsigned char* m_TextureData;
		int m_TextureWidth, m_TextureHeight, m_NChannels;
	};

}