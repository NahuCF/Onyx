#include "pch.h"

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "Buffers.h"

namespace se {

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

	IndexBuffer::IndexBuffer(uint32_t* index, uint32_t count)
		: m_Count(count)
	{
		glGenBuffers(1, &m_BufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), index, GL_STATIC_DRAW);
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
		Bind();
		buffer->Bind();
		
		glEnableVertexAttribArray(attribIndex);
		glVertexAttribPointer(attribIndex, buffer->GetComponentCount(), GL_FLOAT, GL_FALSE, 0, 0); // sizeof(float) * buffer->GetComponentCount()
		
		buffer->UnBind();
		UnBind();
	}

}