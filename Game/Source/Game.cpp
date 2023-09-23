#include "Game.h"
#include <cmath>

Game::Game()
{
	m_WorldSize = { 200, 200 };
	m_Origin = { 6, -16 };
	m_World = new int[m_WorldSize.x * m_WorldSize.y]{ 0 };

	m_Window = new se::Window("asd", 1000, 1000, 1);
	m_BaseShader = new se::Shader("Assets/Shaders/Shader.vs", "Assets/Shaders/Shader.fs");
	m_TextureShader = new se::Shader("Assets/Shaders/TextureShader.vs", "Assets/Shaders/TextureShader.fs");
	m_TilesTexture = new se::Texture("Assets/Textures/isometric_demo.png");
	m_Renderer = new se::Renderer2D(*m_Window);

	m_ParticleSystem = se::ParticleSystem();

	m_ParticleProps.position = { 0.0f, 0.0f };
	m_ParticleProps.lifetime = 1.5f;
	m_ParticleProps.rotation = 45;

	m_ParticleProps.sizeBegin = 0.04f;
	m_ParticleProps.sizeVariation = 0.5f;

	m_ParticleProps.velocity = { 0.0f, 0.0f };
	m_ParticleProps.velocityVariation = { 1.6f, 0.9f };

	m_ParticleProps.colorBegin = { 1.0f, 0.4f, 0.4f, 1.0f };
	m_ParticleProps.colorEnd = { 0.0f, 0.2f, 0.1f, 1.0f };

	m_Window->SetVSync(1);
	m_Window->SetWindowColor({ 1.0f, 1.0f, 1.0f, 1.0f });

}

Game::~Game()
{
	delete m_Window;
	delete m_BaseShader;
	delete m_TextureShader;
	delete m_TilesTexture;
	delete m_Renderer;
}

lptm::Vector2D pixelToNormalized(lptm::Vector2D size, lptm::Vector2D windowSize)
{
	return lptm::Vector2D(
		size.x / windowSize.x, 
		size.y / windowSize.y
	);
}

lptm::Vector2D normalizedToPixel(lptm::Vector2D size, lptm::Vector2D windowSize)
{
	return lptm::Vector2D(
		size.x * windowSize.x,
		size.y * windowSize.y
	);
}


