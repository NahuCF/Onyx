#include "pch.h"
#include "PostProcessStack.h"
#include <Graphics/VertexLayout.h>

namespace Onyx {

void PostProcessStack::Init() {
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    uint32_t quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    m_QuadVBO = std::make_unique<VertexBuffer>(static_cast<const void*>(quadVertices), sizeof(quadVertices));
    m_QuadEBO = std::make_unique<IndexBuffer>(static_cast<const void*>(quadIndices), sizeof(quadIndices));
    m_QuadVAO = std::make_unique<VertexArray>();

    VertexLayout layout;
    layout.PushFloat(2);

    m_QuadVAO->SetVertexBuffer(m_QuadVBO.get());
    m_QuadVAO->SetIndexBuffer(m_QuadEBO.get());
    m_QuadVAO->SetLayout(layout);
}

void PostProcessStack::Resize(uint32_t width, uint32_t height) {
    for (auto& effect : m_Effects) {
        effect->Resize(width, height);
    }
}

uint32_t PostProcessStack::Execute(uint32_t sceneColorTexture, uint32_t depthSourceFBO,
                                   uint32_t width, uint32_t height, const glm::mat4& projection) {
    uint32_t currentInput = sceneColorTexture;

    PostProcessContext ctx;
    ctx.depthSourceFBO = depthSourceFBO;
    ctx.width = width;
    ctx.height = height;
    ctx.projection = projection;
    ctx.fullscreenQuad = m_QuadVAO.get();

    for (auto& effect : m_Effects) {
        if (!effect->IsEnabled()) continue;
        ctx.inputColorTexture = currentInput;
        currentInput = effect->Execute(ctx);
    }

    return currentInput;
}

bool PostProcessStack::HasEnabledEffects() const {
    for (const auto& effect : m_Effects) {
        if (effect->IsEnabled()) return true;
    }
    return false;
}

}
