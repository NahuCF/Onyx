#include "pch.h"

#include "ColliderBox.h"
#include "Source/graphics/window.h"
#include "glm.hpp"

namespace se {

	ColliderBox::ColliderBox(float width, float height, float posX, float posY)
		: m_Width(width), m_Height(height)
	{
		m_Min.x += posX - width / 2;
		m_Min.y += posY - height / 2;

		m_Max.x += posX + width / 2;
		m_Max.y += posY + height / 2;
	}

	void ColliderBox::AddCollider(std::vector<ColliderBox*> &colliderContenedor)
	{
		colliderContenedor.push_back(this);
	}

	void ColliderBox::MoveCollider(float x, float y)
	{
		this->m_Min.y += y;
		this->m_Min.x += x;
	}

	bool IsColliding(std::vector<ColliderBox*> &colliderContenedor, ColliderBox& entityCollider)
	{
		for(int i = 0; i < colliderContenedor.size(); i++)
		{
			bool collisionX = colliderContenedor[i]->m_Min.x + colliderContenedor[i]->m_Width >= entityCollider.m_Min.x && entityCollider.m_Min.x + entityCollider.m_Width >= colliderContenedor[i]->m_Min.x;
			bool collisionY = colliderContenedor[i]->m_Min.y + colliderContenedor[i]->m_Height >= entityCollider.m_Min.y && entityCollider.m_Min.y + entityCollider.m_Height >= colliderContenedor[i]->m_Min.y;

			if(collisionX && collisionY == true)
			{
				return true;
			}
		}

		return false;
	}

	bool IsGointToCollide(std::vector<ColliderBox*> &colliderContenedor, ColliderBox& entityCollider, float xMove, float yMove)
	{
		for(int i = 0; i < colliderContenedor.size(); i++)
		{
			bool collisionX = colliderContenedor[i]->m_Min.x + colliderContenedor[i]->m_Width - xMove >= entityCollider.m_Min.x && entityCollider.m_Min.x + entityCollider.m_Width >= colliderContenedor[i]->m_Min.x - xMove;
			bool collisionY = colliderContenedor[i]->m_Min.y + colliderContenedor[i]->m_Height - yMove >= entityCollider.m_Min.y && entityCollider.m_Min.y + entityCollider.m_Height >= colliderContenedor[i]->m_Min.y - yMove;

			if(collisionX && collisionY == true)
			{
				return true;
			}
		}

		return false;
	}
	
	void MoveBoxsColliderUp(std::vector<ColliderBox*> &colliderContenedor, float y)
	{
		for(int i = 0; i < colliderContenedor.size(); i++)
		{
			colliderContenedor[i]->m_Min.y -= y;
			colliderContenedor[i]->m_Max.y -= y;
		}
	}

	void MoveBoxsColliderDown(std::vector<ColliderBox*> &colliderContenedor, float y)
	{
		for(int i = 0; i < colliderContenedor.size(); i++)
		{
			colliderContenedor[i]->m_Min.y += y;
			colliderContenedor[i]->m_Max.y += y;
		}
	}

	void MoveBoxsColliderRight(std::vector<ColliderBox*> &colliderContenedor, float x)
	{
		for(int i = 0; i < colliderContenedor.size(); i++)
		{
			colliderContenedor[i]->m_Min.x -= x;
			colliderContenedor[i]->m_Max.x -= x;
		}
	}

	void MoveBoxsColliderLeft(std::vector<ColliderBox*> &colliderContenedor, float x)
	{
		for(int i = 0; i < colliderContenedor.size(); i++)
		{
			colliderContenedor[i]->m_Min.x += x;
			colliderContenedor[i]->m_Max.x += x;
		}
	}

}