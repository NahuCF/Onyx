#pragma once

#include "../../../Shared/Source/Scripting/ScriptRegistry.h"
#include "../../../Shared/Source/Spells/SpellDefines.h"
#include "../AI/CreatureAI.h"
#include "../AI/CreatureScript.h"
#include <iostream>

namespace MMO {

	// ============================================================
	// SHADOW LORD AI
	// ============================================================
	//
	// Per-mob runtime: phase management + DAMAGE_IMMUNITY aura.
	//   Phase 1: normal combat
	//   Phase 2 (< 50% HP): summon 3 Shadow Servants, apply DAMAGE_IMMUNITY
	//   Phase 3 (all adds dead): remove immunity, enrage
	//
	// Uses IEntity& for all owner-side aura operations so this class
	// can be unit-tested with a MockEntity stub.
	// ============================================================

	class ShadowLordAI : public CreatureAI
	{
	public:
		ShadowLordAI(IMapContext& ctx, IEntity& owner, const CreatureTemplate* tmpl)
			: CreatureAI(ctx, owner, tmpl)
		{}

		// Custom event IDs (beyond template abilities)
		enum CustomEvents
		{
			EVENT_INVULNERABILITY_CHECK = 100,
			EVENT_ENRAGE_WARNING = 101,
		};

		void OnEnterCombat(IEntity& target) override
		{
			CreatureAI::OnEnterCombat(target);
			RemoveInvulnerabilityAura();
			std::cout << "[ShadowLord] Entering combat!\n";
		}

		void OnPhaseTransition(uint32_t oldPhase, uint32_t newPhase) override
		{
			CreatureAI::OnPhaseTransition(oldPhase, newPhase);

			if (newPhase == 2)
			{
				ApplyInvulnerabilityAura();
				std::cout << "[ShadowLord] Phase 2: INVULNERABLE until adds die!\n";
			}
			else if (newPhase == 3)
			{
				RemoveInvulnerabilityAura();
				std::cout << "[ShadowLord] Phase 3: ENRAGED! Attacks faster!\n";
			}
		}

		void OnSummonDied(IEntity& summon) override
		{
			std::cout << "[ShadowLord] A Shadow Servant has fallen! ("
					  << m_Summons.Count() - 1 << " remaining)\n";

			CreatureAI::OnSummonDied(summon);

			if (m_Summons.Count() == 0)
			{
				std::cout << "[ShadowLord] All adds defeated! Removing invulnerability!\n";
				RemoveInvulnerabilityAura();
			}
		}

		void OnEvade() override
		{
			CreatureAI::OnEvade();
			RemoveInvulnerabilityAura();
		}

		void OnDeath(IEntity* killer) override
		{
			CreatureAI::OnDeath(killer);
			RemoveInvulnerabilityAura();
		}

	private:
		void ApplyInvulnerabilityAura()
		{
			if (m_Owner.HasAuraType(AuraType::DAMAGE_IMMUNITY))
				return;

			Aura aura;
			aura.sourceAbility = AbilityId::NONE;
			aura.casterId = m_Owner.GetId();
			aura.type = AuraType::DAMAGE_IMMUNITY;
			aura.value = 0;
			aura.duration = 600.0f;
			aura.maxDuration = 600.0f;
			aura.tickInterval = 0.0f;

			m_InvulnerabilityAuraId = m_Owner.AddAura(aura);
			std::cout << "[ShadowLord] Applied DAMAGE_IMMUNITY aura (ID: "
					  << m_InvulnerabilityAuraId << ")\n";
		}

		void RemoveInvulnerabilityAura()
		{
			if (m_InvulnerabilityAuraId != 0)
			{
				m_Owner.RemoveAura(m_InvulnerabilityAuraId);
				std::cout << "[ShadowLord] Removed DAMAGE_IMMUNITY aura\n";
				m_InvulnerabilityAuraId = 0;
			}
			else
			{
				m_Owner.RemoveAurasByType(AuraType::DAMAGE_IMMUNITY);
			}
		}

		uint32_t m_InvulnerabilityAuraId = 0;
	};

	// ============================================================
	// SHADOW LORD SCRIPT — registered factory singleton
	// ============================================================

	class ShadowLordScript : public CreatureScript
	{
	public:
		ShadowLordScript() : CreatureScript("shadow_lord") {}

		std::unique_ptr<CreatureAI> CreateAI(IMapContext& map, IEntity& mob,
		                                     const CreatureTemplate* tmpl) const override
		{
			return std::make_unique<ShadowLordAI>(map, mob, tmpl);
		}
	};

} // namespace MMO
