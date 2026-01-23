#include "CascadedShadowMap.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Onyx {

CascadedShadowMap::CascadedShadowMap() {
    m_LightSpaceMatrices.fill(glm::mat4(1.0f));
    m_CascadeSplits.fill(0.0f);
}

CascadedShadowMap::~CascadedShadowMap() {
    Cleanup();
}

void CascadedShadowMap::Create(uint32_t resolution) {
    Cleanup();
    m_Resolution = resolution;

    // Create framebuffer
    glGenFramebuffers(1, &m_FramebufferID);

    // Create texture array for all cascades
    glGenTextures(1, &m_DepthTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_DepthTextureArray);

    // Allocate storage for all cascade layers
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_DEPTH_COMPONENT32F,
        resolution,
        resolution,
        NUM_SHADOW_CASCADES,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Set border color to 1.0 (no shadow outside the map)
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Enable hardware shadow comparison
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    // Attach first layer to framebuffer for initial setup
    glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_DepthTextureArray, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Log error
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CascadedShadowMap::Resize(uint32_t resolution) {
    if (resolution != m_Resolution) {
        Create(resolution);
    }
}

void CascadedShadowMap::BindCascade(uint32_t cascadeIndex) const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_DepthTextureArray, 0, cascadeIndex);
    glViewport(0, 0, m_Resolution, m_Resolution);
}

void CascadedShadowMap::UnBind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CascadedShadowMap::ClearAll() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);
    for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_DepthTextureArray, 0, i);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CascadedShadowMap::Update(
    const glm::mat4& viewMatrix,
    const glm::mat4& projectionMatrix,
    const glm::vec3& lightDir,
    float nearPlane,
    float farPlane,
    float splitLambda
) {
    // Calculate cascade split distances using practical split scheme
    // Blend between logarithmic and uniform distributions
    float range = farPlane - nearPlane;
    float ratio = farPlane / nearPlane;

    for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++) {
        float p = (i + 1) / static_cast<float>(NUM_SHADOW_CASCADES);

        // Logarithmic split
        float logSplit = nearPlane * std::pow(ratio, p);

        // Uniform split
        float uniformSplit = nearPlane + range * p;

        // Blend between them
        m_CascadeSplits[i] = splitLambda * logSplit + (1.0f - splitLambda) * uniformSplit;
    }

    // Calculate light-space matrix for each cascade
    float lastSplit = nearPlane;
    for (uint32_t i = 0; i < NUM_SHADOW_CASCADES; i++) {
        // Get frustum corners for this cascade's range
        auto corners = GetFrustumCornersWorldSpace(projectionMatrix * viewMatrix, lastSplit, m_CascadeSplits[i]);

        // Calculate tight-fitting light space matrix
        m_LightSpaceMatrices[i] = CalculateLightSpaceMatrix(corners, lightDir);

        lastSplit = m_CascadeSplits[i];
    }
}

