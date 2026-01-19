#pragma once

#include "AIDefines.h"
#include "../Entity/Entity.h"
#include <vector>

namespace MMO {

// ============================================================
// CONDITION EVALUATOR
// ============================================================

class ConditionEvaluator {
public:
    static bool Evaluate(const Condition& cond, Entity* self, Entity* target,
                         float combatTime = 0.0f, bool hasSummons = false) {
        switch (cond.type) {
            case ConditionType::NONE:
                return true;

            case ConditionType::HEALTH_BELOW: {
                auto health = self->GetHealth();
                if (!health) return false;
                float pct = health->Percent() * 100.0f;
                return pct < cond.value;
            }

            case ConditionType::HEALTH_ABOVE: {
                auto health = self->GetHealth();
                if (!health) return false;
                float pct = health->Percent() * 100.0f;
                return pct > cond.value;
            }

            case ConditionType::MANA_BELOW: {
                auto mana = self->GetMana();
                if (!mana) return false;
                float pct = mana->Percent() * 100.0f;
                return pct < cond.value;
            }

            case ConditionType::MANA_ABOVE: {
                auto mana = self->GetMana();
                if (!mana) return false;
                float pct = mana->Percent() * 100.0f;
                return pct > cond.value;
            }

            case ConditionType::TARGET_HEALTH_BELOW: {
                if (!target) return false;
                auto health = target->GetHealth();
                if (!health) return false;
                float pct = health->Percent() * 100.0f;
                return pct < cond.value;
            }

            case ConditionType::TARGET_HEALTH_ABOVE: {
                if (!target) return false;
                auto health = target->GetHealth();
                if (!health) return false;
                float pct = health->Percent() * 100.0f;
                return pct > cond.value;
            }

            case ConditionType::IN_RANGE: {
                if (!target) return false;
                auto selfMove = self->GetMovement();
                auto targetMove = target->GetMovement();
                if (!selfMove || !targetMove) return false;
                float dist = Vec2::Distance(selfMove->position, targetMove->position);
                return dist <= cond.value;
            }

            case ConditionType::OUT_OF_RANGE: {
                if (!target) return false;
                auto selfMove = self->GetMovement();
                auto targetMove = target->GetMovement();
                if (!selfMove || !targetMove) return false;
                float dist = Vec2::Distance(selfMove->position, targetMove->position);
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
                // TODO: Implement buff system
                return false;

            case ConditionType::NOT_HAS_BUFF:
                // TODO: Implement buff system
                return true;

            case ConditionType::RANDOM_CHANCE:
                return (rand() % 100) < static_cast<int>(cond.value);

            default:
                return true;
        }
    }

    static bool EvaluateAll(const std::vector<Condition>& conditions, Entity* self, Entity* target,
                            float combatTime = 0.0f, bool hasSummons = false) {
        for (const auto& cond : conditions) {
            if (!Evaluate(cond, self, target, combatTime, hasSummons)) {
                return false;
            }
        }
        return true;
    }
};

} // namespace MMO
