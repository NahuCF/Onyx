#pragma once

#include "../maths/Vec2.h"
#include "src/graphics/Shader.h"

namespace se { namespace collision {


	struct ColliderBox
	{
		ColliderBox(float width, float height, float posX, float posY);
		void AddCollider(ColliderBox* contenedor[], int pos);

		maths::vec2 m_Min;
		maths::vec2 m_Max;

		float m_Width;
		float m_Height;
	};

	bool IsColliding(ColliderBox* contenedor[], ColliderBox& entityCollider, int arrayLength);
	bool IsGointToCollide(ColliderBox* contenedor[], ColliderBox& entityCollider, int arrayLength, float move);
	void ActivateCollition(ColliderBox* contenedor[], ColliderBox& entityCollider, graphics::Shader* shaderContenedor[], int contenedorLength, int shaderLength);

	void MoveBoxsColliderUp		(ColliderBox* contenedor[], float y, int arrayLength);
	void MoveBoxsColliderDown	(ColliderBox* contenedor[], float y, int arrayLength);
	void MoveBoxsColliderRight	(ColliderBox* contenedor[], float x, int arrayLength);
	void MoveBoxsColliderLeft	(ColliderBox* contenedor[], float x, int arrayLength);

}}