#include "EntityManager.h"
#include "Enums.h"

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