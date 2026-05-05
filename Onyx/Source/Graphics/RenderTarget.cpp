#include "RenderTarget.h"
#include "pch.h"

#include <GL/glew.h>

namespace Onyx {

	RenderTarget::~RenderTarget()
	{
		Cleanup();
	}

	void RenderTarget::Cleanup()
	{
		if (m_FBO)
		{
			glDeleteFramebuffers(1, &m_FBO);
			m_FBO = 0;
		}
		if (m_Texture)
		{
			glDeleteTextures(1, &m_Texture);
			m_Texture = 0;
		}
		m_Width = 0;
		m_Height = 0;
	}

	void RenderTarget::Create(uint32_t width, uint32_t height, Format format)
	{
		if (width == 0 || height == 0)
			return;

		Cleanup();

		m_Width = width;
		m_Height = height;

		glGenFramebuffers(1, &m_FBO);
		glGenTextures(1, &m_Texture);
		glBindTexture(GL_TEXTURE_2D, m_Texture);

		switch (format)
		{
		case Format::Depth:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
						 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_Texture, 0);
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
			break;

		case Format::R8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Texture, 0);
			break;

		case Format::RGBA8:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Texture, 0);
			break;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void RenderTarget::Bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
		glViewport(0, 0, m_Width, m_Height);
	}

	void RenderTarget::Unbind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void RenderTarget::BindTexture(uint32_t slot) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, m_Texture);
	}

} // namespace Onyx
