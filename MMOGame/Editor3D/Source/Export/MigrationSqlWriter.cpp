#include "MigrationSqlWriter.h"

#include "../../../Shared/Source/World/SpawnPoint.h"
#include "../../../Shared/Source/World/PlayerSpawn.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <ostream>
#include <set>
#include <sstream>
#include <string>

namespace MMO {

namespace {

// Postgres single-quote escaping (E'…' literal style is overkill here — we
// only need to handle quotes appearing in user-supplied strings).
std::string Quote(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('\'');
    for (char c : s) {
        if (c == '\'') out.push_back('\'');
        out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

// SpawnPoint stores rotation as a quaternion; the DB stores a single yaw angle
// in radians. Extract the Y-axis Euler component.
float ExtractYaw(const glm::quat& q) {
    // yaw = atan2(2*(w*y + x*z), 1 - 2*(y*y + z*z))
    return std::atan2(2.0f * (q.w * q.y + q.x * q.z),
                      1.0f - 2.0f * (q.y * q.y + q.z * q.z));
}

// Editor GUIDs are uint64; render as a stable text string for the TEXT primary
// key column.
std::string GuidToText(uint64_t guid) {
    return std::to_string(guid);
}

} // namespace

void MigrationSqlWriter::EmitCreatureSpawnsForMap(
    uint32_t mapId,
    const std::vector<const SpawnPoint*>& spawns,
    std::ostream& out) {

    out << "-- creature_spawn for map_id=" << mapId << "\n";
    out << "BEGIN;\n";

    if (!spawns.empty()) {
        out << "INSERT INTO creature_spawn "
               "(guid, map_id, entry, position_x, position_y, position_z, "
               "orientation, respawn_secs, wander_radius, max_count) VALUES\n";

        for (size_t i = 0; i < spawns.size(); ++i) {
            const SpawnPoint* s = spawns[i];
            const glm::vec3 pos = s->GetPosition();
            const float yaw = ExtractYaw(s->GetRotation());

            // Axis swap: editor is Y-up (X/Z = ground plane, Y = vertical),
            // server is Z-up (X/Y = ground plane, Z = vertical).
            out << "  ("
                << Quote(GuidToText(s->GetGuid())) << ", "
                << mapId << ", "
                << s->GetCreatureTemplateId() << ", "
                << pos.x << ", " << pos.z << ", " << pos.y << ", "
                << yaw << ", "
                << s->GetRespawnTime() << ", "
                << s->GetWanderRadius() << ", "
                << s->GetMaxCount() << ")"
                << (i + 1 == spawns.size() ? "\n" : ",\n");
        }

        out << "ON CONFLICT (guid) DO UPDATE SET "
               "map_id = EXCLUDED.map_id, "
               "entry = EXCLUDED.entry, "
               "position_x = EXCLUDED.position_x, "
               "position_y = EXCLUDED.position_y, "
               "position_z = EXCLUDED.position_z, "
               "orientation = EXCLUDED.orientation, "
               "respawn_secs = EXCLUDED.respawn_secs, "
               "wander_radius = EXCLUDED.wander_radius, "
               "max_count = EXCLUDED.max_count;\n";
    }

    // Scoped delete: remove rows for this map whose guid is no longer authored.
    out << "DELETE FROM creature_spawn WHERE map_id = " << mapId;
    if (!spawns.empty()) {
        out << " AND guid NOT IN (";
        for (size_t i = 0; i < spawns.size(); ++i) {
            out << Quote(GuidToText(spawns[i]->GetGuid()))
                << (i + 1 == spawns.size() ? "" : ", ");
        }
        out << ")";
    }
    out << ";\n";

    out << "COMMIT;\n";
}

void MigrationSqlWriter::EmitPlayerCreateInfo(
    const std::vector<const PlayerSpawn*>& spawns,
    uint32_t mapId,
    std::ostream& out) {

    out << "-- player_create_info (global; emitted from map_id=" << mapId << ")\n";
    out << "BEGIN;\n";

    // Collect (race, class) tuples with their authoring data.
    struct Row {
        int race;
        int cls;
        float x, y, z;
        float orientation;
        uint32_t mapId;
    };
    std::vector<Row> rows;
    std::set<std::pair<int, int>> seen;     // suppress duplicates within this export

    for (const PlayerSpawn* spawn : spawns) {
        if (!spawn) continue;
        const glm::vec3 pos = spawn->GetPosition();
        const float orientation = spawn->GetOrientation();
        for (const auto& pair : spawn->GetRaceClassPairs()) {
            const int race = static_cast<int>(pair.first);
            const int cls  = static_cast<int>(pair.second);
            if (race <= 0 || cls <= 0) continue;
            auto key = std::make_pair(race, cls);
            if (seen.count(key)) continue;     // first writer wins; conflict is surfaced in Inspector
            seen.insert(key);
            // Axis swap: editor Y-up → server Z-up.
            rows.push_back({race, cls, pos.x, pos.z, pos.y, orientation, mapId});
        }
    }

    if (!rows.empty()) {
        out << "INSERT INTO player_create_info "
               "(race, class, map_id, position_x, position_y, position_z, orientation) VALUES\n";
        for (size_t i = 0; i < rows.size(); ++i) {
            const Row& r = rows[i];
            out << "  ("
                << r.race << ", " << r.cls << ", " << r.mapId << ", "
                << r.x << ", " << r.y << ", " << r.z << ", "
                << r.orientation << ")"
                << (i + 1 == rows.size() ? "\n" : ",\n");
        }
        out << "ON CONFLICT (race, class) DO UPDATE SET "
               "map_id = EXCLUDED.map_id, "
               "position_x = EXCLUDED.position_x, "
               "position_y = EXCLUDED.position_y, "
               "position_z = EXCLUDED.position_z, "
               "orientation = EXCLUDED.orientation;\n";
    }

    // Global delete: any (race, class) row not in the emitted set.
    out << "DELETE FROM player_create_info";
    if (!rows.empty()) {
        out << " WHERE (race, class) NOT IN (";
        for (size_t i = 0; i < rows.size(); ++i) {
            out << "(" << rows[i].race << ", " << rows[i].cls << ")"
                << (i + 1 == rows.size() ? "" : ", ");
        }
        out << ")";
    }
    out << ";\n";

    out << "COMMIT;\n";
}

} // namespace MMO
