#include "pch.h"

#include "OrtogonalTilemap.h"

namespace se {

	OrtogonalTilemap::OrtogonalTilemap(uint32_t tilemapWidth, uint32_t tilemapHeight)
		: m_TileMapWidth(tilemapWidth), m_TileMapHeight(tilemapHeight)
	{
		m_TilemapData.resize(tilemapWidth * tilemapHeight);
	}

	OrtogonalTilemap::~OrtogonalTilemap()
	{
		std::vector<uint32_t>().swap(m_TilemapData);
	}

	uint32_t OrtogonalTilemap::GetTileValue() const
	{
		return m_TilemapData[GetCurrentTile().x + m_TileMapWidth * GetCurrentTile().y];
	}

	lptm::Vector2D OrtogonalTilemap::GetCurrentTile() const
	{
		return lptm::Vector2D((int)((m_MousePos.x - m_Offsets.x) / m_TileMapWidth), (int)((m_MousePos.y + (-m_Offsets.y)) / m_TileMapHeight));
	}

	void OrtogonalTilemap::SetTile(uint32_t index)
	{
		m_TilemapData[GetCurrentTile().x + m_TileMapWidth * GetCurrentTile().y] = index;
	}

	void OrtogonalTilemap::SetData(lptm::Vector2D mousePos, lptm::Vector2D offsets)
	{
		m_MousePos = mousePos;
		m_Offsets = offsets;
	}

}