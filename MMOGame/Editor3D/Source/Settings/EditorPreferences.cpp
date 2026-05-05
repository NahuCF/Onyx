#include "EditorPreferences.h"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>

namespace MMO {

	EditorPreferences& EditorPreferences::Instance()
	{
		static EditorPreferences instance;
		return instance;
	}

	std::filesystem::path EditorPreferences::PreferencesPath()
	{
#ifdef _WIN32
		if (const char* appdata = std::getenv("APPDATA"))
		{
			return std::filesystem::path(appdata) / "Onyx" / "Editor3D" / "preferences.json";
		}
		return std::filesystem::path("preferences.json");
#else
		if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg)
		{
			return std::filesystem::path(xdg) / "onyx" / "editor3d" / "preferences.json";
		}
		if (const char* home = std::getenv("HOME"); home && *home)
		{
			return std::filesystem::path(home) / ".config" / "onyx" / "editor3d" / "preferences.json";
		}
		return std::filesystem::path("preferences.json");
#endif
	}

	void EditorPreferences::Load()
	{
		const auto path = PreferencesPath();
		std::ifstream in(path);
		if (!in.is_open())
		{
			// No file yet → keep defaults.
			return;
		}

		try
		{
			nlohmann::json j;
			in >> j;

			m_RecentMaps.clear();
			if (j.contains("recent_maps") && j["recent_maps"].is_array())
			{
				for (const auto& v : j["recent_maps"])
				{
					if (v.is_number_integer())
					{
						m_RecentMaps.push_back(v.get<uint32_t>());
						if (m_RecentMaps.size() >= kRecentMapsMax)
							break;
					}
				}
			}

			if (j.contains("autosave_enabled") && j["autosave_enabled"].is_boolean())
			{
				m_AutosaveEnabled = j["autosave_enabled"].get<bool>();
			}
			if (j.contains("autosave_interval_secs") && j["autosave_interval_secs"].is_number())
			{
				m_AutosaveIntervalSecs = j["autosave_interval_secs"].get<float>();
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "[EditorPreferences] failed to parse "
					  << path.string() << ": " << e.what() << std::endl;
		}
	}

	void EditorPreferences::Save()
	{
		const auto path = PreferencesPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		nlohmann::json j;
		j["recent_maps"] = m_RecentMaps;
		j["autosave_enabled"] = m_AutosaveEnabled;
		j["autosave_interval_secs"] = m_AutosaveIntervalSecs;

		std::ofstream out(path);
		if (!out.is_open())
		{
			std::cerr << "[EditorPreferences] failed to open "
					  << path.string() << " for writing" << std::endl;
			return;
		}
		out << j.dump(2);
	}

	void EditorPreferences::PushRecentMap(uint32_t mapId)
	{
		// Remove existing instance, then push to front.
		for (auto it = m_RecentMaps.begin(); it != m_RecentMaps.end();)
		{
			it = (*it == mapId) ? m_RecentMaps.erase(it) : it + 1;
		}
		m_RecentMaps.push_front(mapId);
		while (m_RecentMaps.size() > kRecentMapsMax)
		{
			m_RecentMaps.pop_back();
		}
		Save();
	}

} // namespace MMO
