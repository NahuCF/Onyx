#pragma once

#include <cstdint>

namespace Onyx {

	class VertexArray;

	class RenderCommand
	{
	public:
		static void DrawIndexed(const VertexArray& vao, uint32_t indexCount);
		static void DrawArrays(const VertexArray& vao, uint32_t vertexCount);
		static void DrawLines(const VertexArray& vao, uint32_t vertexCount);
		static void DrawLineLoop(const VertexArray& vao, uint32_t vertexCount);
		static void DrawLineStrip(const VertexArray& vao, uint32_t vertexCount);

		static void DrawBatched(const VertexArray& vao, uint32_t drawCount, uint32_t indexType = 0x1405 /* GL_UNSIGNED_INT */);
		static void DrawBatchedIndirectCount(const VertexArray& vao, uint32_t countBufferID,
											 uint32_t maxDrawCount);
		static void DispatchCompute(uint32_t x, uint32_t y, uint32_t z);
		static void MemoryBarrier(uint32_t barriers);

		static void Clear();
		static void SetClearColor(float r, float g, float b, float a);
		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

		static void EnableBlending();
		static void DisableBlending();

		static void EnableDepthTest();
		static void DisableDepthTest();
		static void SetDepthMask(bool write);

		static void EnableCulling();
		static void DisableCulling();
		static void SetCullFace(bool front);

		static void EnablePolygonOffset(float factor, float units);
		static void DisablePolygonOffset();

		static void SetWireframeMode(bool enabled);
		static void SetLineWidth(float width);

		static void BindTextureArray(uint32_t slot, uint32_t textureId);
		static void BindTexture2D(uint32_t slot, uint32_t textureId);
		static void BlitDepth(uint32_t srcFBO, uint32_t dstFBO, uint32_t width, uint32_t height);
		static void ReadPixels(int x, int y, int width, int height, unsigned char* data);
		static void Finish();

		static void ResetState();
	};

} // namespace Onyx
