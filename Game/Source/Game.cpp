#include "Game.h"

Game::Game()
{
	m_Window = new se::Window();
	m_BaseShader = new se::Shader("Assets/Shaders/Shader.vs", "Assets/Shaders/Shader.fs");
	m_Renderer = new se::Renderer2D(*m_Window);

	m_ParticleSystem = se::ParticleSystem();

	m_ParticleProps.position = { 0.0f, 0.0f };
	m_ParticleProps.lifetime = 1.5f;
	m_ParticleProps.rotation = 45;

	m_ParticleProps.sizeBegin = 0.04f;
	m_ParticleProps.sizeVariation = 0.5f;

	m_ParticleProps.velocity = { 0.0f, 0.0f };
	m_ParticleProps.velocityVariation = { 0.6f, 0.4f };

	m_ParticleProps.colorBegin = { 1.0f, 0.4f, 0.4f, 1.0f };
	m_ParticleProps.colorEnd = { 0.0f, 0.2f, 0.1f, 1.0f };

	m_Window->SetVSync(1);
	m_Window->SetWindowColor({ 0.1f, 0.1f, 0.1f, 1.0f });
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

		//m_Player.Render(m_Renderer);
		if (m_Window->IsButtomPressed(GLFW_MOUSE_BUTTON_1))
		{
			m_ParticleProps.position = { m_Window->GetMousePos().x, m_Window->GetMousePos().y};
			m_ParticleSystem.Emit(m_ParticleProps);
		}

		m_ParticleSystem.Update(m_Window->GetSeconds());
		m_ParticleSystem.Render(m_Renderer);

		m_Renderer->Flush();
		m_Window->Update();
	}
}