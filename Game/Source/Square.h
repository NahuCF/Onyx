#pragma once

#include <SE.h>

class Square
{
public:
    Square(std::vector<lptm::Vector2D>& vertices);

    lptm::Vector2D GetTopLeft();
    lptm::Vector2D GetBottomRigh();
private:
    std::vector<lptm::Vector2D> m_Vertices;
};