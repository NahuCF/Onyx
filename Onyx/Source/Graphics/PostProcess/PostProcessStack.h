#pragma once

#include "PostProcessEffect.h"
#include <Graphics/Buffers.h>
#include <memory>
#include <vector>

namespace Onyx {

	class PostProcessStack
	{
	public:
		void Init();

		template <typename T, typename... Args>
		T* AddEffect(Args&&... args)
		{
			auto effect = std::make_unique<T>(std::forward<Args>(args)...);
			T* ptr = effect.get();
			effect->Init();
			m_Effects.push_back(std::move(effect));
			return ptr;
		}

		template <typename T>
		T* GetEffect() const
		{
			for (const auto& effect : m_Effects)
			{
				T* casted = dynamic_cast<T*>(effect.get());
				if (casted)
					return casted;
			}
			return nullptr;
		}

		void Resize(uint32_t width, uint32_t height);

		uint32_t Execute(uint32_t sceneColorTexture, uint32_t depthSourceFBO,
						 uint32_t width, uint32_t height, const glm::mat4& projection);

		bool HasEnabledEffects() const;

	private:
		std::vector<std::unique_ptr<PostProcessEffect>> m_Effects;

		std::unique_ptr<VertexArray> m_QuadVAO;
		std::unique_ptr<VertexBuffer> m_QuadVBO;
		std::unique_ptr<IndexBuffer> m_QuadEBO;
	};

} // namespace Onyx