void Game::Update()
{
	lptm::Vector2D worldSize = m_WorldSize;
	lptm::Vector2D origin = m_Origin;

	lptm::Vector2D spriteSize = { 40, 20 };
	lptm::Vector2D tileSize = pixelToNormalized({ spriteSize.x, spriteSize.y }, m_Window->GetWindowSize());

	// Ajust offset to match the origin
	m_WorldOffset.x += spriteSize.x * m_Origin.x;
	m_WorldOffset.y += spriteSize.y * m_Origin.y;

	auto ToScreen = [&](int x, int y)
	{
		lptm::Vector2D offset = pixelToNormalized(m_WorldOffset, m_Window->GetWindowSize());
		
		return lptm::Vector2D(
			(x - y) * (tileSize.x / 2) + offset.x,
			(x + y) * (tileSize.y / 2) + offset.y
		);
	};

	auto ToWorldWithOffset = [&](lptm::Vector2D mousePos)
	{
		mousePos.x -= m_WorldOffset.x;
		mousePos.y -= m_WorldOffset.y;

		return lptm::Vector2D(
			((mousePos.x + (2 * mousePos.y) - (spriteSize.x / 2)) / spriteSize.x),
			((-mousePos.x + (2 * mousePos.y) + (spriteSize.x / 2)) / spriteSize.x)
		);
	};

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(m_Window->WindowGUI(), true);
	ImGui_ImplOpenGL3_Init("#version 330");

	while (!m_Window->ShouldClose())
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		m_Window->Clear();
		m_TextureShader->Bind();
		m_TilesTexture->Bind();

		lptm::Vector2D worldPosWithOffset = ToWorldWithOffset(m_Window->GetMousePosInPixels());

		if (
			worldPosWithOffset.y > 0 && 
			worldPosWithOffset.x > 0 &&
			worldPosWithOffset.x <= worldSize.x &&
			worldPosWithOffset.y <= worldSize.y 
			)
		{
			lptm::Vector2D tilePosition = ToScreen((int)worldPosWithOffset.x, (int)worldPosWithOffset.y);
			tilePosition.x += tileSize.x / 2;
			tilePosition.y += tileSize.y / 2;
			m_Renderer->RenderQuad(
				tileSize,
				{
					tilePosition.x,
					tilePosition.y,
					.0f
				},
				m_TilesTexture,
				m_TextureShader,
				{ 0, 5 },
				spriteSize
			);
		}


		// Set tiles
		if (m_Window->IsButtomJustPressed(GLFW_MOUSE_BUTTON_LEFT))
		{
			//m_InitialMousePosition = m_Window->GetMousePos();
			
			// Set tile value
			if (
				worldPosWithOffset.y > 0 && 
				worldPosWithOffset.x > 0 &&
				worldPosWithOffset.x <= worldSize.x &&
				worldPosWithOffset.y <= worldSize.y 
				)
			{
				m_World[(int)((int)worldPosWithOffset.y * worldSize.x + (int)worldPosWithOffset.x)]++;
			}
		}

		if (m_Window->IsKeyPressed(GLFW_KEY_RIGHT))
			m_WorldOffset.x -= 10.0f;
		if (m_Window->IsKeyPressed(GLFW_KEY_LEFT))
			m_WorldOffset.x += 10.0f;
		if (m_Window->IsKeyPressed(GLFW_KEY_UP))
			m_WorldOffset.y += 10.0f;
		if (m_Window->IsKeyPressed(GLFW_KEY_DOWN))
			m_WorldOffset.y -= 10.0f;

		// Panning
		if (m_Window->IsButtomPressed(GLFW_MOUSE_BUTTON_LEFT))
		{ 
			//m_WorldOffset += m_Window->GetMousePos() - m_InitialMousePosition;
			//m_InitialMousePosition = m_Window->GetMousePos();
		}

		{
			ImGui::Text("TILE: %d, %d", (int)worldPosWithOffset.x, (int)worldPosWithOffset.y);
			ImGui::Text("WORLD OFFSET (px): %d, %d", (int)m_WorldOffset.x, (int)m_WorldOffset.y);

			ImGui::End();
		}
		for (int y = 0; y < worldSize.y; y++)
		{
			for (int x = 0; x < worldSize.x; x++)
			{
				lptm::Vector2D sizeTree = { 40, 40 };
				lptm::Vector2D worldSizeTree = pixelToNormalized(sizeTree, m_Window->GetWindowSize());
				lptm::Vector2D tilePosition = ToScreen(x, y);
				tilePosition.x += tileSize.x / 2;
				tilePosition.y += tileSize.y / 2;
				//tilePosition.x += pixelToNormalized({ 500, 500 }, m_Window->GetWindowSize()).x;

				switch (m_World[(int)(y * worldSize.x + x)])
				{
				case 0:
					m_Renderer->RenderQuad(
						tileSize,
						{
							tilePosition.x,
							tilePosition.y,
							0.0f
						},
						m_TilesTexture,
						m_TextureShader,
						{ 1, 5 },
						spriteSize
					);
					break;
				case 1:
					m_Renderer->RenderQuad(
						tileSize,
						{
							tilePosition.x,
							tilePosition.y ,
							0.0f
						},
						m_TilesTexture,
						m_TextureShader,
						{ 2, 5 },
						spriteSize
					);
					break;
				case 2:
					m_Renderer->RenderQuad(
						worldSizeTree,
						{
							tilePosition.x,
							tilePosition.y - (worldSizeTree.y / 4),
							0.0f
						},
						m_TilesTexture,
						m_TextureShader,
						{ 0, 0 },
						sizeTree
					);
					break;
				default:
					m_World[(int)((int)worldPosWithOffset.y * worldSize.x + (int)worldPosWithOffset.x)] = 0;
					m_Renderer->RenderQuad(
						tileSize,
						{
							tilePosition.x,
							tilePosition.y,
							0.0f
						},
						m_TilesTexture,
						m_TextureShader,
						{ 0, 0 },
						spriteSize
					);
					break;
				}
			}
		}

	

		//m_Player.Render(m_Renderer);
		/*if (m_Window->IsButtomPressed(GLFW_MOUSE_BUTTON_1))
		{
			m_ParticleProps.position = { m_Window->GetMousePos().x, m_Window->GetMousePos().y};
			for(int i = 0; i < 10; i++)
				m_ParticleSystem.Emit(m_ParticleProps);
		}

		m_ParticleSystem.Update(m_Window->GetSeconds());
		m_ParticleSystem.Render(m_Renderer);*/

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		m_Renderer->Flush();
		m_Window->Update();
	}
}