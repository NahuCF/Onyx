#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Editor3D {

	// =============================================================================
	// NavMeshBuilder — Recast → Detour bake pipeline.
	//
	// BuildNavMesh produces one tile's bytes (dtCreateNavMeshData output). The
	// caller (NavMeshBakeService) loops over tiles and writes the multi-tile
	// .nav file via WriteNavMeshFile.
	// =============================================================================

	struct NavMeshBuildSettings
	{
		// Voxelisation
		float cellSize = 0.3f;     // x/z resolution (world units)
		float cellHeight = 0.2f;   // y resolution (world units)

		// Agent
		float walkableSlopeAngle = 45.0f;  // degrees; >max => non-walkable
		float walkableHeight = 2.0f;       // agent stand-up clearance (world units)
		float walkableClimb = 0.5f;        // max step-up in world units
		float walkableRadius = 0.5f;       // agent radius (erode walkable area)

		// Region / contour / polymesh
		float maxEdgeLen = 12.0f;
		float maxSimplificationError = 1.3f;
		int   minRegionArea = 8;       // squared cells
		int   mergeRegionArea = 20;    // squared cells
		int   maxVertsPerPoly = 6;

		// Detail mesh
		float detailSampleDistance = 6.0f;
		float detailSampleMaxError = 1.0f;

		// Tile size in world units (one tile covers tileSize x tileSize on XZ).
		// Borrowed by the bake service; the builder itself doesn't read this.
		float tileSize = 32.0f;

		// Tile coordinates emitted into dtNavMeshCreateParams. Defaults are
		// (0, 0) for the single-tile case; the bake service overrides these
		// for each tile in a multi-tile bake.
		int32_t tileX = 0;
		int32_t tileY = 0;
	};

	struct NavMeshBuildInput
	{
		// Flat triangle soup. Each vertex = 3 floats. Each triangle = 3 indices.
		// World-space, Y up (matches Recast).
		std::vector<float>   vertices;
		std::vector<int32_t> indices;

		// Voxelisation bounds (cfg.bmin/bmax). For tiled builds this is the
		// tile bounds expanded by a small border so geometry just outside the
		// tile contributes to edge accuracy.
		float boundsMin[3] = {0, 0, 0};
		float boundsMax[3] = {0, 0, 0};

		// Tile bounds (no border). dtNavMeshCreateParams.bmin/bmax. The
		// resulting tile data covers exactly this region. Set equal to
		// boundsMin/boundsMax for single-tile builds.
		float tileBoundsMin[3] = {0, 0, 0};
		float tileBoundsMax[3] = {0, 0, 0};
	};

	struct NavMeshBuildResult
	{
		bool                 success = false;
		std::vector<uint8_t> navData;        // raw dtNavMesh tile bytes
		int                  triangleCount = 0;
		int                  polyCount = 0;
		int                  vertexCount = 0;
		std::string          errorMessage;
	};

	NavMeshBuildResult BuildNavMesh(const NavMeshBuildInput& input,
									const NavMeshBuildSettings& settings);

	// One tile entry passed to WriteNavMeshFile. The bake service produces a
	// list of these and hands them off in one go.
	struct NavTileBlob
	{
		int32_t              tileX = 0;
		int32_t              tileY = 0;
		uint64_t             contentHash = 0;
		std::vector<uint8_t> data; // raw dtCreateNavMeshData output
	};

	struct NavFileWriteParams
	{
		float   origin[3] = {0, 0, 0};
		float   tileWidth = 32.0f;
		float   tileHeight = 32.0f;
		int32_t maxTiles = 0;   // 0 = next power of two of tile count
		int32_t maxPolys = 0;   // 0 = 256
	};

	// Writes <ONAVv2 header><tile records> to the path. Creates parent directories.
	// Returns false on I/O failure.
	bool WriteNavMeshFile(const std::filesystem::path& path,
						  const NavFileWriteParams& params,
						  const std::vector<NavTileBlob>& tiles);

} // namespace Editor3D
