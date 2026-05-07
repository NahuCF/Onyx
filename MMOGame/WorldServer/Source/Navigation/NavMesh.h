#pragma once

#include "../../../Shared/Source/Types/Types.h"

#include <filesystem>
#include <vector>

class dtNavMesh;
class dtNavMeshQuery;
class dtQueryFilter;

namespace MMO {

	// =============================================================================
	// NavMesh — Detour wrapper used by the WorldServer.
	//
	// Loads a single-tile .nav file produced by the editor's NavMeshBakeService
	// and answers FindPath queries. Owned per-MapInstance; each instance holds
	// its own dtNavMeshQuery and is single-threaded with respect to AI ticks.
	// If we ever tick AI from multiple threads, give each thread its own query
	// (Detour's nav data is shared-readable; only the query state is per-call).
	// =============================================================================

	class NavMesh
	{
	public:
		NavMesh();
		~NavMesh();

		NavMesh(const NavMesh&) = delete;
		NavMesh& operator=(const NavMesh&) = delete;

		// Load <ONAV header><tile bytes> from disk. Returns false on parse or
		// dtNavMesh::init failure (and leaves IsLoaded() false).
		bool LoadFromFile(const std::filesystem::path& path);

		bool IsLoaded() const { return m_NavMesh != nullptr; }

		// Find a path between two world positions on the XZ plane. The Y of each
		// input is sampled by the navmesh query (we project onto the surface).
		// On success outPath is populated with the straight-path waypoint corners
		// from start to end. Returns false if no path found, no navmesh loaded,
		// or the start/end aren't near any walkable polygon.
		//
		// outPath is cleared at the start of the call.
		bool FindPath(Vec2 start, Vec2 end, std::vector<Vec2>& outPath, int maxPoints = 64) const;

	private:
		// Owned. dtFreeNavMesh / dtFreeNavMeshQuery in the dtor.
		dtNavMesh* m_NavMesh = nullptr;
		dtNavMeshQuery* m_Query = nullptr;
	};

} // namespace MMO
