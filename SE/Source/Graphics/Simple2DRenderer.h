#pragma once

#include <deque>

#include "Renderable2D.h"

namespace se {

	class Simple2DRenderer
	{
	public:
		void Submit(Renderable2D* renderable);
		void Flush();
	private:
		std::deque<Renderable2D*> m_Renderable;
	};

}