#include "NavMesh.h"

#include "../../../Shared/Source/Navigation/NavFile.h"

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace MMO {

	NavMesh::NavMesh() = default;

	NavMesh::~NavMesh()
	{
		if (m_Query)
		{
			dtFreeNavMeshQuery(m_Query);
			m_Query = nullptr;
		}
		if (m_NavMesh)
		{
			dtFreeNavMesh(m_NavMesh);
			m_NavMesh = nullptr;
		}
	}

	bool NavMesh::LoadFromFile(const std::filesystem::path& path)
	{
		std::error_code ec;
		if (!std::filesystem::exists(path, ec))
		{
			std::cerr << "[NavMesh] file not found: " << path.string() << '\n';
			return false;
		}

		std::ifstream in(path, std::ios::binary);
		if (!in.is_open())
		{
			std::cerr << "[NavMesh] failed to open: " << path.string() << '\n';
			return false;
		}

		NavFileHeader header{};
		in.read(reinterpret_cast<char*>(&header), sizeof(header));
		if (!in.good() || in.gcount() != static_cast<std::streamsize>(sizeof(header)))
		{
			std::cerr << "[NavMesh] short header read: " << path.string() << '\n';
			return false;
		}
		if (header.magic != NAV_FILE_MAGIC)
		{
			std::cerr << "[NavMesh] bad magic in " << path.string() << '\n';
			return false;
		}
		if (header.version != NAV_FILE_VERSION)
		{
			std::cerr << "[NavMesh] unsupported version " << header.version
					  << " (expected " << NAV_FILE_VERSION << ") in "
					  << path.string() << '\n';
			return false;
		}
		if (header.tileCount == 0)
		{
			std::cerr << "[NavMesh] no tiles in " << path.string() << '\n';
			return false;
		}

		dtNavMesh* mesh = dtAllocNavMesh();
		if (!mesh)
		{
			std::cerr << "[NavMesh] dtAllocNavMesh failed\n";
			return false;
		}

		dtNavMeshParams params{};
		std::memcpy(params.orig, header.origin, sizeof(params.orig));
		params.tileWidth = header.tileWidth;
		params.tileHeight = header.tileHeight;
		params.maxTiles = header.maxTiles;
		params.maxPolys = header.maxPolys;
		dtStatus status = mesh->init(&params);
		if (dtStatusFailed(status))
		{
			std::cerr << "[NavMesh] dtNavMesh::init(params) failed status=" << status
					  << " for " << path.string() << '\n';
			dtFreeNavMesh(mesh);
			return false;
		}

		int tilesAdded = 0;
		for (uint32_t i = 0; i < header.tileCount; ++i)
		{
			NavTileRecord rec{};
			in.read(reinterpret_cast<char*>(&rec), sizeof(rec));
			if (!in.good() || in.gcount() != static_cast<std::streamsize>(sizeof(rec)))
			{
				std::cerr << "[NavMesh] short tile-record read at index " << i
						  << " in " << path.string() << '\n';
				dtFreeNavMesh(mesh);
				return false;
			}
			if (rec.dataSize == 0)
				continue;

			auto* tileBytes = static_cast<unsigned char*>(dtAlloc(rec.dataSize, DT_ALLOC_PERM));
			if (!tileBytes)
			{
				std::cerr << "[NavMesh] dtAlloc failed for tile (" << rec.tileX << ","
						  << rec.tileY << ") size " << rec.dataSize << '\n';
				dtFreeNavMesh(mesh);
				return false;
			}
			in.read(reinterpret_cast<char*>(tileBytes), rec.dataSize);
			if (!in.good() || static_cast<uint32_t>(in.gcount()) != rec.dataSize)
			{
				std::cerr << "[NavMesh] short tile-data read at (" << rec.tileX << ","
						  << rec.tileY << ") in " << path.string() << '\n';
				dtFree(tileBytes);
				dtFreeNavMesh(mesh);
				return false;
			}

			dtTileRef ref = 0;
			dtStatus addStatus = mesh->addTile(tileBytes, static_cast<int>(rec.dataSize),
											   DT_TILE_FREE_DATA, /*lastRef=*/0, &ref);
			if (dtStatusFailed(addStatus))
			{
				std::cerr << "[NavMesh] addTile failed at (" << rec.tileX << ","
						  << rec.tileY << ") status=" << addStatus << '\n';
				dtFree(tileBytes);
				continue;
			}
			tilesAdded++;
		}

		if (tilesAdded == 0)
		{
			std::cerr << "[NavMesh] no tiles added (file " << path.string() << ")\n";
			dtFreeNavMesh(mesh);
			return false;
		}

		dtNavMeshQuery* query = dtAllocNavMeshQuery();
		if (!query || dtStatusFailed(query->init(mesh, /*maxNodes=*/2048)))
		{
			std::cerr << "[NavMesh] dtNavMeshQuery::init failed for " << path.string() << '\n';
			if (query) dtFreeNavMeshQuery(query);
			dtFreeNavMesh(mesh);
			return false;
		}

		if (m_Query)   dtFreeNavMeshQuery(m_Query);
		if (m_NavMesh) dtFreeNavMesh(m_NavMesh);
		m_NavMesh = mesh;
		m_Query = query;

		std::cout << "[NavMesh] loaded " << tilesAdded << "/" << header.tileCount
				  << " tiles from " << path.string() << '\n';
		return true;
	}

	bool NavMesh::FindPath(Vec2 start, Vec2 end, std::vector<Vec2>& outPath, int maxPoints) const
	{
		outPath.clear();
		if (!m_Query || !m_NavMesh)
			return false;

		const float startPos[3] = { start.x, 0.0f, start.y };
		const float endPos[3]   = { end.x,   0.0f, end.y };
		const float extents[3]  = { 2.0f, 1024.0f, 2.0f };

		dtQueryFilter filter;
		filter.setIncludeFlags(0xFFFF);
		filter.setExcludeFlags(0);

		dtPolyRef startRef = 0, endRef = 0;
		float startNearest[3] = {0, 0, 0};
		float endNearest[3]   = {0, 0, 0};
		dtStatus s1 = m_Query->findNearestPoly(startPos, extents, &filter, &startRef, startNearest);
		dtStatus s2 = m_Query->findNearestPoly(endPos,   extents, &filter, &endRef,   endNearest);
		if (dtStatusFailed(s1) || dtStatusFailed(s2) || startRef == 0 || endRef == 0)
			return false;

		constexpr int kMaxCorridor = 256;
		dtPolyRef corridor[kMaxCorridor];
		int corridorCount = 0;
		dtStatus pathStatus = m_Query->findPath(startRef, endRef, startNearest, endNearest,
												&filter, corridor, &corridorCount, kMaxCorridor);
		if (dtStatusFailed(pathStatus) || corridorCount == 0)
			return false;

		const int cap = (maxPoints > 0 && maxPoints < kMaxCorridor) ? maxPoints : kMaxCorridor;
		std::vector<float> straight(cap * 3, 0.0f);
		std::vector<unsigned char> straightFlags(cap, 0);
		std::vector<dtPolyRef> straightRefs(cap, 0);
		int straightCount = 0;
		dtStatus straightStatus = m_Query->findStraightPath(
			startNearest, endNearest, corridor, corridorCount,
			straight.data(), straightFlags.data(), straightRefs.data(),
			&straightCount, cap, /*options=*/0);
		if (dtStatusFailed(straightStatus) || straightCount == 0)
			return false;

		outPath.reserve(straightCount);
		for (int i = 0; i < straightCount; ++i)
		{
			outPath.emplace_back(straight[i * 3 + 0], straight[i * 3 + 2]);
		}
		return true;
	}

} // namespace MMO
