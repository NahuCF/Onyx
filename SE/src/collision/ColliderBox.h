#pragma once

#include "../maths/Vec2.h"
#include "src/graphics/Shader.h"

namespace se { namespace collision {


	struct ColliderBox
	{
		ColliderBox(float width, float height, float posX, float posY);
		void AddCollider(std::vector<ColliderBox*> &colliderContenedor);
		void MoveCollider(float x, float y);

		maths::vec2 m_Min;
		maths::vec2 m_Max;

		float m_Width;
		float m_Height;
	};

	bool IsColliding(std::vector<ColliderBox*> &colliderContenedor, ColliderBox& entityCollider);
	bool IsGointToCollide(std::vector<ColliderBox*> &colliderContenedor, ColliderBox& entityCollider, float xMove, float yMove);

	void MoveBoxsColliderUp		(std::vector<ColliderBox*> &colliderContenedor, float y);
	void MoveBoxsColliderDown	(std::vector<ColliderBox*> &colliderContenedor, float y);
	void MoveBoxsColliderRight	(std::vector<ColliderBox*> &colliderContenedor, float x);
	void MoveBoxsColliderLeft	(std::vector<ColliderBox*> &colliderContenedor, float x);

}}