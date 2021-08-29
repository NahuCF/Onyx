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
		static const uint32_t MaxQuads = 100000;
		static const uint32_t Vertices = 6;
		static const uint32_t BufferSize = MaxQuads * Vertices * sizeof(BufferDisposition);
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
		float* m_BufferData = new float[Renderer2DSpecification::BufferSize];
		uint32_t m_BufferIndex = 0;

		VertexArray* m_VAO;
		VertexBuffer* m_VBO;

		uint32_t m_QuadCounter = 0;
	};

}