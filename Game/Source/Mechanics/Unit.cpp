#include "Unit.h"
#include "Enums.h"

Unit::Unit()
{

}

void Unit::Update()
{
    this->HandleActions();

}

void Unit::Render(std::shared_ptr<se::Renderer2D> renderer)
{
    lptm::Vector3D cameraPos = renderer->GetCamera().lock()->GetPosition();

    renderer.get()->RenderQuad(
        { 0.05f, 0.05f },
        { m_Position.x + cameraPos.x, m_Position.y + cameraPos.y, 0.5f },
        m_Color
    );
}

void Unit::HandleActions()
{
	if (m_ActionQueue.empty())
        return;

    bool completed = m_ActionQueue.front()->Update() == EventType::Success;
    if(completed)
        m_ActionQueue.erase(m_ActionQueue.begin());

}