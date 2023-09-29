
#pragma once

#include "Maths/Maths.h"
#include "Buffers.h"
#include "Texture.h"
#include "Shader.h"

#include "Graphics/Window.h"

namespace se {

	struct BufferDisposition
	{
		lptm::Vector3D position;
		lptm::Vector4D color;
		lptm::Vector2D texCoord;
		float texIndex;
	};

	struct Renderer2DSpecification
	{
		static const uint32_t MaxQuads = 400 * 400;
		static const uint32_t Vertices = 4;
		static const uint32_t IndexBufferSize = MaxQuads * Vertices * 6;
		static const uint32_t VertexBufferSize = MaxQuads * Vertices * sizeof(BufferDisposition);

		static const uint32_t MaxTextureUnits = 32;
	};

	class Renderer2D
	{
	public:
		Renderer2D(Window& window);
		~Renderer2D();

		void RenderQuad(lptm::Vector2D size, lptm::Vector3D position, lptm::Vector4D color);
		void RenderQuad(lptm::Vector2D size, lptm::Vector3D position, const se::Texture& texture, const se::Shader* shader);
		void RenderQuad(lptm::Vector2D size, lptm::Vector3D position, const se::Texture* texture, const se::Shader* shader, lptm::Vector2D spriteCoord, lptm::Vector2D spriteSize);

		void RenderCircle(float radius, int subdivision, lptm::Vector3D position, lptm::Vector4D color);

		void RenderRotatedQuad(lptm::Vector2D size, lptm::Vector3D position, lptm::Vector4D color, float rotation);
		void RenderRotatedQuad(lptm::Vector2D size, lptm::Vector3D position, const se::Texture& texture, const se::Shader* shader, float rotation);
		void RenderRotatedQuad(lptm::Vector2D size, lptm::Vector3D position, const se::Texture& texture, const se::Shader* shader, lptm::Vector2D spriteCoord, lptm::Vector2D spriteSize, float rotation);

		void Flush();

        Window& GetWindow() { return m_Window; }
	private:
		void CleanBuffer();
		void CleanTextureUnits();
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

		int32_t m_TextureUnits[Renderer2DSpecification::MaxTextureUnits];
		const se::Shader* m_Shader = nullptr;

		Window& m_Window;
	};

}