#include "MapRenderer.h"
#include "Functions.h"
#include <algorithm>
#include "Enums.h"

MapRenderer::MapRenderer(uint32_t cols, uint32_t rows, lptm::Vector2D origin, lptm::Vector2D spriteSize, std::shared_ptr<Map> &map, std::shared_ptr<se::Renderer2D>& render)
    : m_Cols(cols)
    , m_Rows(rows)
    , m_Origin(origin)
    , m_SpriteSize(spriteSize)
    , m_Map(map)
    , m_Renderer(render)
{
	m_Texture = new se::Texture("Assets/Textures/isometric_demo.png");
	m_Shader = new se::Shader("Assets/Shaders/Shader.vs", "Assets/Shaders/Shader.fs");
	m_TileSize = PixelToNormalized({ spriteSize.x, spriteSize.y }, m_Renderer->GetWindow().GetWindowSize());

    m_Camera = m_Renderer->GetCamera().lock();
}

void MapRenderer::Update()
{
	se::Window& window = m_Renderer->GetWindow();

    float velocity = 0.01f;

	if (window.IsKeyPressed(GLFW_KEY_RIGHT))
		m_WorldOffset.x += velocity;
	if (window.IsKeyPressed(GLFW_KEY_LEFT))
		m_WorldOffset.x -= velocity;
	if (window.IsKeyPressed(GLFW_KEY_UP))
		m_WorldOffset.y -= velocity;
	if (window.IsKeyPressed(GLFW_KEY_DOWN))
		m_WorldOffset.y += velocity;

    m_Camera->SetPosition({m_WorldOffset.x, m_WorldOffset.y, 0.0f});

    {
		lptm::Vector2D tile = ToWorld(
			m_Renderer->GetWindow().GetMousePosInPixels(), 
			m_SpriteSize, 
			NormalizedToPixel({m_Camera->GetPosition().x, m_Camera->GetPosition().y}, m_Renderer->GetWindow().GetWindowSize())
		);
        tile.x = (float)(int)tile.x;
        tile.y = (float)(int)tile.y;

        uint32_t index = WorldToIndex(tile, {(float)m_Cols, (float)m_Rows});

		ImGui::Text("Tile Position %d, %d Index: %i", (int)tile.x, (int)tile.y, index);
	}
}

void MapRenderer::Render()
{
	m_Texture->Bind();
	m_Shader->Bind();
    
	for (int y = m_Cols - 1; y >= 0; y--)
	{
		for (int x = m_Rows - 1; x >= 0; x--)
		{
            lptm::Vector3D cameraPosition = m_Camera->GetPosition();
			lptm::Vector2D tilePosition = ToScreen(x, y, m_TileSize, {cameraPosition.x, cameraPosition.y});

			// Avoid render outside screen
			if (tilePosition.x < -0.5f || tilePosition.x > 1.5f || tilePosition.y < -0.5f || tilePosition.x > 1.5f)
				continue;

            int index = y * m_Rows + x;

            TileData data = m_Map->GetTileData(index);
			if(data.Status == TileStatus::Unexplored)
				continue;

			// idk
			tilePosition.x += m_TileSize.x / 2;
			tilePosition.y += m_TileSize.y / 2;

			if(data.Status == TileStatus::Seeing) 
            {
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

            if(data.Status == TileStatus::Explored)
            {
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

                //Fog
                m_Renderer->RenderQuad(
                    m_TileSize,
                    {
                        tilePosition.x,
                        tilePosition.y,
                        0.1f
                    },
                    m_Texture,
                    m_Shader,
                    { 3, 2 },
                    m_SpriteSize
                );
            }
		}
	}
}