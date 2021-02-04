#pragma once

#include "Source/Maths/Vector2D.h"
#include "Source/graphics/Shader.h"

namespace se {

	struct ColliderBox
	{
		ColliderBox(float width, float height, float posX, float posY);
		void AddCollider(std::vector<ColliderBox*> &colliderContenedor);
		void MoveCollider(float x, float y);

		lptm::Vector2D m_Min;
		lptm::Vector2D m_Max;

		float m_Width;
		float m_Height;
	};

	bool IsColliding(std::vector<ColliderBox*> &colliderContenedor, ColliderBox& entityCollider);
	bool IsGointToCollide(std::vector<ColliderBox*> &colliderContenedor, ColliderBox& entityCollider, float xMove, float yMove);

	void MoveBoxsColliderUp		(std::vector<ColliderBox*> &colliderContenedor, float y);
	void MoveBoxsColliderDown	(std::vector<ColliderBox*> &colliderContenedor, float y);
	void MoveBoxsColliderRight	(std::vector<ColliderBox*> &colliderContenedor, float x);
	void MoveBoxsColliderLeft	(std::vector<ColliderBox*> &colliderContenedor, float x);

}