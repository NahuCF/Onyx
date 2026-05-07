#include "NavMeshDebugView.h"

#include <Navigation/NavFile.h>

#include <DetourNavMesh.h>

#include <cstring>
#include <fstream>
#include <iostream>

namespace Editor3D {

	namespace {

		void PolyColor(uint32_t polyIndex, float outRgba[4])
		{
			uint32_t h = polyIndex * 2654435761u;
			h ^= h >> 16;
			float r = ((h >> 0) & 0xFF) / 255.0f;
			float g = ((h >> 8) & 0xFF) / 255.0f;
			float b = ((h >> 16) & 0xFF) / 255.0f;
			r = 0.35f + 0.65f * r;
			g = 0.35f + 0.65f * g;
			b = 0.35f + 0.65f * b;
			outRgba[0] = r;
			outRgba[1] = g;
			outRgba[2] = b;
			outRgba[3] = 0.45f;
		}

	} // namespace

	NavMeshDebugView::NavMeshDebugView() = default;

	NavMeshDebugView::~NavMeshDebugView()
	{
		Unload();
	}

	void NavMeshDebugView::Unload()
	{
		if (m_NavMesh)
		{
			dtFreeNavMesh(m_NavMesh);
			m_NavMesh = nullptr;
		}
		m_SourcePath.clear();
	}

	bool NavMeshDebugView::LoadFromFile(const std::filesystem::path& path)
	{
		Unload();

		std::error_code ec;
		if (!std::filesystem::exists(path, ec))
			return false;

		std::ifstream in(path, std::ios::binary);
		if (!in.is_open())
			return false;

		MMO::NavFileHeader header{};
		in.read(reinterpret_cast<char*>(&header), sizeof(header));
		if (!in.good() || in.gcount() != static_cast<std::streamsize>(sizeof(header)))
			return false;
		if (header.magic != MMO::NAV_FILE_MAGIC || header.version != MMO::NAV_FILE_VERSION ||
			header.tileCount == 0)
			return false;

		dtNavMesh* mesh = dtAllocNavMesh();
		if (!mesh)
			return false;

		dtNavMeshParams params{};
		std::memcpy(params.orig, header.origin, sizeof(params.orig));
		params.tileWidth = header.tileWidth;
		params.tileHeight = header.tileHeight;
		params.maxTiles = header.maxTiles;
		params.maxPolys = header.maxPolys;
		if (dtStatusFailed(mesh->init(&params)))
		{
			dtFreeNavMesh(mesh);
			return false;
		}

		int tilesAdded = 0;
		for (uint32_t i = 0; i < header.tileCount; ++i)
		{
			MMO::NavTileRecord rec{};
			in.read(reinterpret_cast<char*>(&rec), sizeof(rec));
			if (!in.good() || in.gcount() != static_cast<std::streamsize>(sizeof(rec)))
			{
				dtFreeNavMesh(mesh);
				return false;
			}
			if (rec.dataSize == 0)
				continue;

			auto* tileBytes = static_cast<unsigned char*>(dtAlloc(rec.dataSize, DT_ALLOC_PERM));
			if (!tileBytes)
			{
				dtFreeNavMesh(mesh);
				return false;
			}
			in.read(reinterpret_cast<char*>(tileBytes), rec.dataSize);
			if (!in.good() || static_cast<uint32_t>(in.gcount()) != rec.dataSize)
			{
				dtFree(tileBytes);
				dtFreeNavMesh(mesh);
				return false;
			}

			if (dtStatusFailed(mesh->addTile(tileBytes, static_cast<int>(rec.dataSize),
											 DT_TILE_FREE_DATA, /*lastRef=*/0, /*outRef=*/nullptr)))
			{
				dtFree(tileBytes);
				continue;
			}
			tilesAdded++;
		}

		if (tilesAdded == 0)
		{
			dtFreeNavMesh(mesh);
			return false;
		}

		m_NavMesh = mesh;
		m_SourcePath = path;
		return true;
	}

	int NavMeshDebugView::BuildRenderGeometry(std::vector<float>& outVerts,
											  std::vector<uint32_t>& outIndices,
											  float yBias) const
	{
		outVerts.clear();
		outIndices.clear();

		if (!m_NavMesh)
			return 0;

		const dtNavMesh* mesh = m_NavMesh;

		int polysEmitted = 0;
		const int maxTiles = mesh->getMaxTiles();
		for (int i = 0; i < maxTiles; ++i)
		{
			const dtMeshTile* tile = mesh->getTile(i);
			if (!tile || !tile->header)
				continue;

			for (int p = 0; p < tile->header->polyCount; ++p)
			{
				const dtPoly& poly = tile->polys[p];
				if (poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
					continue;

				const dtPolyDetail& pd = tile->detailMeshes[p];

				float color[4];
				PolyColor(static_cast<uint32_t>(polysEmitted), color);

				for (int t = 0; t < pd.triCount; ++t)
				{
					const unsigned char* tri = &tile->detailTris[(pd.triBase + t) * 4];

					for (int v = 0; v < 3; ++v)
					{
						const unsigned char idx = tri[v];
						const float* src;
						if (idx < poly.vertCount)
						{
							src = &tile->verts[poly.verts[idx] * 3];
						}
						else
						{
							src = &tile->detailVerts[(pd.vertBase + (idx - poly.vertCount)) * 3];
						}

						const uint32_t baseVert = static_cast<uint32_t>(outVerts.size() / 7);
						outVerts.push_back(src[0]);
						outVerts.push_back(src[1] + yBias);
						outVerts.push_back(src[2]);
						outVerts.push_back(color[0]);
						outVerts.push_back(color[1]);
						outVerts.push_back(color[2]);
						outVerts.push_back(color[3]);
						outIndices.push_back(baseVert);
					}
				}

				polysEmitted++;
			}
		}

		return polysEmitted;
	}

} // namespace Editor3D
