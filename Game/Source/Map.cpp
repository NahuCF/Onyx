#include "Map.h"

Map::Map(uint32_t cols, uint32_t rows)
{
    m_TileData.resize(cols * rows);
}

void Map::AddEntityAt(int col, int row, lptm::Vector2D mapSize, Entity& entity)
{

}

void Map::DeleteEntityAt(int col, int row, lptm::Vector2D mapSize, Entity& entity)
{

}
