#pragma once

namespace Onyx {

	class Animation
	{
	public:
		Animation(const char* filePath, uint32_t width, uint32_t height, uint32_t spriteWidth, uint32_t spriteHeight);
		~Animation();

		void CreateAnimation(uint32_t index[]);
		void UseAnimation(uint32_t seg);
	private:
		uint32_t m_TextureID;
		unsigned char* m_TextureData;
		int m_TextureWidth, m_TextureHeight, m_NChannels;
	};

}