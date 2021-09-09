#pragma once

#include <vector>

#include "Maths/Maths.h"
#include "Buffers.h"

namespace se {

	struct BufferDisposition
	{
		lptm::Vector3D position;
		lptm::Vector4D color;
	};

	struct Renderer2DSpecification
	{
		static const uint32_t MaxQuads = 400 * 400;
		static const uint32_t Vertices = 4;
		static const uint32_t IndexBufferSize = MaxQuads * Vertices * 6;
		static const uint32_t VertexBufferSize = MaxQuads * Vertices * sizeof(BufferDisposition);
	};

	class Renderer2D
	{
	public:
		Renderer2D();
		~Renderer2D();

		void RenderQuad(lptm::Vector2D size, lptm::Vector3D position, lptm::Vector4D color);

		void Flush();
	private:
		void CleanBuffer();
	private:
		float* m_VertexBufferData = new float[Renderer2DSpecification::VertexBufferSize];
		uint32_t* m_IndexBufferData = new uint32_t[Renderer2DSpecification::IndexBufferSize];

		VertexArray* m_VAO;
		VertexBuffer* m_VBO;
		IndexBuffer* m_EBO;

		uint32_t m_IndexCount = 0;
		uint32_t m_VertexCount = 0;

		uint32_t m_VertexBufferOffset = 0;
		uint32_t m_IndexBufferOffset = 0;
	};

}