#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace Onyx {

	class VertexArray;

	struct PostProcessContext
	{
		uint32_t inputColorTexture; // from previous stage or resolved scene
		uint32_t depthSourceFBO;	// MSAA FBO to blit depth from
		uint32_t width, height;
		glm::mat4 projection;
		const VertexArray* fullscreenQuad;
	};

	class PostProcessEffect
	{
	public:
		virtual ~PostProcessEffect() = default;
		virtual void Init() = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual uint32_t Execute(const PostProcessContext& ctx) = 0;

		bool& GetEnabled() { return m_Enabled; }
		bool IsEnabled() const { return m_Enabled; }
		void SetEnabled(bool e) { m_Enabled = e; }
		const std::string& GetName() const { return m_Name; }

	protected:
		bool m_Enabled = false;
		std::string m_Name;
	};

} // namespace Onyx
