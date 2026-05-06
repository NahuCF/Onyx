#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>

namespace Onyx {

	class VertexArray;
	class Texture;

	struct SubmitMeshView
	{
		uint32_t indexCount = 0;
		uint32_t firstIndex = 0;
		int32_t  baseVertex = 0;
		glm::vec3 localMin{0.0f};
		glm::vec3 localMax{0.0f};
	};

	class IRenderable
	{
	public:
		virtual ~IRenderable() = default;

		virtual const VertexArray* GetMergedVAO() const = 0;
		virtual size_t GetMeshCount() const = 0;
		virtual SubmitMeshView GetMeshView(size_t meshIndex) const = 0;

		virtual Texture* GetMeshAlbedoOverride(size_t meshIndex) const { (void)meshIndex; return nullptr; }
		virtual Texture* GetMeshNormalOverride(size_t meshIndex) const { (void)meshIndex; return nullptr; }

		// 0x1405 = GL_UNSIGNED_INT (avoid pulling GL headers into this interface)
		virtual uint32_t GetIndexType() const { return 0x1405; }

		virtual bool IsSkinned() const { return false; }
	};

	class ISkinnedRenderable : public IRenderable
	{
	public:
		bool IsSkinned() const final { return true; }
		virtual size_t GetBoneCount() const = 0;
		virtual const glm::mat4* GetBoneMatricesPtr() const = 0;
	};

} // namespace Onyx
