#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <glm/glm.hpp>

namespace Onyx {

// Number of shadow cascades
constexpr uint32_t NUM_SHADOW_CASCADES = 4;

class CascadedShadowMap {
public:
    CascadedShadowMap();
    ~CascadedShadowMap();

    void Create(uint32_t resolution = 2048);
    void Resize(uint32_t resolution);

    // Bind specific cascade for rendering
    void BindCascade(uint32_t cascadeIndex) const;
    void UnBind() const;
    void ClearAll() const;

    // Calculate cascade splits and light matrices
    void Update(
        const glm::mat4& viewMatrix,
        const glm::mat4& projectionMatrix,
        const glm::vec3& lightDir,
        float nearPlane,
        float farPlane,
        float splitLambda = 0.75f  // Blend between logarithmic and uniform splits
    );

    // Getters
    uint32_t GetResolution() const { return m_Resolution; }
    uint32_t GetDepthTextureArray() const { return m_DepthTextureArray; }

    const glm::mat4& GetLightSpaceMatrix(uint32_t cascade) const { return m_LightSpaceMatrices[cascade]; }
    const std::array<glm::mat4, NUM_SHADOW_CASCADES>& GetLightSpaceMatrices() const { return m_LightSpaceMatrices; }

    float GetCascadeSplit(uint32_t cascade) const { return m_CascadeSplits[cascade]; }
    const std::array<float, NUM_SHADOW_CASCADES>& GetCascadeSplits() const { return m_CascadeSplits; }

private:
    void Cleanup();

    // Calculate frustum corners in world space for a given near/far range
    std::vector<glm::vec4> GetFrustumCornersWorldSpace(
        const glm::mat4& projView,
        float nearPlane,
        float farPlane
    );

    // Calculate tight ortho projection for a cascade
    glm::mat4 CalculateLightSpaceMatrix(
        const std::vector<glm::vec4>& frustumCorners,
        const glm::vec3& lightDir
    );

    uint32_t m_Resolution = 0;
    uint32_t m_FramebufferID = 0;
    uint32_t m_DepthTextureArray = 0;  // GL_TEXTURE_2D_ARRAY

    std::array<glm::mat4, NUM_SHADOW_CASCADES> m_LightSpaceMatrices;
    std::array<float, NUM_SHADOW_CASCADES> m_CascadeSplits;  // Far plane of each cascade (in view space)
};

}
