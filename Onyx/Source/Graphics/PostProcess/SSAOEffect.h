#pragma once

#include "PostProcessEffect.h"
#include <Graphics/RenderTarget.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Onyx {

	class SSAOEffect : public PostProcessEffect
	{
	public:
		explicit SSAOEffect(const std::string& shaderBasePath = "Onyx/Assets/Shaders/PostProcess/");

		void Init() override;
		void Resize(uint32_t width, uint32_t height) override;
		uint32_t Execute(const PostProcessContext& ctx) override;

		float& GetRadius() { return m_Radius; }
		float& GetBias() { return m_Bias; }
		float& GetIntensity() { return m_Intensity; }
		int& GetKernelSize() { return m_KernelSize; }

	private:
		void GenerateKernel();
		void BlitDepth(uint32_t srcFBO, uint32_t width, uint32_t height);
		void RenderSSAO(const PostProcessContext& ctx);
		void RenderBlur(const PostProcessContext& ctx);
		void RenderComposite(const PostProcessContext& ctx);

		std::string m_ShaderBasePath;

		float m_Radius = 0.5f;
		float m_Bias = 0.025f;
		float m_Intensity = 1.0f;
		int m_KernelSize = 64;

		std::unique_ptr<Shader> m_SSAOShader;
		std::unique_ptr<Shader> m_BlurShader;
		std::unique_ptr<Shader> m_CompositeShader;

		RenderTarget m_DepthTarget;
		RenderTarget m_SSAOTarget;
		RenderTarget m_BlurTarget;
		RenderTarget m_CompositeTarget;

		std::vector<glm::vec3> m_Kernel;
		std::unique_ptr<Texture> m_NoiseTexture;
	};

} // namespace Onyx
