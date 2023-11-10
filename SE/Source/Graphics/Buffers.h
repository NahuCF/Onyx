#pragma once

#include <vector>

namespace Onyx {

	class VertexBuffer
	{
	public:
		VertexBuffer(uint32_t size);
		VertexBuffer(float* vertices, uint32_t count, uint32_t componentCount);

		~VertexBuffer();

		void Bind() const;
		void UnBind() const;

		uint32_t GetComponentCount() const { return m_BufferCount; }
	private:
		uint32_t m_BufferID;
		uint32_t m_BufferCount;
	};

	class IndexBuffer
	{
	public:
		IndexBuffer(uint32_t* indices, uint32_t size);
		~IndexBuffer();

		void Bind() const;
		void UnBind() const;

		uint32_t GetCount() const { return m_Count; }
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

		void AddBuffer(VertexBuffer* buffer, uint32_t attribIndex);
		void AddBuffer(uint32_t attribCount, uint32_t stride, uint32_t offset, uint32_t attribIndex);

		std::vector<VertexBuffer*> m_Buffers;
		VertexBuffer* m_BatchBuffer;
	private:
		uint32_t m_BufferID;
	};

}