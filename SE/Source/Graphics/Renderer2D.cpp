#include "pch.h"

#include "GL/glew.h"

#include "Renderer2D.h"

namespace se {

	Renderer2D::Renderer2D()
		: m_VAO(new se::VertexArray), m_VBO(new se::VertexBuffer(Renderer2DSpecification::VertexBufferSize))
	{
		for (uint32_t i = 0; i < Renderer2DSpecification::IndexBufferSize; i += 6)
		{
			m_IndexBufferData[i + 0] = 0 + m_IndexBufferOffset;
			m_IndexBufferData[i + 1] = 1 + m_IndexBufferOffset;
			m_IndexBufferData[i + 2] = 2 + m_IndexBufferOffset;

			m_IndexBufferData[i + 3] = 2 + m_IndexBufferOffset;
			m_IndexBufferData[i + 4] = 3 + m_IndexBufferOffset;
			m_IndexBufferData[i + 5] = 0 + m_IndexBufferOffset;

			m_IndexBufferOffset += 4;
		}

		m_EBO = new IndexBuffer(m_IndexBufferData, Renderer2DSpecification::IndexBufferSize);
	}

	Renderer2D::~Renderer2D() 
	{
		delete m_VAO;
		delete m_VBO;
		delete m_EBO;
		delete m_VertexBufferData;
		delete m_IndexBufferData;
	}

	void Renderer2D::RenderQuad(lptm::Vector2D size, lptm::Vector3D position, lptm::Vector4D color)
	{
		float vertices[] = {
			-size.x + position.x,  size.y + position.y, position.z,		color.x, color.y, color.z, color.w,
			 size.x + position.x,  size.y + position.y, position.z,		color.x, color.y, color.z, color.w,
			 size.x + position.x, -size.y + position.y, position.z,		color.x, color.y, color.z, color.w,
			-size.x + position.x, -size.y + position.y, position.z,		color.x, color.y, color.z, color.w
		};
	
		for(uint32_t i = 0; i < sizeof(vertices) / sizeof(float); i++)
			m_VertexBufferData[m_VertexBufferOffset + i] = vertices[i];

		m_VertexCount += 4;
		m_IndexCount += 6;
		m_VertexBufferOffset += sizeof(vertices) / sizeof(float);
	}

	void Renderer2D::Flush()
	{
		m_VAO->Bind();
		m_VBO->Bind();
		m_EBO->Bind();

		m_VAO->AddBuffer(3, 7, offsetof(BufferDisposition, position), 0);
		m_VAO->AddBuffer(4, 7, offsetof(BufferDisposition, color), 1);

		glBufferSubData(GL_ARRAY_BUFFER, 0, m_VertexCount * sizeof(BufferDisposition), m_VertexBufferData);
		glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);

		m_VBO->UnBind();
		m_VAO->UnBind();
		m_EBO->UnBind();

		m_IndexCount = 0;
		m_VertexCount = 0;
		m_VertexBufferOffset = 0;
		CleanBuffer();
	}

	void Renderer2D::CleanBuffer()
	{
		for (uint32_t i = 0; i < sizeof(Renderer2DSpecification::VertexBufferSize); i++)
			m_VertexBufferData[i] = 0.0f;
	}

}
