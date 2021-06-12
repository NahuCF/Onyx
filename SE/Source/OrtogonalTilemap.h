#pragma once

#include <vector>

#include <SE.h>
#include "Tilemap.h"

namespace se {

	class OrtogonalTilemap : public Tilemap
	{
	public:
		OrtogonalTilemap(uint32_t tilemapWidth, uint32_t tilemapHeight, uint32_t tileWidth, uint32_t tileHeight);
		virtual ~OrtogonalTilemap() override;

		virtual lptm::Vector2D GetCurrentTile() const override;
		virtual uint32_t GetTileValue() const override;

		virtual lptm::Vector2D PositionTile() const override;

		virtual void SetTile(uint32_t index) override;
		virtual void SetData(lptm::Vector2D mousePos, lptm::Vector2D offsets, lptm::Vector2D windowSize) override;
	private:
		uint32_t m_TileMapWidth, m_TileMapHeight;
		uint32_t m_TileWidth, m_TileHeight;

		uint32_t m_WindowWidth, m_WindowHeight;

		lptm::Vector2D m_CurrentTile;
		lptm::Vector2D m_MousePos, m_Offsets;

		std::vector<uint32_t> m_TilemapData;
	};

}