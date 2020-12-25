#pragma once

namespace se { namespace graphics {

	class Texture
	{
	public:
		Texture(const char* texturePath, float Width, float Height);
		void UseTexture();
	private:
		unsigned int m_TextureID;
		unsigned char* m_TextureData;
		int m_TextureWidth, m_TextureHeight, m_NChannels;
		unsigned int m_VAO, m_VBO, m_EBO;
		float vertices[20];
		unsigned int indices[6];
	};

}}