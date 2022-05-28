#include "GameObject.h"

GameObject::GameObject()
{

}

GameObject::~GameObject()
{
	delete m_Sprite;
}

void GameObject::Render(se::Renderer2D* renderer)
{
	renderer->RenderQuad({ 0.5f, 0.5f }, m_Position, { 0.0f, 0.2f, 0.0f, 1.0f });
}

void GameObject::SetPosition(lptm::Vector3D pos)
{
	m_Position = pos;
}