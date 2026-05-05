#pragma once

namespace Onyx {

	class Framebuffer
	{
	public:
		Framebuffer();
		~Framebuffer();

		void Create(uint32_t width = 500, uint32_t height = 500, uint32_t samples = 1);

		void Bind() const;
		void UnBind() const;

		void Resize(uint32_t width, uint32_t height);

		// Resolve multisampled framebuffer to regular texture (call before displaying)
		void Resolve() const;

		void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) const;
		void EnableBlending() const;
		void DisableDepthTest() const;

		uint32_t GetColorBufferID() const { return m_ResolveColorBufferID; }
		uint32_t GetWidth() const { return m_FrameBufferWdith; }
		uint32_t GetHeight() const { return m_FrameBufferHeight; }
		uint32_t GetSamples() const { return m_Samples; }
		uint32_t GetFrameBufferID() const { return m_FrameBufferID; }

	private:
		void Cleanup();

		uint32_t m_FrameBufferWdith = 0;
		uint32_t m_FrameBufferHeight = 0;
		uint32_t m_Samples = 1;

		uint32_t m_FrameBufferID = 0;
		uint32_t m_ColorBufferID = 0;
		uint32_t m_DepthBufferID = 0;

		uint32_t m_ResolveFrameBufferID = 0;
		uint32_t m_ResolveColorBufferID = 0;
	};

} // namespace Onyx
