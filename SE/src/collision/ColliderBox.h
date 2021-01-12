#pragma once

#include "../maths/Vec2.h"

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

	bool IsColliding(ColliderBox* contendor[], ColliderBox& entityCollider, int arraySize);
	void ActivateCollition(ColliderBox* contenedor[], ColliderBox& entityCollider, int arraySize);

	void MoveBoxsColliderUp		(ColliderBox* contenedor[], float y, int arrayLength);
	void MoveBoxsColliderDown	(ColliderBox* contenedor[], float y, int arrayLength);
	void MoveBoxsColliderRight	(ColliderBox* contenedor[], float x, int arrayLength);
	void MoveBoxsColliderLeft	(ColliderBox* contenedor[], float x, int arrayLength);

}}