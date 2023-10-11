#pragma once

#include <SE.h>
#include "Entity.h"

class Unit : public Entity
{
public:
    Unit();

    void Update();
    void Render(std::shared_ptr<se::Renderer2D> renderer);

    void HandleActions();
};