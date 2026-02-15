#pragma once

#include <cstdint>
#include <string>

namespace Onyx {

class TextureArray {
public:
    TextureArray();
    ~TextureArray();

    void Create(int width, int height, int layers, int channels);
    void SetLayerData(int layer, const void* data);
    bool LoadLayerFromFile(const std::string& path, int layer);
    void SetLayerSolidColor(int layer, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    void Bind(uint32_t slot) const;
    void UnBind() const;

    uint32_t GetTextureID() const { return m_TextureID; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    int GetLayers() const { return m_Layers; }

private:
    uint32_t m_TextureID = 0;
    int m_Width = 0;
    int m_Height = 0;
    int m_Layers = 0;
    int m_Channels = 4;
};

} // namespace Onyx
