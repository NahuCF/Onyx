#pragma once

#include <vector>
#include "VertexLayout.h"

namespace Onyx {

	class VertexBuffer
	{
	public:
		VertexBuffer(uint32_t size);
		VertexBuffer(float* vertices, uint32_t count, uint32_t componentCount);
		VertexBuffer(const void* data, uint32_t sizeBytes);

		~VertexBuffer();

		void Bind() const;
		void UnBind() const;

		void SetData(const void* data, uint32_t sizeBytes);
		void SetSubData(const void* data, uint32_t offset, uint32_t sizeBytes);

		uint32_t GetComponentCount() const { return m_BufferCount; }
		uint32_t GetBufferID() const { return m_BufferID; }

	private:
		uint32_t m_BufferID = 0;
		uint32_t m_BufferCount = 0;
	};

	class IndexBuffer
	{
	public:
		IndexBuffer(uint32_t sizeBytes);  // Pre-allocate with no data
		IndexBuffer(uint32_t* indices, uint32_t sizeBytes);
		IndexBuffer(const void* data, uint32_t sizeBytes);

		~IndexBuffer();

		void Bind() const;
		void UnBind() const;

		void SetData(const void* data, uint32_t sizeBytes);
		void SetSubData(const void* data, uint32_t offset, uint32_t sizeBytes);
		void SetCount(uint32_t count) { m_Count = count; }

		uint32_t GetCount() const { return m_Count; }
		uint32_t GetBufferID() const { return m_BufferID; }

	private:
		uint32_t m_BufferID = 0;
		uint32_t m_Count = 0;
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

		void SetVertexBuffer(VertexBuffer* vbo);
		void SetIndexBuffer(IndexBuffer* ebo);
		void SetLayout(const VertexLayout& layout);

		IndexBuffer* GetIndexBuffer() const { return m_IndexBuffer; }
		uint32_t GetBufferID() const { return m_BufferID; }

		std::vector<VertexBuffer*> m_Buffers;
		VertexBuffer* m_BatchBuffer = nullptr;

	private:
		uint32_t m_BufferID = 0;
		IndexBuffer* m_IndexBuffer = nullptr;
	};

	class ShaderStorageBuffer
	{
	public:
		ShaderStorageBuffer();
		~ShaderStorageBuffer();

		void Bind() const;
		void UnBind() const;
		void BindBase(uint32_t slot) const;

		// Grow-or-update: reallocates if sizeBytes > current capacity, else SubData.
		// Also binds to the given binding point.
		void Upload(const void* data, size_t sizeBytes, uint32_t bindingPoint);

		// Allocate GPU memory without uploading data (for GPU-written output buffers).
		// Also binds to the given binding point.
		void Allocate(size_t sizeBytes, uint32_t bindingPoint);

		// Clear the entire buffer to a uint value (e.g., 0 for resetting atomic counters).
		void ClearUint(uint32_t bindingPoint, uint32_t value);

		uint32_t GetBufferID() const { return m_BufferID; }

	private:
		uint32_t m_BufferID = 0;
		size_t m_AllocatedSize = 0;
	};

	class DrawCommandBuffer
	{
	public:
		DrawCommandBuffer();
		~DrawCommandBuffer();

		void Bind() const;
		void UnBind() const;

		// Grow-or-update: reallocates if sizeBytes > current capacity, else SubData.
		void Upload(const void* data, size_t sizeBytes);

		uint32_t GetBufferID() const { return m_BufferID; }

	private:
		uint32_t m_BufferID = 0;
		size_t m_AllocatedSize = 0;
	};

}