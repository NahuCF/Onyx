#pragma once

#include <SE.h>
#include <vector>
#include "Mechanics/Entity.h"

struct TileData
{
    bool IsExplored = false;
};

class Map
{
public:
    Map(uint32_t cols, uint32_t rows);

    TileData GetTileData(uint32_t index) { return m_TileData[index]; }

    uint32_t GetTileCount() const { return m_TileData.size();}

    void SetExploredInRange(lptm::Vector2D tileOrigin, int tileVision);
    void CleanExplored();

    void AddEntityAt(int col, int row, lptm::Vector2D mapSize, Entity& entity);
    void DeleteEntityAt(int col, int row, lptm::Vector2D mapSize, Entity& entity);
private:
    std::vector<std::vector<Entity>> m_TileEntities;
    std::vector<TileData> m_TileData;

    uint32_t m_Cols;
    uint32_t m_Rows;
};