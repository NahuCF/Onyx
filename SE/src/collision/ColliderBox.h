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
	};

	bool IsColliding(ColliderBox* contendor[], int arraySize, ColliderBox& entity);
	void ActivateCollition(ColliderBox* contenedor[], graphics::Shader& entity);

	void MoveBoxsColliderUp		(ColliderBox* contenedor[], float y, int arrayLength);
	void MoveBoxsColliderDown	(ColliderBox* contenedor[], float y, int arrayLength);
	void MoveBoxsColliderRight	(ColliderBox* contenedor[], float x, int arrayLength);
	void MoveBoxsColliderLeft	(ColliderBox* contenedor[], float x, int arrayLength);

}}