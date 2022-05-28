#pragma once

#include <SE.h>

class GameObject
{
public:
	GameObject();
	~GameObject();

	void Update();
	void Render(se::Renderer2D* renderer);

	void SetPosition(lptm::Vector3D pos);
	const lptm::Vector3D GetPosition() { return m_Position; };
private:
	lptm::Vector3D m_Position;
	se::Texture* m_Sprite;
};