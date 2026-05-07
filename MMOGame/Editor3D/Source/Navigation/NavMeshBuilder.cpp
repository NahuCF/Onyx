#include "NavMeshBuilder.h"

#include <Navigation/NavFile.h>

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <Recast.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>

namespace Editor3D {

	namespace {

		struct HeightfieldDeleter
		{
			void operator()(rcHeightfield* p) const noexcept { rcFreeHeightField(p); }
		};
		struct CompactHeightfieldDeleter
		{
			void operator()(rcCompactHeightfield* p) const noexcept { rcFreeCompactHeightfield(p); }
		};
		struct ContourSetDeleter
		{
			void operator()(rcContourSet* p) const noexcept { rcFreeContourSet(p); }
		};
		struct PolyMeshDeleter
		{
			void operator()(rcPolyMesh* p) const noexcept { rcFreePolyMesh(p); }
		};
		struct PolyMeshDetailDeleter
		{
			void operator()(rcPolyMeshDetail* p) const noexcept { rcFreePolyMeshDetail(p); }
		};

		using HeightfieldPtr = std::unique_ptr<rcHeightfield, HeightfieldDeleter>;
		using CompactHeightfieldPtr = std::unique_ptr<rcCompactHeightfield, CompactHeightfieldDeleter>;
		using ContourSetPtr = std::unique_ptr<rcContourSet, ContourSetDeleter>;
		using PolyMeshPtr = std::unique_ptr<rcPolyMesh, PolyMeshDeleter>;
		using PolyMeshDetailPtr = std::unique_ptr<rcPolyMeshDetail, PolyMeshDetailDeleter>;

	} // namespace

