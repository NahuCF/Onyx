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

	void Renderer2D::RenderQuad(lptm::Vector2D size, lptm::Vector3D position, const se::Texture& texture, const se::Shader* shader)
	{
		int textureUnit = texture.GetTextureID() - 1;

		float vertices[] = {
			-size.x + position.x,  size.y + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		0.0f, 1.0f,		(float)textureUnit,
			 size.x + position.x,  size.y + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		1.0f, 1.0f,		(float)textureUnit,
			 size.x + position.x, -size.y + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		1.0f, 0.0f,		(float)textureUnit,
			-size.x + position.x, -size.y + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		0.0f, 0.0f,		(float)textureUnit
		};

		for (uint32_t i = 0; i < sizeof(vertices) / sizeof(float); i++)
			m_VertexBufferData[m_VertexBufferOffset + i] = vertices[i];

		m_VertexCount += 4;
		m_IndexCount += 6;
		m_VertexBufferOffset += sizeof(vertices) / sizeof(float);

		m_Shader = shader;
		m_TextureUnits[textureUnit] = textureUnit;
		glBindTextureUnit(textureUnit, texture.GetTextureID());
	}

	void Renderer2D::Flush()
	{
		glUniform1iv(glGetUniformLocation(m_Shader->GetProgramID(), "u_Textures"), Renderer2DSpecification::MaxTextureUnits, m_TextureUnits);

		m_VAO->Bind();
		m_VBO->Bind();
		m_EBO->Bind();

		m_VAO->AddBuffer(3, 10, offsetof(BufferDisposition, position), 0);
		m_VAO->AddBuffer(4, 10, offsetof(BufferDisposition, color), 1);
		m_VAO->AddBuffer(2, 10, offsetof(BufferDisposition, texCoord), 2);
		m_VAO->AddBuffer(1, 10, offsetof(BufferDisposition, texIndex), 3);

		glBufferSubData(GL_ARRAY_BUFFER, 0, m_VertexCount * sizeof(BufferDisposition), m_VertexBufferData);
		glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);

		m_VBO->UnBind();
		m_VAO->UnBind();
		m_EBO->UnBind();

		m_IndexCount = 0;
		m_VertexCount = 0;
		m_VertexBufferOffset = 0;
		m_NextTextureUnit = 0;
		CleanBuffer();
		CleanTextureUnits();
	}

	void Renderer2D::CleanBuffer()
	{
		for (uint32_t i = 0; i < sizeof(Renderer2DSpecification::VertexBufferSize); i++)
			m_VertexBufferData[i] = 0.0f;
	}

	void Renderer2D::CleanTextureUnits()
	{
		for (uint32_t i = 0; i < Renderer2DSpecification::MaxTextureUnits; i++)
			m_TextureUnits[i] = 0;
	}

}
