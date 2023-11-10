#include "pch.h"

#include "ParticleSystem.h";

namespace Onyx {

	ParticleSystem::ParticleSystem(int particleCount)
	{
		m_ParticlePool.resize(particleCount);

	}

	ParticleSystem::~ParticleSystem()
	{
	}

	void ParticleSystem::Update(float timestamp)
	{
		for (auto& particle : m_ParticlePool)
		{
			if (!particle.active)
				continue;

			if (particle.lifeRemaining <= 0.0f)
			{
				particle.active = false;
				continue;
			}

			particle.position += {
				particle.velocity.x * timestamp,
				particle.velocity.y * timestamp
			};
			particle.lifeRemaining -= timestamp;
		}
	}

	void ParticleSystem::Render(Onyx::Renderer2D* renderer)
	{
		for (auto& particle : m_ParticlePool)
		{
			if (!particle.active)
				continue;

			float life = particle.lifeRemaining / particle.lifetime;

			particle.color = Onyx::lerp4D(particle.colorEnd, particle.colorBegin, life);
			float size = Onyx::lerp(particle.sizeEnd, particle.sizeBegin, life);

			renderer->RenderRotatedQuad(
				{ size, size },
				{ particle.position.x, particle.position.y, 0.0f }, 
				particle.color,
				particle.rotation
			);
		}
	}

	void ParticleSystem::Emitter(const ParticleProperties& props)
	{
		/*for (auto& particle : m_ParticlePool)
		{
			particle.velocity = props.velocity;
			particle.size = props.size;
			particle.color = props.color;

			if (particle.lifeRemaining < 0.0f)
			{
				particle.position = props.position;
			}
		}*/
	}

	void ParticleSystem::Emit(const ParticleProperties& props)
	{
		Particle& particle = m_ParticlePool[m_PoolIndex];

		particle.active = true;
		particle.lifeRemaining = props.lifetime;
		particle.lifetime = props.lifetime;

		particle.position = props.position;
		particle.rotation = props.rotation;

		particle.velocity = props.velocity;
		particle.velocity.x += props.velocityVariation.x * Onyx::ramdomInRange(-0.5f, 0.5f);
		particle.velocity.y += props.velocityVariation.y * Onyx::ramdomInRange(-0.5f, 0.5f);

		particle.sizeBegin = props.sizeBegin;
		//particle.sizeBegin += props.sizeVariation * Onyx::ramdomInRange(0.0f, 0.5f);
		particle.sizeEnd = props.sizeEnd;

		particle.color = props.color;
		particle.colorBegin = props.colorBegin;
		particle.colorEnd = props.colorEnd;

		if (m_PoolIndex == m_ParticlePool.size() - 1)
		{
			m_PoolIndex = 0;
			return;
		}

		m_PoolIndex++;
	}

};