#pragma once

namespace se {

	class Framebuffer
	{
	public:
		Framebuffer();
		~Framebuffer();

		void Create(uint32_t width = 500, uint32_t height = 500);

		void Bind() const;
		void UnBind() const;

		void Resize(uint32_t width, uint32_t height);

		uint32_t GetColorBufferID() const { return m_ColorBufferID; }
	private:
		uint32_t m_FrameBufferWdith, m_FrameBufferHeight;

		uint32_t m_FrameBufferID;
		uint32_t m_ColorBufferID = 0;
		uint32_t m_DepthBufferID;
	};

}