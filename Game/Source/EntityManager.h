#pragma once

#include <SE.h>
#include "Mechanics/Entity.h"

typedef std::vector<std::shared_ptr<Entity>> EntityVector;

// Stores all units in the worlds and 
// update them, handle selection
class EntityManager
{
public:
    EntityManager(uint32_t entityCount, std::shared_ptr<se::Renderer2D> renderer);

    void Update();

    void Add(const std::shared_ptr<Entity>& entity) { m_Entities.push_back(entity); }
    void Remove(const std::shared_ptr<Entity>& entity);

    void AddActionToSelected(ActionPtr action);
    void SetSelected(lptm::Vector2D begin, lptm::Vector2D end,std::shared_ptr<Entity> entity);

    void OnRightClick(lptm::Vector2D normalizedMousePos);

    void MoveUnitTo(std::shared_ptr<Entity>& entity, lptm::Vector2D destination);
private:
    EntityVector m_Entities;
    EntityVector m_SelectedEntities;
    std::weak_ptr<se::Renderer2D> m_Renderer;
};