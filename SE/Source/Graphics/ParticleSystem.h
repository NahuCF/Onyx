#pragma once

#include "Maths/Maths.h";
#include "Renderer2D.h";

namespace se {

	struct ParticleProperties
	{
		lptm::Vector2D position, velocity, velocityVariation;
		lptm::Vector4D color, colorBegin, colorEnd;

		float sizeBegin, sizeEnd, sizeVariation;
		float rotation;
		float lifetime = 1.0f;
	};

	class ParticleSystem
	{
	public:
		ParticleSystem(int particleCount = 10000);
		~ParticleSystem();

		void Update(float timestamp);
		void Render(se::Renderer2D* renderer);

		void Emit(const ParticleProperties& props);
		void Emitter(const ParticleProperties& props);
	private:
		struct Particle
		{
			lptm::Vector2D position, velocity;
			lptm::Vector4D color, colorBegin, colorEnd;

			float sizeBegin, sizeEnd;
			float rotation = 0.0f;

			float lifetime = 1.0f;
			float lifeRemaining = 1.0f;
				
			bool active = true;
		};

		std::vector<Particle> m_ParticlePool;
		int m_PoolIndex = 0;
	};

}