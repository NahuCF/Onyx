#include "pch.h"

#include "GL/glew.h"

#include "Renderer2D.h"

namespace se {

	Renderer2D::Renderer2D()
		: m_VAO(new se::VertexArray), m_VBO(new se::VertexBuffer(Renderer2DSpecification::BufferSize))
	{}

	Renderer2D::~Renderer2D() 
	{
		delete m_VAO;
		delete m_VBO;
		delete m_BufferData;
	}

	void Renderer2D::RenderQuad(lptm::Vector2D size, lptm::Vector3D position, lptm::Vector4D color)
	{
		float vertices[] = {
			-size.x + position.x,  size.y + position.y, position.z,		color.x, color.y, color.z, color.w,
			 size.x + position.x,  size.y + position.y, position.z,		color.x, color.y, color.z, color.w,
			 size.x + position.x, -size.y + position.y, position.z,		color.x, color.y, color.z, color.w,

			-size.x + position.x,  size.y + position.y, position.z,		color.x, color.y, color.z, color.w,
			 size.x + position.x, -size.y + position.y, position.z,		color.x, color.y, color.z, color.w,
			-size.x + position.x, -size.y + position.y, position.z,		color.x, color.y, color.z, color.w
		};
	
		for(uint32_t i = 0; i < sizeof(vertices) / sizeof(float); i++)
			m_BufferData[m_BufferIndex + i] = vertices[i];

		m_QuadCounter++;
		m_BufferIndex += sizeof(vertices) / sizeof(float);
	}

	void Renderer2D::Flush()
	{
		m_VAO->Bind();
		m_VBO->Bind();

		m_VAO->AddBuffer(3, 7, offsetof(BufferDisposition, position), 0);
		m_VAO->AddBuffer(4, 7, offsetof(BufferDisposition, color), 1);

		glBufferSubData(GL_ARRAY_BUFFER, 0, Renderer2DSpecification::BufferSize, m_BufferData);
		glDrawArrays(GL_TRIANGLES, 0, m_QuadCounter);

		m_VAO->UnBind();
		m_VBO->UnBind();

		m_QuadCounter = 0;
		m_BufferIndex = 0;
		CleanBuffer();
	}

	void Renderer2D::CleanBuffer()
	{
		for (uint32_t i = 0; i < sizeof(Renderer2DSpecification::BufferSize); i++)
			m_BufferData[i] = 0.0f;
	}

}
