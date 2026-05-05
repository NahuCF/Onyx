#include "RenderCommand.h"
#include "Buffers.h"
#include "pch.h"

#include <GL/glew.h>

namespace Onyx {

	void RenderCommand::DrawIndexed(const VertexArray& vao, uint32_t indexCount)
	{
		vao.Bind();
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
	}

	void RenderCommand::DrawArrays(const VertexArray& vao, uint32_t vertexCount)
	{
		vao.Bind();
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}

	void RenderCommand::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void RenderCommand::SetClearColor(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);
	}

	void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void RenderCommand::EnableBlending()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void RenderCommand::DisableBlending()
	{
		glDisable(GL_BLEND);
	}

	void RenderCommand::EnableDepthTest()
	{
		glEnable(GL_DEPTH_TEST);
	}

	void RenderCommand::DisableDepthTest()
	{
		glDisable(GL_DEPTH_TEST);
	}

	void RenderCommand::SetDepthMask(bool write)
	{
		glDepthMask(write ? GL_TRUE : GL_FALSE);
	}

	void RenderCommand::EnableCulling()
	{
		glEnable(GL_CULL_FACE);
	}

	void RenderCommand::DisableCulling()
	{
		glDisable(GL_CULL_FACE);
	}

	void RenderCommand::SetCullFace(bool front)
	{
		glCullFace(front ? GL_FRONT : GL_BACK);
	}

	void RenderCommand::BindTextureArray(uint32_t slot, uint32_t textureId)
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textureId);
	}

	void RenderCommand::BindTexture2D(uint32_t slot, uint32_t textureId)
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, textureId);
	}

	void RenderCommand::BlitDepth(uint32_t srcFBO, uint32_t dstFBO, uint32_t width, uint32_t height)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFBO);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void RenderCommand::ReadPixels(int x, int y, int width, int height, unsigned char* data)
	{
		glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}

	void RenderCommand::Finish()
	{
		glFinish();
	}

	void RenderCommand::EnablePolygonOffset(float factor, float units)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(factor, units);
	}

	void RenderCommand::DisablePolygonOffset()
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void RenderCommand::SetWireframeMode(bool enabled)
	{
		if (enabled)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	void RenderCommand::SetLineWidth(float width)
	{
		glLineWidth(width);
	}

	void RenderCommand::DrawLines(const VertexArray& vao, uint32_t vertexCount)
	{
		vao.Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void RenderCommand::DrawLineLoop(const VertexArray& vao, uint32_t vertexCount)
	{
		vao.Bind();
		glDrawArrays(GL_LINE_LOOP, 0, vertexCount);
	}

	void RenderCommand::DrawLineStrip(const VertexArray& vao, uint32_t vertexCount)
	{
		vao.Bind();
		glDrawArrays(GL_LINE_STRIP, 0, vertexCount);
	}

	void RenderCommand::DrawBatched(const VertexArray& vao, uint32_t drawCount)
	{
		vao.Bind();
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
									static_cast<GLsizei>(drawCount), 0);
	}

	void RenderCommand::DrawBatchedIndirectCount(const VertexArray& vao, uint32_t countBufferID,
												 uint32_t maxDrawCount)
	{
		vao.Bind();
		glBindBuffer(GL_PARAMETER_BUFFER, countBufferID);
		glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
										 0, static_cast<GLsizei>(maxDrawCount), 0);
		glBindBuffer(GL_PARAMETER_BUFFER, 0);
	}

	void RenderCommand::DispatchCompute(uint32_t x, uint32_t y, uint32_t z)
	{
		glDispatchCompute(x, y, z);
	}

	void RenderCommand::MemoryBarrier(uint32_t barriers)
	{
		glMemoryBarrier(barriers);
	}

	void RenderCommand::ResetState()
	{
		glBindVertexArray(0);
		glUseProgram(0);
	}

} // namespace Onyx
