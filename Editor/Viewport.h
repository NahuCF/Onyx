#pragma once

#include <SE.h>

class Viewport
{
public:
	Viewport();
	~Viewport();

	void Loop(ImGuiIO& io);
	ImGuiIO& Init(se::Window* window);
private:
	se::Window* m_Window;
	se::OrtogonalTilemap* map;
	
	std::vector<se::Shader*> m_Shaders;
};