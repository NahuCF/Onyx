#pragma once

#include "pch.h"
#include "Renderable2D.h"
#include "Maths/Maths.h"

namespace se {

	Renderable2D::Renderable2D(lptm::Vector2D size, lptm::Vector4D color)
	{
		float vertices[]
		{
			0.0f,   0.0f,   0.0f,
			size.x, 0.0f,   0.0f,
			size.x, size.y, 0.0f,
			0.0f,   size.y, 0.0f
		};
		
		float colors[]
		{
			color.x, color.y, color.z, color.w,
			color.x, color.y, color.z, color.w,
			color.x, color.y, color.z, color.w,
			color.x, color.y, color.z, color.w
		};

		uint32_t indices[]
		{
			0, 1, 2,
			2, 3, 0
		};

		m_VAO = new se::VertexArray();
		m_VAO->AddBuffer(new VertexBuffer(vertices, 4 * 3, 3), 0);
		m_VAO->AddBuffer(new VertexBuffer(colors, 4 * 4, 4), 1);
		m_IBO = new se::IndexBuffer(indices, 6);
	}

	Renderable2D::~Renderable2D()
	{
		delete m_IBO;
		delete m_VAO;
	}

}