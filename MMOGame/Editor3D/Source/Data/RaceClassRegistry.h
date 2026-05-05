#pragma once

#include "../../../Shared/Source/Types/Types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace MMO {

	// RaceClassRegistry — editor-side catalog of races, classes, and valid combos.
	//
	// IDs match MMOGame/Database/migrations/0003_seed_races_classes.sql exactly.
	// If the seed migration changes, update this file in lockstep.
	//
	// class_mask bit positions: bit (classId - 1).
	//   bit 0 = Warrior (id 1), bit 1 = Witch (id 2),
	//   bit 2 = Mage (id 3),    bit 3 = Rogue (id 4).

	struct RaceDef
	{
		CharacterRace id;
		std::string name;
		uint32_t classMask; // bitmask of valid CharacterClass values
	};

	struct ClassDef
	{
		CharacterClass id;
		std::string name;
	};

	struct RaceClassCombo
	{
		CharacterRace raceId;
		CharacterClass classId;

		std::string DisplayName() const;
	};

	class RaceClassRegistry
	{
	public:
		static const std::vector<RaceDef>& Races();
		static const std::vector<ClassDef>& Classes();
		static const std::vector<RaceClassCombo>& Combos();

		static const RaceDef* FindRace(CharacterRace id);
		static const ClassDef* FindClass(CharacterClass id);
	};

} // namespace MMO
