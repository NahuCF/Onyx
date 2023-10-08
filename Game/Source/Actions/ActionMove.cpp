#include "Actions/ActionMove.h"

ActionMove::ActionMove()
{
}

std::shared_ptr<ActionMove> ActionMove::MoveTo(lptm::Vector2D destination, std::shared_ptr<Entity>& entity)
{
    std::shared_ptr<ActionMove> action = std::make_shared<ActionMove>();
    action->SetDestination(destination);
    action->SetEntity(entity);

    return action;
}

EventType ActionMove::Update()
{
    std::shared_ptr<Entity> entity = m_Entity.lock();

    lptm::Vector2D position = entity->GetPosition();

    if(m_Destination == position)
        return EventType::Success;

    float speed = entity->GetSpeed();

    // Calculate angle
    lptm::Vector2D vector = m_Destination - position;
    lptm::Vector2D direction = lptm::Normalize(vector);
    lptm::Vector2D velocity = direction * speed;
    lptm::Vector2D newPos = position + velocity;

    entity->SetPosition(newPos);
    
    // If next step is > than distance set position in destination
    if(lptm::VectorModule(velocity) > lptm::VectorModule(vector))
    {
        entity->SetPosition(m_Destination);
        return EventType::Success;
    }


    return EventType::Processing;
}
