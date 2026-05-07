#pragma once

#include <cstdint>

namespace MMO {

	// =============================================================================
	// .nav file format — tiled navmesh
	// =============================================================================
	//
	// Detour navmesh serialised as one or more tiles. Single-tile maps store
	// one record; tiled maps store many. Editing one chunk only invalidates
	// the few overlapping tiles, so re-bake stays cheap as the world grows.
	//
	// Layout (little-endian):
	//   NavFileHeader   header;            // magic + dtNavMeshParams + tileCount
	//   for each tile:
	//       NavTileRecord record;
	//       uint8_t       data[record.dataSize];   // raw dtCreateNavMeshData output
	//
	// The header carries the dtNavMeshParams the runtime needs to call
	// dtNavMesh::init(const dtNavMeshParams*) before adding tiles. Each tile's
	// raw bytes are passed verbatim to dtNavMesh::addTile with DT_TILE_FREE_DATA.
	//
	// contentHash is a fingerprint of the input geometry that produced this
	// tile (chunk file mtimes, object transforms, model file mtimes). The
	// editor uses it to skip re-baking tiles whose inputs haven't changed.
	// The runtime ignores it.

	constexpr uint32_t NAV_FILE_MAGIC = 0x56414E4Fu; // 'O','N','A','V' little-endian
	constexpr uint32_t NAV_FILE_VERSION = 1;

	struct NavFileHeader
	{
		uint32_t magic;
		uint32_t version;
		uint32_t flags;     // bitset, currently 0

		// Multi-tile init parameters — passed to dtNavMesh::init(dtNavMeshParams*).
		float    origin[3];
		float    tileWidth;
		float    tileHeight;
		int32_t  maxTiles;
		int32_t  maxPolys;

		uint32_t tileCount;
		uint32_t reserved;
	};

	static_assert(sizeof(NavFileHeader) == 48, "NavFileHeader must be 48 bytes");

	struct NavTileRecord
	{
		int32_t  tileX;
		int32_t  tileY;
		uint64_t contentHash;
		uint32_t dataSize;
		uint32_t reserved;
		// followed by [dataSize] bytes of dtNavMesh tile data
	};

	static_assert(sizeof(NavTileRecord) == 24, "NavTileRecord must be 24 bytes");

} // namespace MMO
