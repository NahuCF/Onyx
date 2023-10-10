#include "EntityManager.h"
#include "Enums.h"
#include "Actions/ActionMove.h"

EntityManager::EntityManager(uint32_t entityCount, std::shared_ptr<se::Renderer2D> renderer)
    : m_Renderer(renderer)
{
    m_Entities.reserve(entityCount);
}

// Loops through the entities and execute their actions
void EntityManager::Update()
{
    EntityVector::iterator i;
    for (i = m_Entities.begin(); i != m_Entities.end(); i++) {
        (*i)->Update();
        (*i)->Render(m_Renderer.lock());
    }
}

void EntityManager::Remove(const std::shared_ptr<Entity>& entity)
{
    EntityVector::iterator it = std::find(m_Entities.begin(), m_Entities.end(), entity);
    if(it != m_Entities.end()) {
        m_Entities.erase(it);
    }
}

void EntityManager::AddActionToSelected(ActionPtr action)
{
    EntityVector::iterator it;
    for (it = m_SelectedEntities.begin(); it != m_SelectedEntities.end(); it++) {
        (*it)->AddAction(action);
    }
}

void EntityManager::SetSelected(lptm::Vector2D begin, lptm::Vector2D end,std::shared_ptr<Entity> entity)
{
    EntityVector::iterator it;
    for(it = m_Entities.begin(); it != m_Entities.end(); it ++ ){
        (*it)->SetColor({1.0f, 0.0, 0.0f, 1.0f});
    }

    m_SelectedEntities.clear();

    lptm::Vector3D cameraPosition = m_Renderer.lock()->GetCamera().lock()->GetPosition();
    begin = begin - lptm::Vector2D(cameraPosition.x, cameraPosition.y);
    end = end - lptm::Vector2D(cameraPosition.x, cameraPosition.y);

    //EntityVector::iterator it;
    for(it = m_Entities.begin(); it != m_Entities.end(); it ++ ){
        lptm::Vector2D entityPosition = (*it)->GetPosition();

        if(entityPosition.x > begin.x && entityPosition.x < end.x
        && entityPosition.y > begin.y && entityPosition.y < end.y)
        {
            (*it)->SetColor({0.0f, 0.0, 1.0f, 1.0f});
            m_SelectedEntities.push_back(*it);
        }

    }
}

void EntityManager::OnRightClick(lptm::Vector2D normalizedMousePos)
{
    // Move
    EntityVector::iterator it;
    for(it = m_SelectedEntities.begin(); it != m_SelectedEntities.end(); it ++ ){
        MoveUnitTo(*it, normalizedMousePos);
    }
}

void EntityManager::MoveUnitTo(std::shared_ptr<Entity>& entity, lptm::Vector2D destination)
{
   entity->AddAction(ActionMove::MoveTo(destination, entity));
}