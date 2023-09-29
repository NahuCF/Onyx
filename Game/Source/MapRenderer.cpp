#include "MapRenderer.h"
#include "Functions.h"

MapRenderer::MapRenderer(uint32_t cols, uint32_t rows, lptm::Vector2D origin, lptm::Vector2D spriteSize, std::shared_ptr<se::Renderer2D>& render)
    : m_Cols(cols)
    , m_Rows(rows)
    , m_Origin(origin)
    , m_SpriteSize(spriteSize)
    , m_Renderer(render)
{
	m_Texture = new se::Texture("Assets/Textures/isometric_demo.png");
	m_Shader = new se::Shader("Assets/Shaders/TextureShader.vs", "Assets/Shaders/TextureShader.fs");
	m_TileSize = PixelToNormalized({ spriteSize.x, spriteSize.y }, m_Renderer->GetWindow().GetWindowSize());
}

void MapRenderer::Update()
{
	se::Window& window = m_Renderer->GetWindow();

	if (window.IsKeyPressed(GLFW_KEY_RIGHT))
		m_WorldOffset.x -= 10.0f;
	if (window.IsKeyPressed(GLFW_KEY_LEFT))
		m_WorldOffset.x += 10.0f;
	if (window.IsKeyPressed(GLFW_KEY_UP))
		m_WorldOffset.y += 10.0f;
	if (window.IsKeyPressed(GLFW_KEY_DOWN))
		m_WorldOffset.y -= 10.0f;

}

void MapRenderer::Render()
{
	m_Texture->Bind();
	m_Shader->Bind();

	for (int y = m_Cols - 1; y > 0; y--)
	{
		for (int x = m_Rows - 1; x > 0; x--)
		{
			lptm::Vector2D tilePosition = ToScreen(x, y, m_TileSize, m_WorldOffset, m_Renderer->GetWindow().GetWindowSize());

			// Avoid render outside screen
			if (tilePosition.x < -0.5f || tilePosition.x > 1.5f || tilePosition.y < -0.5f || tilePosition.x > 1.5f)
				continue;

			// idk
			tilePosition.x += m_TileSize.x / 2;
			tilePosition.y += m_TileSize.y / 2;

			m_Renderer->RenderQuad(
				m_TileSize,
				{
					tilePosition.x,
					tilePosition.y,
					0.0f
				},
				m_Texture,
				m_Shader,
				{ 2, 5 },
				m_SpriteSize
			);
		}
	}
}