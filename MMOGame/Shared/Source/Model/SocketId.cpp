#include "SocketId.h"

#include <string>
#include <unordered_map>

namespace MMO {

	namespace {

		struct NameEntry
		{
			SocketId id;
			const char* name;
		};

		const NameEntry kSocketNames[] = {
			{SocketId::OverheadName,    "OverheadName"},
			{SocketId::OverheadMounted, "OverheadMounted"},
			{SocketId::Chest,           "Chest"},
			{SocketId::Head,            "Head"},
			{SocketId::WeaponR,         "WeaponR"},
			{SocketId::WeaponL,         "WeaponL"},
			{SocketId::MuzzleR,         "MuzzleR"},
			{SocketId::MuzzleL,         "MuzzleL"},
			{SocketId::ShieldL,         "ShieldL"},
			{SocketId::SheathBack,      "SheathBack"},
			{SocketId::SheathHipL,      "SheathHipL"},
			{SocketId::SheathHipR,      "SheathHipR"},
			{SocketId::HandR,           "HandR"},
			{SocketId::HandL,           "HandL"},
			{SocketId::SpellOrigin,     "SpellOrigin"},
			{SocketId::MountSeat,       "MountSeat"},
			{SocketId::MountReinsL,     "MountReinsL"},
			{SocketId::MountReinsR,     "MountReinsR"},
			{SocketId::EquipHelm,       "EquipHelm"},
			{SocketId::EquipShoulderL,  "EquipShoulderL"},
			{SocketId::EquipShoulderR,  "EquipShoulderR"},
			{SocketId::EquipBackpack,   "EquipBackpack"},
			{SocketId::EquipCape,       "EquipCape"},
			{SocketId::EquipBeltFront,  "EquipBeltFront"},
			{SocketId::EquipBeltBack,   "EquipBeltBack"},
			{SocketId::EquipQuiver,     "EquipQuiver"},
		};

		const std::unordered_map<std::string, SocketId>& NameToIdMap()
		{
			static const std::unordered_map<std::string, SocketId> map = []() {
				std::unordered_map<std::string, SocketId> m;
				m.reserve(sizeof(kSocketNames) / sizeof(kSocketNames[0]));
				for (const auto& entry : kSocketNames)
				{
					m.emplace(entry.name, entry.id);
				}
				return m;
			}();
			return map;
		}

	} // namespace

	SocketId SocketNameToId(std::string_view name)
	{
		const auto& m = NameToIdMap();
		auto it = m.find(std::string(name));
		return it != m.end() ? it->second : SocketId::Custom;
	}

	const char* SocketIdToName(SocketId id)
	{
		for (const auto& entry : kSocketNames)
		{
			if (entry.id == id)
				return entry.name;
		}
		return nullptr;
	}

} // namespace MMO
