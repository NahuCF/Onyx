#include "Map.h"

Map::Map(uint32_t cols, uint32_t rows)
    : m_Cols(cols)
    , m_Rows(rows)
    
{
    TileData a;
    m_TileData = std::vector<TileData>(cols * rows, a);
}

void Map::SetExploredInRange(lptm::Vector2D origin, int tileVision)
{
    for(int y = -tileVision; y <= tileVision; y++)
    {
        for(int x = -tileVision; x <= tileVision; x++)
        {
            int32_t posX = (int)origin.x + x; 
            int32_t posY = (int)origin.y + y; 

            // Avoid see another side of map
            if(posX < 0 || posY < 0)
                continue;

            uint32_t index = posY * m_Rows + posX;

            if(index >= m_TileData.size())
                continue;   

            m_TileData[index].Status = TileStatus::Seeing;

            //ImGui::Text("PosX: %i PosY: %i Index: %i, Origin: %i, %i", x, y, index, (int)origin.x, (int)origin.y);
        }
    }
}

void Map::CleanExplored()
{
    for(auto& tile : m_TileData) 
    {
        if(tile.Status == TileStatus::Seeing)
            tile.Status = TileStatus::Explored;
    }
}

void Map::AddEntityAt(int col, int row, lptm::Vector2D mapSize, Entity& entity)
{

}

void Map::DeleteEntityAt(int col, int row, lptm::Vector2D mapSize, Entity& entity)
{

}
