#pragma once

#include <vector>

#include <SE.h>

namespace se {

	class Tilemap
	{
	public:
		Tilemap(uint32_t tilemapWidth, uint32_t tilemapHeight);
		~Tilemap();

		lptm::Vector2D GetCurrentTile(se::Window* window) const;
	private:
		uint32_t m_TileMapWidth, m_TileMapHeight;
		lptm::Vector2D m_CurrentTile;
		std::vector<uint32_t> m_TilemapData;
	};

}