std::vector<glm::vec4> CascadedShadowMap::GetFrustumCornersWorldSpace(
    const glm::mat4& projView,
    float cascadeNear,
    float cascadeFar
) {
    // Get inverse of the projection-view matrix
    const glm::mat4 inv = glm::inverse(projView);

    // Get the full frustum corners in world space
    // NDC z=-1 is near plane, z=+1 is far plane (OpenGL convention)
    std::vector<glm::vec3> nearCorners;
    std::vector<glm::vec3> farCorners;

    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            // Near plane corner (z = -1 in NDC)
            glm::vec4 nearPt = inv * glm::vec4(
                2.0f * x - 1.0f,
                2.0f * y - 1.0f,
                -1.0f,
                1.0f
            );
            nearCorners.push_back(glm::vec3(nearPt) / nearPt.w);

            // Far plane corner (z = +1 in NDC)
            glm::vec4 farPt = inv * glm::vec4(
                2.0f * x - 1.0f,
                2.0f * y - 1.0f,
                1.0f,
                1.0f
            );
            farCorners.push_back(glm::vec3(farPt) / farPt.w);
        }
    }

    // Calculate the full frustum depth (distance from near to far along view direction)
    // Use the center points to estimate the camera's near/far
    glm::vec3 nearCenter(0.0f), farCenter(0.0f);
    for (int i = 0; i < 4; i++) {
        nearCenter += nearCorners[i];
        farCenter += farCorners[i];
    }
    nearCenter /= 4.0f;
    farCenter /= 4.0f;

    // The full frustum depth
    float fullDepth = glm::length(farCenter - nearCenter);

    // Interpolation factors for this cascade slice
    // cascadeNear and cascadeFar are depths from camera
    // Assume camera near plane is at depth ~0 for interpolation
    float tNear = cascadeNear / fullDepth;
    float tFar = cascadeFar / fullDepth;

    // Clamp to valid range
    tNear = glm::clamp(tNear, 0.0f, 1.0f);
    tFar = glm::clamp(tFar, 0.0f, 1.0f);

    // Interpolate corners to get the cascade sub-frustum
    std::vector<glm::vec4> corners;
    for (int i = 0; i < 4; i++) {
        glm::vec3 nearPt = nearCorners[i];
        glm::vec3 farPt = farCorners[i];

        // Interpolate along the frustum ray
        glm::vec3 cascadeNearPt = glm::mix(nearPt, farPt, tNear);
        glm::vec3 cascadeFarPt = glm::mix(nearPt, farPt, tFar);

        corners.push_back(glm::vec4(cascadeNearPt, 1.0f));
        corners.push_back(glm::vec4(cascadeFarPt, 1.0f));
    }

    return corners;
}

glm::mat4 CascadedShadowMap::CalculateLightSpaceMatrix(
    const std::vector<glm::vec4>& frustumCorners,
    const glm::vec3& lightDir
) {
    // Calculate frustum center
    glm::vec3 center(0.0f);
    for (const auto& corner : frustumCorners) {
        center += glm::vec3(corner);
    }
    center /= static_cast<float>(frustumCorners.size());

    // Calculate the radius of the bounding sphere for the frustum
    float radius = 0.0f;
    for (const auto& corner : frustumCorners) {
        float dist = glm::length(glm::vec3(corner) - center);
        radius = std::max(radius, dist);
    }

    // Round up to reduce shadow edge swimming when camera rotates
    radius = std::ceil(radius * 16.0f) / 16.0f;

    // Create light view matrix
    glm::vec3 lightDirNorm = glm::normalize(lightDir);
    const glm::mat4 lightView = glm::lookAt(
        center - lightDirNorm * radius * 2.0f,
        center,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Use a fixed-size ortho based on the bounding sphere
    // This prevents size flickering when camera moves
    float orthoSize = radius;

    // Create orthographic projection
    glm::mat4 lightProjection = glm::ortho(
        -orthoSize, orthoSize,
        -orthoSize, orthoSize,
        0.1f, radius * 4.0f
    );

    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // Texel snapping: round the origin to shadow map texel increments
    // This prevents sub-texel movement which causes flickering
    glm::vec4 shadowOrigin = lightSpaceMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    shadowOrigin = shadowOrigin * (static_cast<float>(m_Resolution) / 2.0f);

    glm::vec4 roundedOrigin = glm::round(shadowOrigin);
    glm::vec4 roundOffset = (roundedOrigin - shadowOrigin) * (2.0f / static_cast<float>(m_Resolution));

    lightProjection[3][0] += roundOffset.x;
    lightProjection[3][1] += roundOffset.y;

    return lightProjection * lightView;
}

void CascadedShadowMap::Cleanup() {
    if (m_FramebufferID != 0) {
        glDeleteFramebuffers(1, &m_FramebufferID);
        m_FramebufferID = 0;
    }
    if (m_DepthTextureArray != 0) {
        glDeleteTextures(1, &m_DepthTextureArray);
        m_DepthTextureArray = 0;
    }
}

}
