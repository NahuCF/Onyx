#pragma once

namespace se { namespace graphics {

	class Texture
	{
	public:
		Texture(const char* texturePath, float Width, float Height);
		~Texture();
		void UseTexture() const;
	private:
		uint32_t m_TextureID;
		unsigned char* m_TextureData;
		int m_TextureWidth, m_TextureHeight, m_NChannels;
		uint32_t m_VAO, m_VBO, m_EBO;
	};

}}