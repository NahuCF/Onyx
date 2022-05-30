#include "Game.h"

Game::Game()
{
	m_Window = new se::Window();
	m_BaseShader = new se::Shader("Assets/Shaders/Shader.vs", "Assets/Shaders/Shader.fs");
	m_Renderer = new se::Renderer2D(*m_Window);
}

Game::~Game()
{
	delete m_Window;
	delete m_BaseShader;
	delete m_Renderer;
}

void Game::Update()
{
	while(!m_Window->ShouldClose())
	{
		m_Window->Clear();

		m_BaseShader->Bind();
		m_Player.Render(m_Renderer);

		m_Renderer->Flush();
		m_Window->Update();
	}
}