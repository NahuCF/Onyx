#include "pch.h"

#include "Renderer2D.h"

namespace se {

	Renderer2D::Renderer2D(Window& window)
		: m_VAO(new se::VertexArray)
		, m_VBO(new se::VertexBuffer(Renderer2DSpecification::VertexBufferSize))
		, m_Window(window)
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
		float aspectRatio = m_Window.GetAspectRatio();

		float vertices[] = {
			-size.x + position.x,  size.y * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			 size.x + position.x,  size.y * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			 size.x + position.x, -size.y * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			-size.x + position.x, -size.y * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f
		};
	
		for(uint32_t i = 0; i < sizeof(vertices) / sizeof(float); i++)
			m_VertexBufferData[m_VertexBufferOffset + i] = vertices[i];

		m_VertexCount += 4;
		m_IndexCount += 6;
		m_VertexBufferOffset += sizeof(vertices) / sizeof(float);
	}

	void Renderer2D::RenderQuad(lptm::Vector2D size, lptm::Vector3D position, const se::Texture& texture, const se::Shader* shader)
	{
		float aspectRatio = m_Window.GetAspectRatio();
		int textureUnit = texture.GetTextureID() - 1;
		
		float vertices[] = {
			-size.x + position.x,  size.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		0.0f, 0.0f,		(float)textureUnit,
			 size.x + position.x,  size.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		1.0f, 0.0f,		(float)textureUnit,
			 size.x + position.x, -size.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		1.0f, 1.0f,		(float)textureUnit,
			-size.x + position.x, -size.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		0.0f, 1.0f,		(float)textureUnit
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

	void Renderer2D::RenderQuad(lptm::Vector2D size, lptm::Vector3D position, const se::Texture& texture, const se::Shader* shader, lptm::Vector2D spriteCoord, lptm::Vector2D spriteSize)
	{
		float aspectRatio = m_Window.GetAspectRatio();
		int textureUnit = texture.GetTextureID() - 1;

		lptm::Vector2D spriteUV[4];
		spriteUV[0] = { (spriteCoord.x * spriteSize.x) / texture.GetTextureSize().x, (spriteCoord.y * spriteSize.y) / texture.GetTextureSize().y };				// Bottom left
		spriteUV[1] = { ((spriteCoord.x + 1) * spriteSize.x) / texture.GetTextureSize().x, (spriteCoord.y * spriteSize.y) / texture.GetTextureSize().y };		// Bottom right
		spriteUV[2] = { ((spriteCoord.x + 1) * spriteSize.x) / texture.GetTextureSize().x, ((spriteCoord.y + 1) * spriteSize.y) / texture.GetTextureSize().y }; // Top right
		spriteUV[3] = { (spriteCoord.x * spriteSize.x) / texture.GetTextureSize().x, ((spriteCoord.y + 1) * spriteSize.y) / texture.GetTextureSize().y };		// Top left

		float vertices[] = {
			-size.x + position.x, -size.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		spriteUV[3].x, spriteUV[3].y,		(float)textureUnit,
			 size.x + position.x, -size.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		spriteUV[2].x, spriteUV[2].y,		(float)textureUnit,
			 size.x + position.x,  size.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		spriteUV[1].x, spriteUV[1].y,		(float)textureUnit,
			-size.x + position.x,  size.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		spriteUV[0].x, spriteUV[0].y,		(float)textureUnit
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

	void Renderer2D::RenderRotatedQuad(lptm::Vector2D size, lptm::Vector3D position, lptm::Vector4D color, float rotation)
	{
		float aspectRatio = m_Window.GetAspectRatio();

		lptm::Vector2D topLeft		= lptm::Vector2D(-size.x,  size.y).Rotate(-rotation);
		lptm::Vector2D topRight		= lptm::Vector2D( size.x,  size.y).Rotate(-rotation);
		lptm::Vector2D bottomRight	= lptm::Vector2D( size.x, -size.y).Rotate(-rotation);
		lptm::Vector2D bottomLeft	= lptm::Vector2D(-size.x, -size.y).Rotate(-rotation);
	
		float vertices[] = {
			topLeft.x	  + position.x, topLeft.y     * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			topRight.x	  + position.x, topRight.y    * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			bottomRight.x + position.x,	bottomRight.y * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			bottomLeft.x  + position.x,	bottomLeft.y  * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f
		};

		for (uint32_t i = 0; i < sizeof(vertices) / sizeof(float); i++)
			m_VertexBufferData[m_VertexBufferOffset + i] = vertices[i];

		m_VertexCount += 4;
		m_IndexCount += 6;

		m_VertexBufferOffset += sizeof(vertices) / sizeof(float);
	}

	void Renderer2D::RenderRotatedQuad(lptm::Vector2D size, lptm::Vector3D position, const se::Texture& texture, const se::Shader* shader, float rotation)
	{
		float aspectRatio = m_Window.GetAspectRatio();
		int textureUnit = texture.GetTextureID() - 1;
		
		lptm::Vector2D topLeft		= lptm::Vector2D(-size.x,  size.y).Rotate(-rotation);
		lptm::Vector2D topRight		= lptm::Vector2D( size.x,  size.y).Rotate(-rotation);
		lptm::Vector2D bottomRight	= lptm::Vector2D( size.x, -size.y).Rotate(-rotation);
		lptm::Vector2D bottomLeft	= lptm::Vector2D(-size.x, -size.y).Rotate(-rotation);

		float vertices[] = {
			topLeft.x	  + position.x, topLeft.y     * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		0.0f, 0.0f,		(float)textureUnit,
			topRight.x	  + position.x, topRight.y    * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		1.0f, 0.0f,		(float)textureUnit,
			bottomRight.x + position.x,	bottomRight.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		1.0f, 1.0f,		(float)textureUnit,
			bottomLeft.x  + position.x,	bottomLeft.y  * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		0.0f, 1.0f,		(float)textureUnit
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

	void Renderer2D::RenderRotatedQuad(lptm::Vector2D size, lptm::Vector3D position, const se::Texture& texture, const se::Shader* shader, lptm::Vector2D spriteCoord, lptm::Vector2D spriteSize, float rotation)
	{
		float aspectRatio = m_Window.GetAspectRatio();
		int textureUnit = texture.GetTextureID() - 1;

		lptm::Vector2D topLeft		= lptm::Vector2D(-size.x,  size.y).Rotate(-rotation);
		lptm::Vector2D topRight		= lptm::Vector2D( size.x,  size.y).Rotate(-rotation);
		lptm::Vector2D bottomRight	= lptm::Vector2D( size.x, -size.y).Rotate(-rotation);
		lptm::Vector2D bottomLeft	= lptm::Vector2D(-size.x, -size.y).Rotate(-rotation);

		lptm::Vector2D spriteUV[4];
		spriteUV[0] = { (spriteCoord.x * spriteSize.x) / texture.GetTextureSize().x, (spriteCoord.y * spriteSize.y) / texture.GetTextureSize().y };				// Bottom left
		spriteUV[1] = { ((spriteCoord.x + 1) * spriteSize.x) / texture.GetTextureSize().x, (spriteCoord.y * spriteSize.y) / texture.GetTextureSize().y };		// Bottom right
		spriteUV[2] = { ((spriteCoord.x + 1) * spriteSize.x) / texture.GetTextureSize().x, ((spriteCoord.y + 1) * spriteSize.y) / texture.GetTextureSize().y }; // Top right
		spriteUV[3] = { (spriteCoord.x * spriteSize.x) / texture.GetTextureSize().x, ((spriteCoord.y + 1) * spriteSize.y) / texture.GetTextureSize().y };		// Top left

		float vertices[] = {
			topLeft.x	  + position.x, topLeft.y     * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		spriteUV[3].x, spriteUV[3].y,		(float)textureUnit,
			topRight.x    + position.x, topRight.y    * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		spriteUV[2].x, spriteUV[2].y,		(float)textureUnit,
			bottomRight.x + position.x,	bottomRight.y * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		spriteUV[1].x, spriteUV[1].y,		(float)textureUnit,
			bottomLeft.x  + position.x,	bottomLeft.y  * aspectRatio + position.y, position.z,		0.0f, 0.0f, 0.0f, 0.0f,		spriteUV[0].x, spriteUV[0].y,		(float)textureUnit
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
		if(m_Shader != nullptr)
			glUniform1iv(glGetUniformLocation(m_Shader->GetProgramID(), "u_Textures"), Renderer2DSpecification::MaxTextureUnits, m_TextureUnits);

		m_VAO->Bind();
		m_VBO->Bind();
		m_EBO->Bind();

		m_VAO->AddBuffer(3, sizeof(BufferDisposition) / sizeof(float), offsetof(BufferDisposition, position), 0);
		m_VAO->AddBuffer(4, sizeof(BufferDisposition) / sizeof(float), offsetof(BufferDisposition, color), 1);
		m_VAO->AddBuffer(2, sizeof(BufferDisposition) / sizeof(float), offsetof(BufferDisposition, texCoord), 2);
		m_VAO->AddBuffer(1, sizeof(BufferDisposition) / sizeof(float), offsetof(BufferDisposition, texIndex), 3);

		glBufferSubData(GL_ARRAY_BUFFER, 0, m_VertexCount * sizeof(BufferDisposition), m_VertexBufferData);
		glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);

		m_VBO->UnBind();
		m_VAO->UnBind();
		m_EBO->UnBind();

		m_IndexCount = 0;
		m_VertexCount = 0;
		m_VertexBufferOffset = 0;
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
