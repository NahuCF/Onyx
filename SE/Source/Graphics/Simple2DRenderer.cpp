#include "pch.h"

#include <deque>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "Renderable2D.h"
#include "Simple2DRenderer.h" 

namespace se {

	void Simple2DRenderer::Submit(Renderable2D* renderable)
	{
		m_Renderable.push_back(renderable);
	}

	void Simple2DRenderer::Flush() 
	{
		while(!m_Renderable.empty())
		{
			Renderable2D* sprite = m_Renderable.front();

			sprite->GetVAO()->Bind();
			sprite->GetIBO()->Bind();
			
			glDrawElements(GL_TRIANGLES, sprite->GetIBO()->GetCount(), GL_UNSIGNED_INT, 0);

			sprite->GetIBO()->UnBind();
			sprite->GetVAO()->UnBind();

			m_Renderable.pop_front();
		}
	}
}