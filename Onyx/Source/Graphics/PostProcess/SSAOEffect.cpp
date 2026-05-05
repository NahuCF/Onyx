#include "SSAOEffect.h"
#include "pch.h"
#include <Graphics/Buffers.h>
#include <Graphics/RenderCommand.h>
#include <algorithm>
#include <random>

namespace Onyx {

	SSAOEffect::SSAOEffect(const std::string& shaderBasePath)
		: m_ShaderBasePath(shaderBasePath)
	{
		m_Name = "SSAO";
	}

	void SSAOEffect::Init()
	{
		std::string vertPath = m_ShaderBasePath + "ssao.vert";
		std::string ssaoFragPath = m_ShaderBasePath + "ssao.frag";
		std::string blurFragPath = m_ShaderBasePath + "ssao_blur.frag";
		std::string compositeFragPath = m_ShaderBasePath + "ssao_composite.frag";

		m_SSAOShader = std::make_unique<Shader>(vertPath.c_str(), ssaoFragPath.c_str());
		m_BlurShader = std::make_unique<Shader>(vertPath.c_str(), blurFragPath.c_str());
		m_CompositeShader = std::make_unique<Shader>(vertPath.c_str(), compositeFragPath.c_str());

		GenerateKernel();

		// Create 4x4 noise texture
		std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		std::default_random_engine gen;
		std::vector<glm::vec3> noiseData(16);
		for (int i = 0; i < 16; i++)
		{
			noiseData[i] = glm::vec3(dist(gen), dist(gen), 0.0f);
		}

		m_NoiseTexture = Texture::CreateNoiseTexture(
			reinterpret_cast<const float*>(noiseData.data()), 4, 4);
	}

	void SSAOEffect::GenerateKernel()
	{
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		std::default_random_engine gen;

		m_Kernel.resize(64);
		for (int i = 0; i < 64; i++)
		{
			glm::vec3 sample(
				dist(gen) * 2.0f - 1.0f,
				dist(gen) * 2.0f - 1.0f,
				dist(gen) // hemisphere: z in [0, 1]
			);
			sample = glm::normalize(sample) * dist(gen);

			// Accelerating interpolation: more samples closer to origin
			float scale = static_cast<float>(i) / 64.0f;
			scale = 0.1f + scale * scale * 0.9f;
			sample *= scale;
			m_Kernel[i] = sample;
		}
	}

	void SSAOEffect::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
			return;

		m_DepthTarget.Create(width, height, RenderTarget::Format::Depth);
		m_SSAOTarget.Create(width, height, RenderTarget::Format::R8);
		m_BlurTarget.Create(width, height, RenderTarget::Format::R8);
		m_CompositeTarget.Create(width, height, RenderTarget::Format::RGBA8);
	}

	uint32_t SSAOEffect::Execute(const PostProcessContext& ctx)
	{
		// Ensure targets are the right size
		if (ctx.width != m_DepthTarget.GetWidth() || ctx.height != m_DepthTarget.GetHeight())
		{
			Resize(ctx.width, ctx.height);
		}

		BlitDepth(ctx.depthSourceFBO, ctx.width, ctx.height);
		RenderSSAO(ctx);
		RenderBlur(ctx);
		RenderComposite(ctx);

		return m_CompositeTarget.GetTextureID();
	}

	void SSAOEffect::BlitDepth(uint32_t srcFBO, uint32_t width, uint32_t height)
	{
		RenderCommand::BlitDepth(srcFBO, m_DepthTarget.GetFBO(), width, height);
	}

	void SSAOEffect::RenderSSAO(const PostProcessContext& ctx)
	{
		m_SSAOTarget.Bind();
		RenderCommand::Clear();
		RenderCommand::DisableDepthTest();

		m_SSAOShader->Bind();

		// Upload kernel samples
		int kernelSize = std::min(m_KernelSize, 64);
		for (int i = 0; i < kernelSize; i++)
		{
			m_SSAOShader->SetVec3("u_Samples[" + std::to_string(i) + "]", m_Kernel[i]);
		}

		m_SSAOShader->SetMat4("u_Projection", ctx.projection);
		glm::mat4 invProj = glm::inverse(ctx.projection);
		m_SSAOShader->SetMat4("u_InvProjection", invProj);
		m_SSAOShader->SetVec2("u_NoiseScale",
							  static_cast<float>(m_SSAOTarget.GetWidth()) / 4.0f,
							  static_cast<float>(m_SSAOTarget.GetHeight()) / 4.0f);
		m_SSAOShader->SetInt("u_KernelSize", kernelSize);
		m_SSAOShader->SetFloat("u_Radius", m_Radius);
		m_SSAOShader->SetFloat("u_Bias", m_Bias);
		m_SSAOShader->SetFloat("u_Intensity", m_Intensity);

		// Bind textures
		m_SSAOShader->SetInt("u_DepthTexture", 0);
		m_DepthTarget.BindTexture(0);

		m_SSAOShader->SetInt("u_NoiseTexture", 1);
		m_NoiseTexture->Bind(1);

		RenderCommand::DrawIndexed(*ctx.fullscreenQuad, 6);

		m_SSAOTarget.Unbind();
	}

	void SSAOEffect::RenderBlur(const PostProcessContext& ctx)
	{
		m_BlurTarget.Bind();
		RenderCommand::Clear();
		RenderCommand::DisableDepthTest();

		m_BlurShader->Bind();
		m_BlurShader->SetInt("u_SSAOInput", 0);
		m_SSAOTarget.BindTexture(0);

		m_BlurShader->SetInt("u_DepthTexture", 1);
		m_DepthTarget.BindTexture(1);

		RenderCommand::DrawIndexed(*ctx.fullscreenQuad, 6);

		m_BlurTarget.Unbind();
	}

	void SSAOEffect::RenderComposite(const PostProcessContext& ctx)
	{
		m_CompositeTarget.Bind();
		RenderCommand::Clear();
		RenderCommand::DisableDepthTest();

		m_CompositeShader->Bind();

		m_CompositeShader->SetInt("u_SceneColor", 0);
		RenderCommand::BindTexture2D(0, ctx.inputColorTexture);

		m_CompositeShader->SetInt("u_SSAOBlurred", 1);
		m_BlurTarget.BindTexture(1);

		RenderCommand::DrawIndexed(*ctx.fullscreenQuad, 6);

		m_CompositeTarget.Unbind();
	}

} // namespace Onyx
