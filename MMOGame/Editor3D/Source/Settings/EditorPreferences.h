#pragma once

#include <deque>
#include <filesystem>
#include <string>

namespace MMO {

// EditorPreferences — persistent user-scoped settings for the editor.
// Stored as JSON at:
//   Linux:   ${XDG_CONFIG_HOME:-$HOME/.config}/onyx/editor3d/preferences.json
//   Windows: %APPDATA%/Onyx/Editor3D/preferences.json
//
// Singleton; load on startup, save on change.
class EditorPreferences {
public:
    static EditorPreferences& Instance();

    // Recent maps — most-recent-first, capped at kRecentMapsMax.
    static constexpr size_t kRecentMapsMax = 8;
    const std::deque<uint32_t>& RecentMaps() const { return m_RecentMaps; }
    void PushRecentMap(uint32_t mapId);

    // Auto-save settings — read by Editor3DLayer.
    bool  AutosaveEnabled() const  { return m_AutosaveEnabled; }
    void  SetAutosaveEnabled(bool v) { m_AutosaveEnabled = v; Save(); }
    float AutosaveIntervalSecs() const  { return m_AutosaveIntervalSecs; }
    void  SetAutosaveIntervalSecs(float s) { m_AutosaveIntervalSecs = s; Save(); }

    // Persistence.
    void Load();   // safe to call multiple times; tolerates missing/corrupt file
    void Save();

    static std::filesystem::path PreferencesPath();

private:
    EditorPreferences() = default;

    std::deque<uint32_t> m_RecentMaps;
    bool                 m_AutosaveEnabled = true;
    float                m_AutosaveIntervalSecs = 300.0f;
};

} // namespace MMO
