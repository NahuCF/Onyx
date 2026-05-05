#pragma once

#include "../../../Shared/Source/Spells/SpellDefines.h"
#include "../../../Shared/Source/Types/Types.h"
#include <string>

namespace MMO {

	// ============================================================
	// IENTITY — minimal interface for what scripts read / mutate.
	//
	// Entity implements this. Test stubs (MockEntity) also implement
	// it, enabling unit tests without spinning up a MapInstance.
	//
	// Rule: only add methods scripts ACTUALLY call. Resist the urge
	// to pre-expose every getter. Adding later is cheap; removing
	// breaks scripts.
	// ============================================================

	class IEntity
	{
	public:
		virtual ~IEntity() = default;

		// Identity
		virtual EntityId GetId() const = 0;
		virtual const std::string& GetName() const = 0;
		virtual EntityType GetType() const = 0;

		// Position
		virtual Vec2 GetPosition() const = 0;
		virtual float GetHeight() const = 0;

		// Vitals (normalised 0–1 fractions, no raw component access)
		virtual float GetHealthPercent() const = 0;
		virtual float GetManaPercent() const = 0;
		virtual bool IsDead() const = 0;

		// Auras — delegates to AuraComponent on the concrete Entity
		virtual uint32_t AddAura(const Aura& aura) = 0;
		virtual void RemoveAura(uint32_t auraId) = 0;
		virtual void RemoveAurasByType(AuraType type) = 0;
		virtual bool HasAuraType(AuraType type) const = 0;
	};

} // namespace MMO
