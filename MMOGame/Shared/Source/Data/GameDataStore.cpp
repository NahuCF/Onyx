#include "GameDataStore.h"

#ifdef HAS_DATABASE
#include "../Database/Database.h"
#endif

#include <iostream>

namespace MMO {

#ifdef HAS_DATABASE
	void GameDataStore::LoadFromDatabase(Database& db)
	{
		// Load race templates
		auto races = db.LoadRaceTemplates();
		for (auto& r : races)
		{
			m_Races[static_cast<uint8_t>(r.id)] = r;
		}
		m_RaceList = std::move(races);

		// Load class templates
		auto classes = db.LoadClassTemplates();
		for (auto& c : classes)
		{
			m_Classes[static_cast<uint8_t>(c.id)] = c;
		}
		m_ClassList = std::move(classes);

		// Load player create info
		auto createInfos = db.LoadPlayerCreateInfo();
		for (auto& row : createInfos)
		{
			m_CreateInfo[MakeKey(row.race, row.cls)] = row.info;
		}

		// Load map names
		auto mapTemplates = db.LoadAllMapTemplates();
		for (auto& t : mapTemplates)
		{
			m_MapNames[t.id] = t.name;
		}

		std::cout << "[GameDataStore] Loaded " << m_Races.size() << " races, "
				  << m_Classes.size() << " classes, "
				  << m_CreateInfo.size() << " create infos, "
				  << m_MapNames.size() << " map names" << '\n';
	}
#endif

	const RaceTemplate* GameDataStore::GetRaceTemplate(CharacterRace race) const
	{
		auto it = m_Races.find(static_cast<uint8_t>(race));
		return it != m_Races.end() ? &it->second : nullptr;
	}

	const ClassTemplate* GameDataStore::GetClassTemplate(CharacterClass cls) const
	{
		auto it = m_Classes.find(static_cast<uint8_t>(cls));
		return it != m_Classes.end() ? &it->second : nullptr;
	}

	const PlayerCreateInfo* GameDataStore::GetPlayerCreateInfo(CharacterRace race, CharacterClass cls) const
	{
		auto it = m_CreateInfo.find(MakeKey(static_cast<uint8_t>(race), static_cast<uint8_t>(cls)));
		return it != m_CreateInfo.end() ? &it->second : nullptr;
	}

	const std::string& GameDataStore::GetMapName(uint32_t mapId) const
	{
		static const std::string unknown = "Unknown";
		auto it = m_MapNames.find(mapId);
		return it != m_MapNames.end() ? it->second : unknown;
	}

	bool GameDataStore::IsValidRaceClass(CharacterRace race, CharacterClass cls) const
	{
		const RaceTemplate* raceTemplate = GetRaceTemplate(race);
		if (!raceTemplate)
			return false;

		uint8_t classBit = static_cast<uint8_t>(cls);
		if (classBit == 0)
			return false;

		// classMask is a bitmask where bit N means class N+1 is allowed... no, bit0=class1, bit1=class2
		// classMask & (1 << (classId - 1))
		return (raceTemplate->classMask & (1u << (classBit - 1))) != 0;
	}

} // namespace MMO
