#pragma once

#include <cstdint>

namespace Onyx {

class RenderTarget {
public:
    enum class Format { R8, RGBA8, Depth };

    RenderTarget() = default;
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    void Create(uint32_t width, uint32_t height, Format format);
    void Bind() const;
    void Unbind() const;
    void BindTexture(uint32_t slot) const;

    uint32_t GetTextureID() const { return m_Texture; }
    uint32_t GetFBO() const { return m_FBO; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    void Cleanup();

    uint32_t m_FBO = 0;
    uint32_t m_Texture = 0;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};

}
