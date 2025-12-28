#include "pch.h"
#include "Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Onyx {

	Renderer2D::Renderer2D(Window& window, std::shared_ptr<Camera> camera)
		: m_VAO(new Onyx::VertexArray)
		, m_VBO(new Onyx::VertexBuffer(Renderer2DSpecification::VertexBufferSize))
		, m_Window(window)
        , m_Camera(camera)
	{
		m_DefaultShader = std::make_unique<Onyx::Shader>(
			"assets/shaders/basic.vert", 
			"assets/shaders/basic.frag"
		);

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

	void Renderer2D::RenderQuad(Onyx::Vector2D size, Onyx::Vector3 position, Onyx::Vector4D color)
	{
		float aspectRatio = m_Window.GetAspectRatio();

		float vertices[] = {
			-size.x + position.x*2,  size.y * aspectRatio + position.y*2, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			 size.x + position.x*2,  size.y * aspectRatio + position.y*2, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			 size.x + position.x*2, -size.y * aspectRatio + position.y*2, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			-size.x + position.x*2, -size.y * aspectRatio + position.y*2, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f
		};
	
		for(uint32_t i = 0; i < sizeof(vertices) / sizeof(float); i++)
			m_VertexBufferData[m_VertexBufferOffset + i] = vertices[i];

		m_VertexCount += 4;
		m_IndexCount += 6;
		m_VertexBufferOffset += sizeof(vertices) / sizeof(float);
		
		// Use default shader for colored quads
		m_Shader = m_DefaultShader.get();
	}

	void Renderer2D::RenderQuad(Onyx::Vector2D size, Onyx::Vector3 position, const Onyx::Texture& texture, const Onyx::Shader* shader)
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

	void Renderer2D::RenderQuad(Onyx::Vector2D size, Onyx::Vector3 position, const Onyx::Texture* texture, const Onyx::Shader* shader, Onyx::Vector2D spriteCoord, Onyx::Vector2D spriteSize)
	{
		float aspectRatio = m_Window.GetAspectRatio();
		int textureUnit = texture->GetTextureID() - 1;

		Onyx::Vector2D spriteUV[4];
		spriteUV[0] = { (spriteCoord.x * spriteSize.x) / texture->GetTextureSize().x, (spriteCoord.y * spriteSize.y) / texture->GetTextureSize().y };				// Bottom left
		spriteUV[1] = { ((spriteCoord.x + 1) * spriteSize.x) / texture->GetTextureSize().x, (spriteCoord.y * spriteSize.y) / texture->GetTextureSize().y };		// Bottom right
		spriteUV[2] = { ((spriteCoord.x + 1) * spriteSize.x) / texture->GetTextureSize().x, ((spriteCoord.y + 1) * spriteSize.y) / texture->GetTextureSize().y }; // Top right
		spriteUV[3] = { (spriteCoord.x * spriteSize.x) / texture->GetTextureSize().x, ((spriteCoord.y + 1) * spriteSize.y) / texture->GetTextureSize().y };		// Top left
	
		float vertices[] = {
			-size.x + position.x * 2, -size.y + position.y * 2, position.z,		1.0f, 1.0f, 1.0f, 1.0f,		spriteUV[0].x, spriteUV[0].y,		(float)textureUnit,
			 size.x + position.x * 2, -size.y + position.y * 2, position.z,		1.0f, 1.0f, 1.0f, 1.0f,		spriteUV[1].x, spriteUV[1].y,		(float)textureUnit,
			 size.x + position.x * 2,  size.y + position.y * 2, position.z,		1.0f, 1.0f, 1.0f, 1.0f,		spriteUV[2].x, spriteUV[2].y,		(float)textureUnit,
			-size.x + position.x * 2,  size.y + position.y * 2, position.z,		1.0f, 1.0f, 1.0f, 1.0f,		spriteUV[3].x, spriteUV[3].y,		(float)textureUnit
		};

		for (uint32_t i = 0; i < sizeof(vertices) / sizeof(float); i++)
			m_VertexBufferData[m_VertexBufferOffset + i] = vertices[i];

		m_VertexCount += 4;
		m_IndexCount += 6;
		m_VertexBufferOffset += sizeof(vertices) / sizeof(float);

		m_Shader = shader;
		m_TextureUnits[textureUnit] = textureUnit;
		glBindTextureUnit(textureUnit, texture->GetTextureID());
	}

    void Renderer2D::RenderRotatedLine(Onyx::Vector2D start, Onyx::Vector2D end, float width, Onyx::Vector4D color, float rotation)
    {
        float length = Onyx::VectorModule(end - start);
        Onyx::Vector2D center = Onyx::Vector2D(end.x - start.x, end.y - end.y) / 2;

        RenderRotatedQuad({length, width}, {start.x + center.x, start.y, 0.0f}, color, rotation);
    }

	void Renderer2D::RenderCircle(float radius, int subdivision, Onyx::Vector3 position, Onyx::Vector4D color)
	{
		float aspectRatio = m_Window.GetAspectRatio();

		float angle = 0;

		for (int i = 0; i < 4; i++)
		{
			radius = 1;
			subdivision = 3;


			/*float vertices[] = {
				0.0f	+ position.x,  0.0f * aspectRatio	+ position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
				radius	+ position.x,  height * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
				radius + position.x,  height* aspectRatio + position.y, position.z,			color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			};*/

			float point1Angle = (90 / subdivision) * 2;
			Onyx::Vector2D point1 = Onyx::Vector2D(
				std::cos(point1Angle) * radius,
				std::sin(point1Angle) * radius
			).Rotate(-angle);

			float point2Angle = (90 / subdivision);
			Onyx::Vector2D point2 = Onyx::Vector2D(
				std::sin(point2Angle) * radius,
				std::cos(point2Angle) * radius
			).Rotate(-angle);

			float point3Angle = (90 / subdivision);
			Onyx::Vector2D point3 = Onyx::Vector2D(
				radius / std::cos(point3Angle),
				std::tan(point3Angle) * radius
			).Rotate(-angle);


			float vertices[] = {
				0.0f + position.x, 0.0f * aspectRatio + position.y, position.z,				0.2,  0.2f, 0.2f, 0.2f, 1.0f,			0.0f, 0.0f,		-1.0f,
				point1.x + position.x, point1.y * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		1.0f, 0.0f,		-1.0f,
				point2.x + position.x, point2.y * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		1.0f, 1.0f,		-1.0f,
				point3.x + position.x, point3.y * aspectRatio + position.y, position.z,		color.x, color.y, color.z, color.w,		0.0f, 1.0f,		-1.0f
			};

			//angle += 90 / subdivision;

			for (uint32_t i = 0; i < sizeof(vertices) / sizeof(float); i++)
				m_VertexBufferData[m_VertexBufferOffset + i] = vertices[i];

			m_VertexCount += 4;
			m_IndexCount += 6;
			m_VertexBufferOffset += sizeof(vertices) / sizeof(float);
		}
	}

	void Renderer2D::RenderRotatedQuad(Onyx::Vector2D size, Onyx::Vector3 position, Onyx::Vector4D color, float rotation)
	{
		float aspectRatio = m_Window.GetAspectRatio();

		Onyx::Vector2D topLeft		= Onyx::Vector2D(-size.x,  size.y).Rotate(-rotation);
		Onyx::Vector2D topRight		= Onyx::Vector2D( size.x,  size.y).Rotate(-rotation);
		Onyx::Vector2D bottomRight	= Onyx::Vector2D( size.x, -size.y).Rotate(-rotation);
		Onyx::Vector2D bottomLeft	= Onyx::Vector2D(-size.x, -size.y).Rotate(-rotation);
	
		float vertices[] = {
			topLeft.x	  + position.x * 2, topLeft.y* aspectRatio + position.y * 2, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			topRight.x	  + position.x * 2, topRight.y* aspectRatio + position.y * 2, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			bottomRight.x + position.x * 2,	bottomRight.y* aspectRatio + position.y * 2, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f,
			bottomLeft.x  + position.x * 2,	bottomLeft.y* aspectRatio + position.y * 2, position.z,		color.x, color.y, color.z, color.w,		0.0f, 0.0f,		-1.0f
		};

		for (uint32_t i = 0; i < sizeof(vertices) / sizeof(float); i++)
			m_VertexBufferData[m_VertexBufferOffset + i] = vertices[i];

		m_VertexCount += 4;
		m_IndexCount += 6;

		m_VertexBufferOffset += sizeof(vertices) / sizeof(float);
		
		m_Shader = m_DefaultShader.get();
	}

	void Renderer2D::RenderRotatedQuad(Onyx::Vector2D size, Onyx::Vector3 position, const Onyx::Texture& texture, const Onyx::Shader* shader, float rotation)
	{
		float aspectRatio = m_Window.GetAspectRatio();
		int textureUnit = texture.GetTextureID() - 1;
		
		Onyx::Vector2D topLeft		= Onyx::Vector2D(-size.x,  size.y).Rotate(-rotation);
		Onyx::Vector2D topRight		= Onyx::Vector2D( size.x,  size.y).Rotate(-rotation);
		Onyx::Vector2D bottomRight	= Onyx::Vector2D( size.x, -size.y).Rotate(-rotation);
		Onyx::Vector2D bottomLeft	= Onyx::Vector2D(-size.x, -size.y).Rotate(-rotation);

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

	void Renderer2D::RenderRotatedQuad(Onyx::Vector2D size, Onyx::Vector3 position, const Onyx::Texture& texture, const Onyx::Shader* shader, Onyx::Vector2D spriteCoord, Onyx::Vector2D spriteSize, float rotation)
	{
		float aspectRatio = m_Window.GetAspectRatio();
		int textureUnit = texture.GetTextureID() - 1;

		Onyx::Vector2D topLeft		= Onyx::Vector2D(-size.x,  size.y).Rotate(-rotation);
		Onyx::Vector2D topRight		= Onyx::Vector2D( size.x,  size.y).Rotate(-rotation);
		Onyx::Vector2D bottomRight	= Onyx::Vector2D( size.x, -size.y).Rotate(-rotation);
		Onyx::Vector2D bottomLeft	= Onyx::Vector2D(-size.x, -size.y).Rotate(-rotation);

		Onyx::Vector2D spriteUV[4];
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
		{
			m_Shader->Bind();
			
			// Set up orthographic projection matrix for 2D rendering
			auto camera = m_Camera.lock();
			if (camera)
			{
				float aspectRatio = m_Window.GetAspectRatio();
				glm::mat4 projection = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 100.0f);
				glm::mat4 view = camera->GetViewMatrix();
				glm::mat4 viewProjection = projection * view;
				
				glUniformMatrix4fv(glGetUniformLocation(m_Shader->GetProgramID(), "u_ViewProjection"), 1, GL_FALSE, glm::value_ptr(viewProjection));
			}
			
			glUniform1iv(glGetUniformLocation(m_Shader->GetProgramID(), "u_Textures"), Renderer2DSpecification::MaxTextureUnits, m_TextureUnits);
		}
		
		m_VAO->Bind();
		m_VBO->Bind();
		m_EBO->Bind();

		m_VAO->AddBuffer(3, sizeof(BufferDisposition) / sizeof(float), offsetof(BufferDisposition, position), 0);
		m_VAO->AddBuffer(4, sizeof(BufferDisposition) / sizeof(float), offsetof(BufferDisposition, color), 1);
		m_VAO->AddBuffer(2, sizeof(BufferDisposition) / sizeof(float), offsetof(BufferDisposition, texCoord), 2);
		m_VAO->AddBuffer(1, sizeof(BufferDisposition) / sizeof(float), offsetof(BufferDisposition, texIndex), 3);

		glBufferSubData(GL_ARRAY_BUFFER, 0, m_VertexCount * sizeof(BufferDisposition), m_VertexBufferData);
		glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);

		if(m_Shader != nullptr)
			m_Shader->UnBind();
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
