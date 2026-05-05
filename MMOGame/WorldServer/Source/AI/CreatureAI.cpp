#include "CreatureAI.h"
#include "ConditionEvaluator.h"
#include <iostream>

namespace MMO {

	// ============================================================
	// EVENT HANDLERS
	// ============================================================

	void CreatureAI::OnEnterCombat(IEntity& target)
	{
		m_InCombat = true;
		m_CombatTime = 0.0f;
		m_Events.Reset();
		if (m_Template)
		{
			m_Events.SetPhase(m_Template->initialPhase);
			m_CurrentPhase = m_Template->initialPhase;

			// Schedule all template abilities with initial delays
			for (size_t i = 0; i < m_Template->abilities.size(); i++)
			{
				const auto& ability = m_Template->abilities[i];
				float delay = ability.initialDelay > 0
								  ? ability.initialDelay
								  : ability.cooldown * 0.3f;
				m_Events.ScheduleEvent(static_cast<uint32_t>(i), delay, ability.phaseMask);
			}
		}
		m_Summons.Clear();
		m_TriggeredPhases.clear();
	}

	void CreatureAI::OnEvade()
	{
		m_InCombat = false;
		m_Events.Reset();
		m_Summons.Clear();
		m_TriggeredPhases.clear();
	}

	void CreatureAI::OnDeath(IEntity* killer)
	{
		m_InCombat = false;
		m_Events.Reset();
	}

	void CreatureAI::OnDamageTaken(IEntity& attacker, int32_t damage)
	{
		// Default: no-op. Override for custom on-hit reactions.
	}

	void CreatureAI::OnSummonDied(IEntity& summon)
	{
		m_Summons.Remove(summon.GetId());

		if (!m_Template)
			return;

		// Check if we should transition phase due to all summons dead
		for (const auto& trigger : m_Template->phaseTriggers)
		{
			if (trigger.type == PhaseTriggerType::SUMMONS_DEAD &&
				m_CurrentPhase == trigger.fromPhase &&
				m_Summons.IsEmpty())
			{
				TransitionToPhase(trigger.toPhase, trigger);
			}
		}
	}

	void CreatureAI::OnPhaseTransition(uint32_t oldPhase, uint32_t newPhase)
	{
		std::cout << "[AI] " << m_Owner.GetName() << " transitioned from phase "
				  << oldPhase << " to phase " << newPhase << '\n';
	}

	// ============================================================
	// MAIN UPDATE
	// ============================================================

	void CreatureAI::Update(float dt, IEntity* target, IMapContext& ctx)
	{
		if (!m_InCombat || !target || !m_Template)
			return;

		m_CombatTime += dt;
		m_Events.Update(dt);

		// Check HP/time/summon-based phase triggers
		CheckPhaseTriggers();

		// Execute scheduled abilities
		while (uint32_t rawEventId = m_Events.ExecuteEvent())
		{
			uint32_t eventId = rawEventId - 1;
			if (eventId >= m_Template->abilities.size())
				continue;

			const auto& rule = m_Template->abilities[eventId];

			bool hasSummons = !m_Summons.IsEmpty();
			if (ConditionEvaluator::EvaluateAll(rule.conditions, &m_Owner, target,
												m_CombatTime, hasSummons))
			{
				ctx.ProcessAbility(m_Owner.GetId(), target->GetId(), rule.ability);
			}

			m_Events.ScheduleEvent(rawEventId, rule.cooldown, rule.phaseMask);
		}
	}

	// ============================================================
	// PHASE MANAGEMENT
	// ============================================================

	void CreatureAI::SetPhase(uint32_t phase)
	{
		uint32_t oldPhase = m_CurrentPhase;
		m_CurrentPhase = phase;
		m_Events.SetPhase(phase);
		OnPhaseTransition(oldPhase, phase);
	}

	void CreatureAI::CheckPhaseTriggers()
	{
		if (!m_Template)
			return;

		for (const auto& trigger : m_Template->phaseTriggers)
		{
			if (m_CurrentPhase != trigger.fromPhase)
				continue;

			uint64_t triggerKey = (static_cast<uint64_t>(trigger.fromPhase) << 32) | trigger.toPhase;
			if (m_TriggeredPhases.count(triggerKey))
				continue;

			bool shouldTransition = false;

			switch (trigger.type)
			{
			case PhaseTriggerType::HEALTH_BELOW:
				if (m_Owner.GetHealthPercent() * 100.0f < trigger.value)
					shouldTransition = true;
				break;
			case PhaseTriggerType::HEALTH_ABOVE:
				if (m_Owner.GetHealthPercent() * 100.0f > trigger.value)
					shouldTransition = true;
				break;
			case PhaseTriggerType::COMBAT_TIME:
				if (m_CombatTime >= trigger.value)
					shouldTransition = true;
				break;
			case PhaseTriggerType::SUMMONS_DEAD:
				if (m_Summons.IsEmpty())
					shouldTransition = true;
				break;
			case PhaseTriggerType::MANUAL:
				break;
			}

			if (shouldTransition)
			{
				m_TriggeredPhases.insert(triggerKey);
				TransitionToPhase(trigger.toPhase, trigger);
			}
		}
	}

	void CreatureAI::TransitionToPhase(uint32_t newPhase, const PhaseTrigger& trigger)
	{
		if (trigger.castOnTransition != AbilityId::NONE)
		{
			m_Ctx.ProcessAbility(m_Owner.GetId(), m_Owner.GetId(), trigger.castOnTransition);
		}

		if (trigger.summonCreatureId > 0 && trigger.summonCount > 0)
		{
			Vec2 basePos = m_Owner.GetPosition();
			float totalWidth = (trigger.summonCount - 1) * trigger.summonSpacing;
			float startX = basePos.x - totalWidth / 2.0f;

			for (uint8_t i = 0; i < trigger.summonCount; i++)
			{
				Vec2 spawnPos = {
					startX + i * trigger.summonSpacing,
					basePos.y + 2.0f
				};
				IEntity* summon = m_Ctx.SummonCreature(trigger.summonCreatureId, spawnPos, m_Owner.GetId());
				if (summon)
				{
					m_Summons.Add(summon->GetId());
				}
			}
		}

		SetPhase(newPhase);
	}

	void CreatureAI::TriggerPhase(uint32_t newPhase)
	{
		if (!m_Template)
		{
			SetPhase(newPhase);
			return;
		}
		for (const auto& trigger : m_Template->phaseTriggers)
		{
			if (trigger.fromPhase == m_CurrentPhase && trigger.toPhase == newPhase)
			{
				TransitionToPhase(newPhase, trigger);
				return;
			}
		}
		SetPhase(newPhase);
	}

} // namespace MMO
