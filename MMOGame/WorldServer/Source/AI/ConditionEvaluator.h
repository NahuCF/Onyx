#pragma once

#include "../Scripting/IEntity.h"
#include "AIDefines.h"
#include <vector>

namespace MMO {

	// ============================================================
	// CONDITION EVALUATOR
	// ============================================================

	class ConditionEvaluator
	{
	public:
		static bool Evaluate(const Condition& cond, IEntity* self, IEntity* target,
							 float combatTime = 0.0f, bool hasSummons = false)
		{
			switch (cond.type)
			{
			case ConditionType::NONE:
				return true;

			case ConditionType::HEALTH_BELOW:
			{
				if (!self)
					return false;
				return self->GetHealthPercent() * 100.0f < cond.value;
			}

			case ConditionType::HEALTH_ABOVE:
			{
				if (!self)
					return false;
				return self->GetHealthPercent() * 100.0f > cond.value;
			}

			case ConditionType::MANA_BELOW:
			{
				if (!self)
					return false;
				return self->GetManaPercent() * 100.0f < cond.value;
			}

			case ConditionType::MANA_ABOVE:
			{
				if (!self)
					return false;
				return self->GetManaPercent() * 100.0f > cond.value;
			}

			case ConditionType::TARGET_HEALTH_BELOW:
			{
				if (!target)
					return false;
				return target->GetHealthPercent() * 100.0f < cond.value;
			}

			case ConditionType::TARGET_HEALTH_ABOVE:
			{
				if (!target)
					return false;
				return target->GetHealthPercent() * 100.0f > cond.value;
			}

			case ConditionType::IN_RANGE:
			{
				if (!self || !target)
					return false;
				Vec2 selfPos = self->GetPosition();
				Vec2 tgtPos = target->GetPosition();
				float dx = selfPos.x - tgtPos.x;
				float dy = selfPos.y - tgtPos.y;
				float dist = std::sqrt(dx * dx + dy * dy);
				return dist <= cond.value;
			}

			case ConditionType::OUT_OF_RANGE:
			{
				if (!self || !target)
					return false;
				Vec2 selfPos = self->GetPosition();
				Vec2 tgtPos = target->GetPosition();
				float dx = selfPos.x - tgtPos.x;
				float dy = selfPos.y - tgtPos.y;
				float dist = std::sqrt(dx * dx + dy * dy);
				return dist > cond.value;
			}

			case ConditionType::COMBAT_TIME_ABOVE:
				return combatTime > cond.value;

			case ConditionType::COMBAT_TIME_BELOW:
				return combatTime < cond.value;

			case ConditionType::SUMMONS_ALIVE:
				return hasSummons;

			case ConditionType::NO_SUMMONS_ALIVE:
				return !hasSummons;

			case ConditionType::HAS_BUFF:
				return false; // TODO: buff system

			case ConditionType::NOT_HAS_BUFF:
				return true; // TODO: buff system

			case ConditionType::RANDOM_CHANCE:
				return (rand() % 100) < static_cast<int>(cond.value);

			default:
				return true;
			}
		}

		static bool EvaluateAll(const std::vector<Condition>& conditions, IEntity* self, IEntity* target,
								float combatTime = 0.0f, bool hasSummons = false)
		{
			for (const auto& cond : conditions)
			{
				if (!Evaluate(cond, self, target, combatTime, hasSummons))
					return false;
			}
			return true;
		}
	};

} // namespace MMO
