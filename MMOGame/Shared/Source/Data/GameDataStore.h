#pragma once

#include "../Types/Types.h"
#include <unordered_map>
#include <vector>

#ifdef HAS_DATABASE
namespace MMO {
	class Database;
}
#endif

namespace MMO {

	class GameDataStore
	{
	public:
		static GameDataStore& Instance()
		{
			static GameDataStore instance;
			return instance;
		}

#ifdef HAS_DATABASE
		void LoadFromDatabase(Database& db);
#endif

		const RaceTemplate* GetRaceTemplate(CharacterRace race) const;
		const ClassTemplate* GetClassTemplate(CharacterClass cls) const;
		const PlayerCreateInfo* GetPlayerCreateInfo(CharacterRace race, CharacterClass cls) const;
		bool IsValidRaceClass(CharacterRace race, CharacterClass cls) const;
		const std::vector<RaceTemplate>& GetAllRaces() const { return m_RaceList; }
		const std::vector<ClassTemplate>& GetAllClasses() const { return m_ClassList; }
		const std::string& GetMapName(uint32_t mapId) const;

	private:
		GameDataStore() = default;
		~GameDataStore() = default;
		GameDataStore(const GameDataStore&) = delete;
		GameDataStore& operator=(const GameDataStore&) = delete;

		static uint16_t MakeKey(uint8_t race, uint8_t cls) { return (static_cast<uint16_t>(race) << 8) | cls; }

		std::unordered_map<uint8_t, RaceTemplate> m_Races;
		std::unordered_map<uint8_t, ClassTemplate> m_Classes;
		std::unordered_map<uint16_t, PlayerCreateInfo> m_CreateInfo;

		std::vector<RaceTemplate> m_RaceList;
		std::vector<ClassTemplate> m_ClassList;
		std::unordered_map<uint32_t, std::string> m_MapNames;
	};

} // namespace MMO
