#include "pch.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

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

	VertexBuffer::VertexBuffer(const void* data, uint32_t sizeBytes)
		: m_BufferCount(0)
	{
		glGenBuffers(1, &m_BufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeBytes, data, GL_STATIC_DRAW);
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

	void VertexBuffer::SetData(const void* data, uint32_t sizeBytes)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeBytes, data, GL_STATIC_DRAW);
	}

	void VertexBuffer::SetSubData(const void* data, uint32_t offset, uint32_t sizeBytes)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
		glBufferSubData(GL_ARRAY_BUFFER, offset, sizeBytes, data);
	}

	// --------------------------------------------
	// --------------------------------------------

	IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t sizeBytes)
		: m_Count(sizeBytes / sizeof(uint32_t))
	{
		glGenBuffers(1, &m_BufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeBytes, indices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	IndexBuffer::IndexBuffer(const void* data, uint32_t sizeBytes)
		: m_Count(sizeBytes / sizeof(uint32_t))
	{
		glGenBuffers(1, &m_BufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeBytes, data, GL_STATIC_DRAW);
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
		glVertexAttribPointer(attribIndex, attribCount, GL_FLOAT, GL_FALSE, stride * sizeof(float), (const void*)(uintptr_t)offset);
	}

	void VertexArray::SetVertexBuffer(VertexBuffer* vbo)
	{
		Bind();
		vbo->Bind();
	}

	void VertexArray::SetIndexBuffer(IndexBuffer* ebo)
	{
		m_IndexBuffer = ebo;
		Bind();
		ebo->Bind();
	}

	void VertexArray::SetLayout(const VertexLayout& layout)
	{
		Bind();

		const auto& attributes = layout.GetAttributes();
		for (uint32_t index = 0; index < attributes.size(); index++) {
			const auto& attr = attributes[index];

			glEnableVertexAttribArray(index);

			switch (attr.type) {
				case VertexAttributeType::Float:
				case VertexAttributeType::Float2:
				case VertexAttributeType::Float3:
				case VertexAttributeType::Float4:
					glVertexAttribPointer(
						index,
						GetAttributeComponentCount(attr.type),
						GL_FLOAT,
						attr.normalized ? GL_TRUE : GL_FALSE,
						layout.GetStride(),
						(const void*)(uintptr_t)attr.offset
					);
					break;

				case VertexAttributeType::Int:
				case VertexAttributeType::Int2:
				case VertexAttributeType::Int3:
				case VertexAttributeType::Int4:
					// Use glVertexAttribIPointer for integer attributes (bone IDs, etc.)
					glVertexAttribIPointer(
						index,
						GetAttributeComponentCount(attr.type),
						GL_INT,
						layout.GetStride(),
						(const void*)(uintptr_t)attr.offset
					);
					break;

				case VertexAttributeType::UInt:
					glVertexAttribIPointer(
						index,
						GetAttributeComponentCount(attr.type),
						GL_UNSIGNED_INT,
						layout.GetStride(),
						(const void*)(uintptr_t)attr.offset
					);
					break;

				case VertexAttributeType::Bool:
					glVertexAttribIPointer(
						index,
						1,
						GL_INT,
						layout.GetStride(),
						(const void*)(uintptr_t)attr.offset
					);
					break;
			}
		}

		UnBind();
	}

}