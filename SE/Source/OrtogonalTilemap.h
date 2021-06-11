#pragma once

#include <vector>

#include <SE.h>
#include "Tilemap.h"

namespace se {

	class OrtogonalTilemap : public Tilemap
	{
	public:
		OrtogonalTilemap(uint32_t tilemapWidth, uint32_t tilemapHeight);
		virtual ~OrtogonalTilemap() override;

		virtual lptm::Vector2D GetCurrentTile() const override;
		virtual uint32_t GetTileValue() const override;
		virtual void SetTile(uint32_t index) override;
		virtual void SetData(lptm::Vector2D mousePos, lptm::Vector2D offsets) override;
	private:
		uint32_t m_TileMapWidth, m_TileMapHeight;

		lptm::Vector2D m_CurrentTile;
		lptm::Vector2D m_MousePos, m_Offsets;

		std::vector<uint32_t> m_TilemapData;
	};

}