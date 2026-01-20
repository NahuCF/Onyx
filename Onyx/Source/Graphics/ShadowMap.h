#pragma once

#include <cstdint>

namespace Onyx {

class ShadowMap {
public:
    ShadowMap();
    ~ShadowMap();

    void Create(uint32_t width = 2048, uint32_t height = 2048);
    void Resize(uint32_t width, uint32_t height);

    void Bind() const;
    void UnBind() const;
    void Clear() const;

    uint32_t GetDepthTextureID() const { return m_DepthTextureID; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    void Cleanup();

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_FramebufferID = 0;
    uint32_t m_DepthTextureID = 0;
};

}
