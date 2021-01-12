#include "ColliderBox.h"
#include "src/graphics/window.h"
#include <iostream>
namespace se { namespace collision {

	ColliderBox::ColliderBox(float width, float height, float posX, float posY)
		: m_Width(width), m_Height(height)
	{
		m_Min.x += posX - width / 2;
		m_Min.y += posY - height / 2;

		m_Max.x += posX + width / 2;
		m_Max.y += posY + height / 2;
	}

	void ColliderBox::AddCollider(ColliderBox* contenedor[], int pos)
	{
		contenedor[pos] = this;
	}

	bool IsColliding(ColliderBox* contendor[], ColliderBox& entityCollider, int arraySize)
	{
		//One is the container
		for(int i = 0; i < arraySize; i++)
		{
			bool collisionX = contendor[i]->m_Min.x + contendor[i]->m_Width >= entityCollider.m_Min.x && entityCollider.m_Min.x + entityCollider.m_Width >= contendor[i]->m_Min.x;
			bool collisionY = contendor[i]->m_Min.y + contendor[i]->m_Height >= entityCollider.m_Min.y && entityCollider.m_Min.y + entityCollider.m_Height >= contendor[i]->m_Min.y;

			if(collisionX && collisionY == true)
			{
				return true;
			}
		}

		return false;
	}

	void ActivateCollition(ColliderBox* contenedor[], ColliderBox& entityCollider, int arraySize)
	{
		for(int i = 0; i < arraySize; i++)
		{
		}
	}

	void MoveBoxsColliderUp(ColliderBox* contenedor[], float y, int arrayLength)
	{
		for(int i = 0; i < arrayLength; i++)
		{
			contenedor[i]->m_Min.y -= y;
			contenedor[i]->m_Max.y -= y;
		}
	}

	void MoveBoxsColliderDown(ColliderBox* contenedor[], float y, int arrayLength)
	{
		for (int i = 0; i < arrayLength; i++)
		{
			contenedor[i]->m_Min.y += y;
			contenedor[i]->m_Max.y += y;
		}
	}

	void MoveBoxsColliderRight(ColliderBox* contenedor[], float x, int arrayLength)
	{
		for (int i = 0; i < arrayLength; i++)
		{
			contenedor[i]->m_Min.x -= x;
			contenedor[i]->m_Max.x -= x;
		}
	}

	void MoveBoxsColliderLeft(ColliderBox* contenedor[], float x, int arrayLength)
	{
		for (int i = 0; i < arrayLength; i++)
		{
			contenedor[i]->m_Min.x += x;
			contenedor[i]->m_Max.x += x;
		}
	}

}}