#include "pch.h"

#include <cmath>

#include "OrtogonalTilemap.h"

namespace se {

	OrtogonalTilemap::OrtogonalTilemap(uint32_t tilemapWidth, uint32_t tilemapHeight, uint32_t tileWidth, uint32_t tileHeight)
		: m_TileMapWidth(tilemapWidth), m_TileMapHeight(tilemapHeight), m_TileWidth(tileWidth), m_TileHeight(tileHeight)
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

	lptm::Vector2D OrtogonalTilemap::PositionTile() const
	{
		lptm::Vector2D currentTile = GetCurrentTile();
		
		float width = (currentTile.x * m_TileWidth + m_TileWidth / 2 - m_WindowWidth / 2) + m_Offsets.x;
		float height = (currentTile.y * m_TileHeight + m_TileHeight / 2 - m_WindowHeight / 2) + m_Offsets.y;
	
		// Pixels to NDC
		return lptm::Vector2D(width / (m_WindowWidth / 2), -(height / (m_WindowHeight / 2)));
	}

	lptm::Vector2D OrtogonalTilemap::GetCurrentTile() const
	{
		return lptm::Vector2D(std::floor(((m_MousePos.x - m_Offsets.x) / m_TileWidth)), std::floor(((m_MousePos.y + (-m_Offsets.y)) / m_TileHeight)));
	}

	void OrtogonalTilemap::SetTile(uint32_t index)
	{
		m_TilemapData[GetCurrentTile().x + m_TileMapWidth * GetCurrentTile().y] = index;
	}

	void OrtogonalTilemap::SetData(lptm::Vector2D mousePos, lptm::Vector2D offsets, lptm::Vector2D windowSize)
	{
		m_MousePos = mousePos;
		m_Offsets = offsets;
		m_WindowWidth = windowSize.x;
		m_WindowHeight = windowSize.y;
	}

}