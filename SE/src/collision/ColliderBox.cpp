#include "ColliderBox.h"
#include "src/graphics/window.h"

namespace se { namespace collision {

	ColliderBox::ColliderBox(float width, float height, float posX, float posY)
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

	bool IsColliding(ColliderBox* contendor[], int arraySize, ColliderBox& entity)
	{
		for(int i = 0; i < arraySize; i++)
		{
		}
	}

	void ActivateCollition(ColliderBox* contenedor[], graphics::Shader& entity)
	{

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