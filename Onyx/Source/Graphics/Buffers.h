#pragma once

#include <vector>
#include "VertexLayout.h"

namespace Onyx {

	class VertexBuffer
	{
	public:
		// Dynamic buffer (for batch rendering)
		VertexBuffer(uint32_t size);

		// Static buffer with float data (legacy)
		VertexBuffer(float* vertices, uint32_t count, uint32_t componentCount);

		// Static buffer with raw data (new)
		VertexBuffer(const void* data, uint32_t sizeBytes);

		~VertexBuffer();

		void Bind() const;
		void UnBind() const;

		// Update buffer data
		void SetData(const void* data, uint32_t sizeBytes);
		void SetSubData(const void* data, uint32_t offset, uint32_t sizeBytes);

		uint32_t GetComponentCount() const { return m_BufferCount; }
		uint32_t GetBufferID() const { return m_BufferID; }

	private:
		uint32_t m_BufferID;
		uint32_t m_BufferCount;
	};

	class IndexBuffer
	{
	public:
		// Create with index data (size in bytes)
		IndexBuffer(uint32_t* indices, uint32_t sizeBytes);

		// Create with raw data
		IndexBuffer(const void* data, uint32_t sizeBytes);

		~IndexBuffer();

		void Bind() const;
		void UnBind() const;

		uint32_t GetCount() const { return m_Count; }
		uint32_t GetBufferID() const { return m_BufferID; }

	private:
		uint32_t m_BufferID;
		uint32_t m_Count;
	};

	class VertexArray
	{
	public:
		VertexArray();
		~VertexArray();

		void Bind() const;
		void UnBind() const;

		// Legacy methods
		void AddBuffer(VertexBuffer* buffer, uint32_t attribIndex);
		void AddBuffer(uint32_t attribCount, uint32_t stride, uint32_t offset, uint32_t attribIndex);

		// New layout-based methods
		void SetVertexBuffer(VertexBuffer* vbo);
		void SetIndexBuffer(IndexBuffer* ebo);
		void SetLayout(const VertexLayout& layout);

		IndexBuffer* GetIndexBuffer() const { return m_IndexBuffer; }
		uint32_t GetBufferID() const { return m_BufferID; }

		std::vector<VertexBuffer*> m_Buffers;
		VertexBuffer* m_BatchBuffer;

	private:
		uint32_t m_BufferID;
		IndexBuffer* m_IndexBuffer = nullptr;
	};

}