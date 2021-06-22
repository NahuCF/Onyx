#pragma once

#include "Buffers.h"
#include "Maths/Maths.h"

namespace se {

	class Renderable2D
	{
	public:
		Renderable2D(lptm::Vector2D size, lptm::Vector4D color);
		~Renderable2D();

		se::VertexArray* GetVAO() const { return m_VAO; }
		se::IndexBuffer* GetIBO() const { return m_IBO; }
	private:
		se::VertexArray* m_VAO;
		se::IndexBuffer* m_IBO;
	};

}