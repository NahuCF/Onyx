#pragma once

#include "NavMeshBuilder.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace MMO {
	class EditorWorld;
}

namespace Editor3D {

	class EditorWorldSystem;

	// =============================================================================
	// NavMeshBakeService — orchestrates a tiled Recast bake for the whole map.
	//
	//   - Walks every chunk on disk (loaded or not — the streaming system can't
	//     hide content from the bake).
	//   - Splits the world bounds into tileSize-sized tiles.
	//   - Per tile: hashes its inputs (chunk file mtimes, object transforms,
	//     model file mtimes). If the hash matches the prior bake's value the
	//     tile bytes are reused; otherwise the tile is rebuilt.
	//   - Writes the multi-tile .nav alongside the chunks at
	//     <outputDir>/maps/<padded mapId>/navmesh.nav.
	// =============================================================================

	struct NavMeshBakeStats
	{
		bool                  success = false;
		int                   tileCount = 0;       // total tiles in the file
		int                   tilesRebuilt = 0;    // tiles whose hash changed
		int                   tilesReused = 0;     // tiles served from prior bake
		int                   chunksUsed = 0;
		int                   objectsUsed = 0;
		int                   triangleCount = 0;   // sum across rebuilt tiles
		int                   polyCount = 0;       // sum across all tiles in the new file
		std::filesystem::path outputPath;
		std::string           errorMessage;
	};

	class NavMeshBakeService
	{
	public:
		static NavMeshBakeStats Bake(EditorWorldSystem& worldSystem,
									 MMO::EditorWorld* editorWorld,
									 uint32_t mapId,
									 const std::filesystem::path& outputDir,
									 const NavMeshBuildSettings& settings = {});
	};

} // namespace Editor3D
