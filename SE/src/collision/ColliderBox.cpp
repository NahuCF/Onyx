#include "ColliderBox.h"
#include "src/graphics/window.h"
#include "glm.hpp"

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

	bool IsColliding(ColliderBox* contenedor[], ColliderBox& entityCollider, int arrayLength)
	{
		for(int i = 0; i < arrayLength; i++)
		{
			bool collisionX = contenedor[i]->m_Min.x + contenedor[i]->m_Width >= entityCollider.m_Min.x && entityCollider.m_Min.x + entityCollider.m_Width >= contenedor[i]->m_Min.x;
			bool collisionY = contenedor[i]->m_Min.y + contenedor[i]->m_Height >= entityCollider.m_Min.y && entityCollider.m_Min.y + entityCollider.m_Height >= contenedor[i]->m_Min.y;

			if(collisionX && collisionY == true)
			{
				return true;
			}
		}

		return false;
	}

	bool IsGointToCollide(ColliderBox* contenedor[], ColliderBox& entityCollider, int arrayLength, float move)
	{
		for (int i = 0; i < arrayLength; i++)
		{
			bool collisionX = contenedor[i]->m_Min.x + contenedor[i]->m_Width >= entityCollider.m_Min.x && entityCollider.m_Min.x + entityCollider.m_Width + move >= contenedor[i]->m_Min.x;
			bool collisionY = contenedor[i]->m_Min.y + contenedor[i]->m_Height >= entityCollider.m_Min.y && entityCollider.m_Min.y + entityCollider.m_Height + move >= contenedor[i]->m_Min.y;

			if (collisionX && collisionY == true)
			{
				return true;
			}
		}

		return false;
	}

	void ActivateCollition(ColliderBox* contenedor[], ColliderBox& entityCollider, graphics::Shader* shaderContenedor[], int contenedorLength, int shaderLength)
	{
		for(int i = 0; i < contenedorLength; i++)
		{
			//Left
			if(entityCollider.m_Min.x + entityCollider.m_Width >= contenedor[i]->m_Min.x && entityCollider.m_Min.x <= contenedor[i]->m_Min.x)
			{
				float distanceSeparate = entityCollider.m_Width - (contenedor[i]->m_Min.x);
				std::cout << distanceSeparate << std::endl;
				for(int i = 0; i < shaderLength; i++)
				{
					shaderContenedor[i]->SetPos(glm::vec3(shaderContenedor[i]->m_ActualXPos + distanceSeparate, shaderContenedor[i]->m_ActualYPos, 0));

				}
				for(int i = 0; i < contenedorLength; i++)
				{
					contenedor[i]->m_Min.x += distanceSeparate;
					contenedor[i]->m_Max.x += distanceSeparate;
				}
			}
			//Right
			if(contenedor[i]->m_Min.x + contenedor[i]->m_Width >= entityCollider.m_Min.x && entityCollider.m_Min.x >= contenedor[i]->m_Min.x)
			{
				float distanceSeparate = contenedor[i]->m_Min.x + contenedor[i]->m_Width - entityCollider.m_Min.x;
				for(int i = 0; i < shaderLength; i++)
				{
					shaderContenedor[i]->SetPos(glm::vec3(shaderContenedor[i]->m_ActualXPos - distanceSeparate, shaderContenedor[i]->m_ActualYPos, 0));

				}
				for(int i = 0; i < contenedorLength; i++)
				{
					contenedor[i]->m_Min.x -= distanceSeparate;
					contenedor[i]->m_Max.x -= distanceSeparate;
				}
			}
			//Buttom
			if(entityCollider.m_Min.y + entityCollider.m_Height >= contenedor[i]->m_Min.y && entityCollider.m_Min.y <= contenedor[i]->m_Min.y)
			{
				float distanceSeparate = entityCollider.m_Min.y + entityCollider.m_Height - contenedor[i]->m_Min.y;
				for(int i = 0; i < shaderLength; i++)
				{
					shaderContenedor[i]->SetPos(glm::vec3(shaderContenedor[i]->m_ActualXPos, shaderContenedor[i]->m_ActualYPos + distanceSeparate, 0));

				}
				for(int i = 0; i < contenedorLength; i++)
				{
					contenedor[i]->m_Min.y += distanceSeparate;
					contenedor[i]->m_Max.y += distanceSeparate;
				}
			}
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