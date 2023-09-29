#pragma once

#include <SE.h>
#include "Core.h"

class MapRenderer
{
public:
    MapRenderer(uint32_t cols, uint32_t rows, lptm::Vector2D origin, lptm::Vector2D spriteSize, std::shared_ptr<se::Renderer2D>& render);

    void Update();
    void Render();
private:
    lptm::Vector2D m_Offset;
    lptm::Vector2D m_WorldOffset;
    uint32_t m_Cols, m_Rows;

    std::shared_ptr<se::Renderer2D> m_Renderer;

	se::Texture* m_Texture;
	se::Shader* m_Shader;

    lptm::Vector2D m_SpriteSize;
    lptm::Vector2D m_TileSize;
    lptm::Vector2D m_Origin;
};