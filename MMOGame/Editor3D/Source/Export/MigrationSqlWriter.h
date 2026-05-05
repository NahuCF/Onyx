#pragma once

#include <cstdint>
#include <iosfwd>
#include <vector>

namespace MMO {

	class SpawnPoint;
	class PlayerSpawn;

	// MigrationSqlWriter — generates idempotent UPSERT/DELETE SQL for DB-bound
	// authored entities. The output is consumed two ways:
	//   1. Mode A "Run Locally" — applied directly to the designer's local Postgres.
	//   2. Mode B "Build Release Bundle" — shipped inside the release artifact for
	//      production import.
	//
	// Each entity type is wrapped in its own BEGIN/COMMIT transaction so a partial
	// failure in (say) creature_spawn doesn't leave player_create_info in an
	// inconsistent state.
	class MigrationSqlWriter
	{
	public:
		// creature_spawn UPSERT for the given map + scoped DELETE removing any row
		// whose guid is no longer authored in this map.
		static void EmitCreatureSpawnsForMap(
			uint32_t mapId,
			const std::vector<const SpawnPoint*>& spawns,
			std::ostream& out);

		// player_create_info UPSERT for every (race, class) pair on every passed
		// PlayerSpawn + global DELETE removing any (race, class) row not in the
		// emitted set. Note: player_create_info is global (not per-map); callers
		// must pass every PlayerSpawn across every authored map in one call.
		static void EmitPlayerCreateInfo(
			const std::vector<const PlayerSpawn*>& spawns,
			uint32_t mapId,
			std::ostream& out);
	};

} // namespace MMO