	NavMeshBuildResult BuildNavMesh(const NavMeshBuildInput& input,
									const NavMeshBuildSettings& settings)
	{
		NavMeshBuildResult result;

		const int triCount = static_cast<int>(input.indices.size()) / 3;
		const int vertCount = static_cast<int>(input.vertices.size()) / 3;
		result.triangleCount = triCount;
		result.vertexCount = vertCount;

		if (triCount == 0 || vertCount == 0)
		{
			result.errorMessage = "Input geometry is empty";
			return result;
		}

		rcContext ctx(/*state=*/false);

		// ---------- Configure ----------
		rcConfig cfg{};
		cfg.cs = settings.cellSize;
		cfg.ch = settings.cellHeight;
		cfg.walkableSlopeAngle = settings.walkableSlopeAngle;
		cfg.walkableHeight = static_cast<int>(std::ceil(settings.walkableHeight / cfg.ch));
		cfg.walkableClimb = static_cast<int>(std::floor(settings.walkableClimb / cfg.ch));
		cfg.walkableRadius = static_cast<int>(std::ceil(settings.walkableRadius / cfg.cs));
		cfg.maxEdgeLen = static_cast<int>(settings.maxEdgeLen / cfg.cs);
		cfg.maxSimplificationError = settings.maxSimplificationError;
		cfg.minRegionArea = static_cast<int>(rcSqr(settings.minRegionArea));
		cfg.mergeRegionArea = static_cast<int>(rcSqr(settings.mergeRegionArea));
		cfg.maxVertsPerPoly = settings.maxVertsPerPoly;
		cfg.detailSampleDist = settings.detailSampleDistance < 0.9f
								   ? 0.0f
								   : cfg.cs * settings.detailSampleDistance;
		cfg.detailSampleMaxError = cfg.ch * settings.detailSampleMaxError;

		std::memcpy(cfg.bmin, input.boundsMin, sizeof(cfg.bmin));
		std::memcpy(cfg.bmax, input.boundsMax, sizeof(cfg.bmax));
		rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

		if (cfg.width <= 0 || cfg.height <= 0)
		{
			result.errorMessage = "Computed grid size is zero (input bounds too small for cell size)";
			return result;
		}

		// ---------- Rasterise ----------
		HeightfieldPtr solid(rcAllocHeightfield());
		if (!solid)
		{
			result.errorMessage = "Failed to allocate heightfield";
			return result;
		}
		if (!rcCreateHeightfield(&ctx, *solid, cfg.width, cfg.height,
								 cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
		{
			result.errorMessage = "rcCreateHeightfield failed";
			return result;
		}

		std::vector<unsigned char> triAreas(triCount, 0);
		rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle,
								input.vertices.data(), vertCount,
								input.indices.data(), triCount,
								triAreas.data());

		if (!rcRasterizeTriangles(&ctx,
								  input.vertices.data(), vertCount,
								  input.indices.data(), triAreas.data(),
								  triCount, *solid, cfg.walkableClimb))
		{
			result.errorMessage = "rcRasterizeTriangles failed";
			return result;
		}

		// ---------- Filter ----------
		rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *solid);
		rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *solid);
		rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *solid);

		// ---------- Compact + erode ----------
		CompactHeightfieldPtr chf(rcAllocCompactHeightfield());
		if (!chf || !rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb,
											   *solid, *chf))
		{
			result.errorMessage = "rcBuildCompactHeightfield failed";
			return result;
		}
		solid.reset();

		if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf))
		{
			result.errorMessage = "rcErodeWalkableArea failed";
			return result;
		}

		// ---------- Regions ----------
		if (!rcBuildDistanceField(&ctx, *chf))
		{
			result.errorMessage = "rcBuildDistanceField failed";
			return result;
		}
		if (!rcBuildRegions(&ctx, *chf, /*borderSize=*/0,
							cfg.minRegionArea, cfg.mergeRegionArea))
		{
			result.errorMessage = "rcBuildRegions failed";
			return result;
		}

		// ---------- Contours / polymesh ----------
		ContourSetPtr cset(rcAllocContourSet());
		if (!cset || !rcBuildContours(&ctx, *chf, cfg.maxSimplificationError,
									  cfg.maxEdgeLen, *cset))
		{
			result.errorMessage = "rcBuildContours failed";
			return result;
		}

		PolyMeshPtr pmesh(rcAllocPolyMesh());
		if (!pmesh || !rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh))
		{
			result.errorMessage = "rcBuildPolyMesh failed";
			return result;
		}

		PolyMeshDetailPtr dmesh(rcAllocPolyMeshDetail());
		if (!dmesh || !rcBuildPolyMeshDetail(&ctx, *pmesh, *chf,
											 cfg.detailSampleDist,
											 cfg.detailSampleMaxError, *dmesh))
		{
			result.errorMessage = "rcBuildPolyMeshDetail failed";
			return result;
		}

		// Mark all polys walkable so the default dtQueryFilter accepts them.
		for (int i = 0; i < pmesh->npolys; ++i)
		{
			if (pmesh->areas[i] == RC_WALKABLE_AREA)
				pmesh->flags[i] = 1;
		}

		// ---------- Pack into Detour tile data ----------
		dtNavMeshCreateParams params{};
		params.verts = pmesh->verts;
		params.vertCount = pmesh->nverts;
		params.polys = pmesh->polys;
		params.polyAreas = pmesh->areas;
		params.polyFlags = pmesh->flags;
		params.polyCount = pmesh->npolys;
		params.nvp = pmesh->nvp;
		params.detailMeshes = dmesh->meshes;
		params.detailVerts = dmesh->verts;
		params.detailVertsCount = dmesh->nverts;
		params.detailTris = dmesh->tris;
		params.detailTriCount = dmesh->ntris;
		params.walkableHeight = settings.walkableHeight;
		params.walkableRadius = settings.walkableRadius;
		params.walkableClimb = settings.walkableClimb;
		// Tile bounds (without border) — what the runtime treats as this tile's
		// footprint for addTile coordinate lookups.
		std::memcpy(params.bmin, input.tileBoundsMin, sizeof(params.bmin));
		std::memcpy(params.bmax, input.tileBoundsMax, sizeof(params.bmax));
		params.cs = cfg.cs;
		params.ch = cfg.ch;
		params.tileX = settings.tileX;
		params.tileY = settings.tileY;
		params.tileLayer = 0;
		params.buildBvTree = true;

		unsigned char* navData = nullptr;
		int navDataSize = 0;
		if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			result.errorMessage = "dtCreateNavMeshData failed";
			return result;
		}

		result.navData.assign(navData, navData + navDataSize);
		dtFree(navData);

		result.success = true;
		result.polyCount = pmesh->npolys;
		return result;
	}

	namespace {
		// Smallest power-of-two >= n, clamped to 1.
		int32_t NextPow2(int32_t n)
		{
			int32_t p = 1;
			while (p < n) p <<= 1;
			return p;
		}
	}

	bool WriteNavMeshFile(const std::filesystem::path& path,
						  const NavFileWriteParams& params,
						  const std::vector<NavTileBlob>& tiles)
	{
		if (tiles.empty())
			return false;

		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out.is_open())
			return false;

		MMO::NavFileHeader header{};
		header.magic = MMO::NAV_FILE_MAGIC;
		header.version = MMO::NAV_FILE_VERSION;
		header.flags = 0;
		std::memcpy(header.origin, params.origin, sizeof(header.origin));
		header.tileWidth = params.tileWidth;
		header.tileHeight = params.tileHeight;
		header.maxTiles = params.maxTiles > 0
							  ? params.maxTiles
							  : NextPow2(static_cast<int32_t>(tiles.size()));
		header.maxPolys = params.maxPolys > 0 ? params.maxPolys : 256;
		header.tileCount = static_cast<uint32_t>(tiles.size());
		header.reserved = 0;

		out.write(reinterpret_cast<const char*>(&header), sizeof(header));

		for (const auto& tile : tiles)
		{
			MMO::NavTileRecord record{};
			record.tileX = tile.tileX;
			record.tileY = tile.tileY;
			record.contentHash = tile.contentHash;
			record.dataSize = static_cast<uint32_t>(tile.data.size());
			record.reserved = 0;

			out.write(reinterpret_cast<const char*>(&record), sizeof(record));
			if (record.dataSize > 0)
			{
				out.write(reinterpret_cast<const char*>(tile.data.data()),
						  static_cast<std::streamsize>(tile.data.size()));
			}
		}

		return out.good();
	}

} // namespace Editor3D
