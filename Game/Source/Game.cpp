#include "Game.h"

Game::Game()
{
	m_WorldSize = { 20, 12 };
	m_Origin = { 10, 7 };
	m_World = new int[m_WorldSize.x * m_WorldSize.y]{ 0 };

	m_Window = new se::Window();
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

void Game::Update()
{
	lptm::Vector2D worldSize = m_WorldSize;
	lptm::Vector2D origin = m_Origin;

	lptm::Vector2D spriteSize = { 40, 20 };
	lptm::Vector2D tileSize = pixelToNormalized({ spriteSize.x, spriteSize.y }, m_Window->GetWindowSize());

	auto ToScreen = [&](int x, int y)
	{
		return lptm::Vector2D(
			(origin.x * tileSize.x) + (x - y) * (tileSize.x / 2) + tileSize.x / 2,
			(origin.y * tileSize.y) + (x + y) * (tileSize.y / 2) + tileSize.y / 2
		);
	};

	auto ToWorld = [&](lptm::Vector2D mousePos)
	{
		lptm::Vector2D mouse =
		{
			mousePos.x - (int)(origin.x * spriteSize.x),
			mousePos.y - (int)(origin.y * spriteSize.y),
		};

		return lptm::Vector2D(
			(mouse.x + (2 * mouse.y) - (spriteSize.x / 2)) / spriteSize.x,
			(-mouse.x + (2 * mouse.y) + (spriteSize.x / 2)) / spriteSize.x
		);
	};


	while(!m_Window->ShouldClose())
	{
		m_Window->Clear(); 

		lptm::Vector2D mouse = m_Window->GetMousePosInPixels();
		lptm::Vector2D cell = { mouse.x / spriteSize.x, mouse.y / spriteSize.y };
		
		lptm::Vector2D selected =
		{
			(mouse.x / (spriteSize.x / 2) + mouse.y / (spriteSize.y / 2)) / 2,
			(mouse.y / (spriteSize.y / 2) - mouse.x / (spriteSize.x / 2)) / 2
		};

		lptm::Vector2D worldPos = ToWorld(mouse);

		m_TextureShader->Bind();
		m_TilesTexture->Bind();

		// Render select tile
		lptm::Vector2D screenPos = ToScreen(worldPos.x, worldPos.y);
		m_Renderer->RenderQuad(
			tileSize,
			{
				screenPos.x,
				screenPos.y,
				.0f
			},
			m_TilesTexture,
			m_TextureShader,
			{ 0, 5 },
			spriteSize
		);

		if (m_Window->IsButtomJustPressed(GLFW_MOUSE_BUTTON_LEFT))
		{
			m_World[(int)((int)worldPos.y * worldSize.x + (int)worldPos.x)]++;
		}

		for (int y = 0; y < worldSize.y; y++)
		{
			for (int x = 0; x < worldSize.x; x++)
			{
				lptm::Vector2D sizeTree = { 40, 40 };
				lptm::Vector2D worldSizeTree = pixelToNormalized(sizeTree, m_Window->GetWindowSize());
				lptm::Vector2D tilePosition = ToScreen(x, y);

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
					m_World[(int)((int)worldPos.y * worldSize.x + (int)worldPos.x)] = 0;
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

		m_Renderer->Flush();
		m_Window->Update();
	}
}