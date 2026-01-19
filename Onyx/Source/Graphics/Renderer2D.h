#pragma once

#include <memory>
#include "Maths/Maths.h"
#include "Buffers.h"
#include "Texture.h"
#include "Shader.h"
#include "Camera.h"

#include "Graphics/Window.h"

namespace Onyx {

	struct BufferDisposition
	{
		Onyx::Vector3 position;
		Onyx::Vector4D color;
		Onyx::Vector2D texCoord;
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

	struct Renderer2DStats
	{
		uint32_t DrawCalls = 0;
		uint32_t QuadCount = 0;

		void Reset() { DrawCalls = 0; QuadCount = 0; }
	};

	class Renderer2D
	{
	public:
		Renderer2D(Window& window, std::shared_ptr<Camera> camera);
		~Renderer2D();

		void RenderQuad(Onyx::Vector2D size, Onyx::Vector3 position, Onyx::Vector4D color);
		void RenderQuad(Onyx::Vector2D size, Onyx::Vector3 position, const Onyx::Texture& texture, const Onyx::Shader* shader);
		void RenderQuad(Onyx::Vector2D size, Onyx::Vector3 position, const Onyx::Texture* texture, const Onyx::Shader* shader, Onyx::Vector2D spriteCoord, Onyx::Vector2D spriteSize);

		// Screen-space rendering (for editor/UI - position and size in pixels)
		void BeginScreenSpace(float viewportX, float viewportY, float viewportWidth, float viewportHeight);
		void RenderQuadScreenSpace(Onyx::Vector2D position, Onyx::Vector2D size, const Onyx::Texture* texture, Onyx::Vector2D uvMin, Onyx::Vector2D uvMax);
		void EndScreenSpace();

		// For ImGui callback-based rendering
		void UploadScreenSpaceData();
		void DrawScreenSpaceBatch(uint32_t indexCount);
		uint32_t GetScreenSpaceVertexCount() const { return m_VertexCount; }
		uint32_t GetScreenSpaceIndexCount() const { return m_IndexCount; }
		float* GetScreenSpaceViewport() { return m_ScreenSpaceViewport; }

        void RenderRotatedLine(Onyx::Vector2D start, Onyx::Vector2D end, float width, Onyx::Vector4D color, float rotation);

		void RenderCircle(float radius, int subdivision, Onyx::Vector3 position, Onyx::Vector4D color);

		void RenderRotatedQuad(Onyx::Vector2D size, Onyx::Vector3 position, Onyx::Vector4D color, float rotation);
		void RenderRotatedQuad(Onyx::Vector2D size, Onyx::Vector3 position, const Onyx::Texture& texture, const Onyx::Shader* shader, float rotation);
		void RenderRotatedQuad(Onyx::Vector2D size, Onyx::Vector3 position, const Onyx::Texture& texture, const Onyx::Shader* shader, Onyx::Vector2D spriteCoord, Onyx::Vector2D spriteSize, float rotation);

		void Flush();

        std::weak_ptr<Onyx::Camera> GetCamera() const { return m_Camera; }

        Window& GetWindow() { return m_Window; }

		// Stats
		const Renderer2DStats& GetStats() const { return m_Stats; }
		Renderer2DStats& GetStats() { return m_Stats; }
		void ResetStats() { m_Stats.Reset(); }
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
		const Onyx::Shader* m_Shader = nullptr;
		std::unique_ptr<Onyx::Shader> m_DefaultShader;

		std::weak_ptr<Onyx::Camera> m_Camera;

		Window& m_Window;

		// Screen-space rendering state
		bool m_ScreenSpaceMode = false;
		float m_ScreenSpaceViewport[4] = { 0, 0, 0, 0 }; // x, y, width, height

		// Stats
		Renderer2DStats m_Stats;
	};

}