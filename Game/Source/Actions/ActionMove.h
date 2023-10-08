#pragma once

#include <SE.h>
#include "Mechanics/Entity.h"
#include "Actions/Action.h"
#include "Enums.h"

class ActionMove : public Action
{
public:
    ActionMove();

    static std::shared_ptr<ActionMove> MoveTo(lptm::Vector2D destination, std::shared_ptr<Entity>& entity);

    EventType Update();

    void SetDestination(lptm::Vector2D& destination) { m_Destination = destination; }
    void SetEntity(std::shared_ptr<Entity>& entity) { m_Entity = entity; }
private:
    std::vector<int> m_Path;

    lptm::Vector2D m_Destination;

    std::weak_ptr<Entity> m_Entity;

    std::weak_ptr<se::Camera> m_Camera;
};