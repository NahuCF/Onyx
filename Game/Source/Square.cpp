#include "Square.h"

Square::Square(std::vector<lptm::Vector2D>& vertices) 
{
    m_Vertices = vertices;    
}

lptm::Vector2D Square::GetTopLeft() 
{
    lptm::Vector2D topLeft;
    for(auto& vertice : m_Vertices)
    {
       if(vertice.x < topLeft.x && vertice.y > topLeft.y) 
            topLeft = vertice;
    }

    return topLeft;
}

lptm::Vector2D Square::GetBottomRigh() 
{
    lptm::Vector2D bottomRight;
    for(auto& vertice : m_Vertices)
    {
       if(vertice.x > bottomRight.x && vertice.y < bottomRight.y) 
            bottomRight = vertice;
    }

    return bottomRight;
}
