#include "pch.h"

#include "Vendor/GLEW/include/GL/glew.h"
#include "Vendor/GLFW/glfw3.h"

#include "Buffers.h"

namespace Onyx {

	VertexBuffer::VertexBuffer(uint32_t size)
	{
		glGenBuffers(1, &m_BufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	VertexBuffer::VertexBuffer(float* vertices, uint32_t count, uint32_t componentCount)
		: m_BufferCount(componentCount)
	{
		glGenBuffers(1, &m_BufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
		glBufferData(GL_ARRAY_BUFFER, count * sizeof(float), vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	VertexBuffer::~VertexBuffer()
	{
		glDeleteBuffers(1, &m_BufferID);
	}

	void VertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
	}

	void VertexBuffer::UnBind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// --------------------------------------------
	// --------------------------------------------

	IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t size)
	{
		glGenBuffers(1, &m_BufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	IndexBuffer::~IndexBuffer()
	{
		glDeleteBuffers(1, &m_BufferID);
	}

	void IndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferID);
	}

	void IndexBuffer::UnBind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	// --------------------------------------------
	// --------------------------------------------

	VertexArray::VertexArray()
	{
		glGenVertexArrays(1, &m_BufferID);
	}

	VertexArray::~VertexArray()
	{
		for(uint32_t i = 0; i < m_Buffers.size(); i++)
			delete m_Buffers[i];

		glDeleteVertexArrays(1, &m_BufferID);
	}

	void VertexArray::Bind() const
	{
		glBindVertexArray(m_BufferID);
	}

	void VertexArray::UnBind() const
	{
		glBindVertexArray(0);
	}

	void VertexArray::AddBuffer(VertexBuffer* buffer, uint32_t attribIndex)
	{
		m_Buffers.push_back(buffer);

		Bind();
		buffer->Bind();
		
		glEnableVertexAttribArray(attribIndex);
		glVertexAttribPointer(attribIndex, buffer->GetComponentCount(), GL_FLOAT, GL_FALSE, 0, 0);
		
		buffer->UnBind();
		UnBind();
	}

	void VertexArray::AddBuffer(uint32_t attribCount, uint32_t stride, uint32_t offset, uint32_t attribIndex)
	{
		glEnableVertexAttribArray(attribIndex);
		glVertexAttribPointer(attribIndex, attribCount, GL_FLOAT, GL_FALSE, stride * sizeof(float), (const void*)offset);
	}

}