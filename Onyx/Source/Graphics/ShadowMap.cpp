#include "pch.h"

#include <GL/glew.h>
#include "ShadowMap.h"

namespace Onyx {

ShadowMap::ShadowMap() {
    Create();
}

ShadowMap::~ShadowMap() {
    Cleanup();
}

void ShadowMap::Cleanup() {
    if (m_FramebufferID) {
        glDeleteFramebuffers(1, &m_FramebufferID);
        m_FramebufferID = 0;
    }
    if (m_DepthTextureID) {
        glDeleteTextures(1, &m_DepthTextureID);
        m_DepthTextureID = 0;
    }
}

void ShadowMap::Create(uint32_t width, uint32_t height) {
    m_Width = width;
    m_Height = height;

    Cleanup();

    // Create framebuffer
    glGenFramebuffers(1, &m_FramebufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);

    // Create depth texture
    glGenTextures(1, &m_DepthTextureID);
    glBindTexture(GL_TEXTURE_2D, m_DepthTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Shadow map specific texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Border color for areas outside shadow map (white = no shadow)
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Enable hardware shadow comparison
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // Attach depth texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTextureID, 0);

    // No color buffer needed
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::SHADOWMAP:: Shadow map framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::Resize(uint32_t width, uint32_t height) {
    if (width == m_Width && height == m_Height)
        return;
    Create(width, height);
}

void ShadowMap::Bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);
    glViewport(0, 0, m_Width, m_Height);
}

void ShadowMap::UnBind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::Clear() const {
    glClear(GL_DEPTH_BUFFER_BIT);
}

}
