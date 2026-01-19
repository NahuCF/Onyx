#include "pch.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Framebuffer.h"

namespace Onyx {
	Framebuffer::Framebuffer()
	{
		Create();
	}

	Framebuffer::~Framebuffer()
	{
		Cleanup();
	}

	void Framebuffer::Cleanup()
	{
		if (m_FrameBufferID) {
			glDeleteFramebuffers(1, &m_FrameBufferID);
			m_FrameBufferID = 0;
		}
		if (m_ColorBufferID) {
			glDeleteTextures(1, &m_ColorBufferID);
			m_ColorBufferID = 0;
		}
		if (m_DepthBufferID) {
			glDeleteRenderbuffers(1, &m_DepthBufferID);
			m_DepthBufferID = 0;
		}
		if (m_ResolveFrameBufferID) {
			glDeleteFramebuffers(1, &m_ResolveFrameBufferID);
			m_ResolveFrameBufferID = 0;
		}
		if (m_ResolveColorBufferID) {
			glDeleteTextures(1, &m_ResolveColorBufferID);
			m_ResolveColorBufferID = 0;
		}
	}

	void Framebuffer::Create(uint32_t width, uint32_t height, uint32_t samples)
	{
		m_FrameBufferWdith = width;
		m_FrameBufferHeight = height;
		m_Samples = samples;

		Cleanup();

		bool multisampled = samples > 1;

		// Create main framebuffer
		glGenFramebuffers(1, &m_FrameBufferID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);

		if (multisampled) {
			// Multisampled color texture
			glGenTextures(1, &m_ColorBufferID);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ColorBufferID);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, width, height, GL_TRUE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_ColorBufferID, 0);

			// Multisampled depth/stencil renderbuffer
			glGenRenderbuffers(1, &m_DepthBufferID);
			glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBufferID);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBufferID);
		} else {
			// Regular color texture
			glCreateTextures(GL_TEXTURE_2D, 1, &m_ColorBufferID);
			glBindTexture(GL_TEXTURE_2D, m_ColorBufferID);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorBufferID, 0);

			// Regular depth/stencil renderbuffer
			glGenRenderbuffers(1, &m_DepthBufferID);
			glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBufferID);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBufferID);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Main framebuffer is not complete!" << std::endl;

		// Create resolve framebuffer (always non-multisampled for display)
		glGenFramebuffers(1, &m_ResolveFrameBufferID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_ResolveFrameBufferID);

		glCreateTextures(GL_TEXTURE_2D, 1, &m_ResolveColorBufferID);
		glBindTexture(GL_TEXTURE_2D, m_ResolveColorBufferID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ResolveColorBufferID, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Resolve framebuffer is not complete!" << std::endl;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Framebuffer::Bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
		glViewport(0, 0, m_FrameBufferWdith, m_FrameBufferHeight);
	}

	void Framebuffer::UnBind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Framebuffer::Resolve() const
	{
		// Blit from multisampled framebuffer to resolve framebuffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FrameBufferID);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolveFrameBufferID);
		glBlitFramebuffer(
			0, 0, m_FrameBufferWdith, m_FrameBufferHeight,
			0, 0, m_FrameBufferWdith, m_FrameBufferHeight,
			GL_COLOR_BUFFER_BIT,
			GL_NEAREST
		);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Framebuffer::Resize(uint32_t width, uint32_t height)
	{
		if (width == m_FrameBufferWdith && height == m_FrameBufferHeight)
			return;
		Create(width, height, m_Samples);
	}

	void Framebuffer::Clear(float r, float g, float b, float a) const
	{
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Framebuffer::EnableBlending() const
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void Framebuffer::DisableDepthTest() const
	{
		glDisable(GL_DEPTH_TEST);
	}

}
