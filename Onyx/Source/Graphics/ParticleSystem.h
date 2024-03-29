#pragma once

#include "Maths/Maths.h";
#include "Renderer2D.h";

namespace Onyx {

	struct ParticleProperties
	{
		Onyx::Vector2D position, velocity, velocityVariation;
		Onyx::Vector4D color, colorBegin, colorEnd;

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
		void Render(Onyx::Renderer2D* renderer);

		void Emit(const ParticleProperties& props);
		void Emitter(const ParticleProperties& props);
	private:
		struct Particle
		{
			Onyx::Vector2D position, velocity;
			Onyx::Vector4D color, colorBegin, colorEnd;

			float sizeBegin, sizeEnd;
			float rotation = 0.0f;

			float lifetime = 1.0f;
			float lifeRemaining = 1.0f;
				
			bool active = false;
		};

		std::vector<Particle> m_ParticlePool;
		int m_PoolIndex = 0;
	};

}