#pragma once

#include "../Types/Types.h"
#include "WorldObject.h"
#include <algorithm>
#include <utility>
#include <vector>

namespace MMO {

	class PlayerSpawn : public WorldObject
	{
	public:
		PlayerSpawn(uint64_t guid, const std::string& name = "")
			: WorldObject(guid, WorldObjectType::PLAYER_SPAWN, name) {}

		// Race+Class assignments
		using RaceClassPair = std::pair<CharacterRace, CharacterClass>;

		void AddRaceClass(CharacterRace race, CharacterClass cls)
		{
			// Don't add duplicates
			for (const auto& pair : m_RaceClassPairs)
			{
				if (pair.first == race && pair.second == cls)
					return;
			}
			m_RaceClassPairs.push_back({race, cls});
		}

		void RemoveRaceClass(CharacterRace race, CharacterClass cls)
		{
			m_RaceClassPairs.erase(
				std::remove_if(m_RaceClassPairs.begin(), m_RaceClassPairs.end(),
							   [race, cls](const RaceClassPair& p) {
								   return p.first == race && p.second == cls;
							   }),
				m_RaceClassPairs.end());
		}

		bool HasRaceClass(CharacterRace race, CharacterClass cls) const
		{
			for (const auto& pair : m_RaceClassPairs)
			{
				if (pair.first == race && pair.second == cls)
					return true;
			}
			return false;
		}

		const std::vector<RaceClassPair>& GetRaceClassPairs() const { return m_RaceClassPairs; }

		// Orientation (facing direction in radians)
		void SetOrientation(float radians) { m_Orientation = radians; }
		float GetOrientation() const { return m_Orientation; }

		const char* GetTypeName() const override { return "Player Spawn"; }

		std::unique_ptr<WorldObject> Clone(uint64_t newGuid) const override
		{
			auto copy = std::make_unique<PlayerSpawn>(newGuid, GetName() + " Copy");
			copy->SetPosition(GetPosition());
			copy->SetRotation(GetRotation());
			copy->SetScale(GetScale());
			copy->m_RaceClassPairs = m_RaceClassPairs;
			copy->m_Orientation = m_Orientation;
			return copy;
		}

	private:
		std::vector<RaceClassPair> m_RaceClassPairs;
		float m_Orientation = 0.0f;
	};

} // namespace MMO
