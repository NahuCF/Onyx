#include "pch.h"

#include "Tilemap.h"

namespace se {

	Tilemap::Tilemap(uint32_t tilemapWidth, uint32_t tilemapHeight)
		: m_TileMapWidth(tilemapWidth), m_TileMapHeight(tilemapHeight)
	{
		m_TilemapData.resize(tilemapWidth * tilemapHeight);
	}
	
	Tilemap::~Tilemap()
	{
		std::vector<uint32_t>().swap(m_TilemapData);
	}

	lptm::Vector2D Tilemap::GetCurrentTile(se::Window* window) const
	{
		return lptm::Vector2D((int)((window->GetMouseX() - window->GetOffsets().x) / m_TileMapWidth), (int)((window->GetMouseY() + (-window->GetOffsets().y)) / m_TileMapHeight));
	}

}