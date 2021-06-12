#pragma once

#include <vector>

#include <SE.h>

namespace se {

	class Tilemap
	{
	public:
		virtual ~Tilemap() {}
		
		virtual lptm::Vector2D GetCurrentTile() const = 0;
		virtual uint32_t GetTileValue() const = 0;
		virtual lptm::Vector2D GetTileSize() const = 0;
		virtual lptm::Vector2D GetTilemapSize() const = 0;

		virtual lptm::Vector2D PositionTile() const = 0;

		virtual void SetTile(uint32_t index) = 0;
		virtual void SetData(lptm::Vector2D mousePos, lptm::Vector2D offsets, lptm::Vector2D windowSize) = 0;
	};

}