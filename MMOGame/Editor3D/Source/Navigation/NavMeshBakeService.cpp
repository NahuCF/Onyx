#include "NavMeshBakeService.h"

#include "../World/EditorWorld.h"
#include "../World/EditorWorldSystem.h"
#include "../World/WorldChunk.h"
#include "NavMeshBuilder.h"

#include <Graphics/Mesh.h>
#include <Graphics/Model.h>
#include <Navigation/NavFile.h>
#include <Terrain/TerrainData.h>
#include <World/StaticObject.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Editor3D {

	namespace {

		// ---------- FNV-1a 64-bit hash --------------------------------------------------
		constexpr uint64_t FNV_OFFSET = 1469598103934665603ull;
		constexpr uint64_t FNV_PRIME  = 1099511628211ull;

		inline void HashBytes(uint64_t& h, const void* data, size_t bytes)
		{
			const uint8_t* p = static_cast<const uint8_t*>(data);
			for (size_t i = 0; i < bytes; ++i)
			{
				h ^= p[i];
				h *= FNV_PRIME;
			}
		}

		template <typename T>
		inline void HashValue(uint64_t& h, const T& v)
		{
			HashBytes(h, &v, sizeof(T));
		}

		// ---------- Geometry accumulator ------------------------------------------------
		struct GeometryAccumulator
		{
			std::vector<float>   vertices;
			std::vector<int32_t> indices;

			void AddVertex(float x, float y, float z)
			{
				vertices.push_back(x);
				vertices.push_back(y);
				vertices.push_back(z);
			}
			void AddTriangle(int32_t a, int32_t b, int32_t c)
			{
				indices.push_back(a);
				indices.push_back(b);
				indices.push_back(c);
			}
			bool Empty() const { return vertices.empty() || indices.empty(); }
		};

		// ---------- Cached chunk info (file metadata) -----------------------------------
		struct ChunkSlot
		{
			int32_t cx = 0;
			int32_t cz = 0;
			float   minX = 0, maxX = 0;
			float   minZ = 0, maxZ = 0;
			float   minY = 0, maxY = 0;
			uint64_t fileMtime = 0;
			uint64_t fileSize  = 0;
			std::string filePath;
		};

		// ---------- Cached static object info ------------------------------------------
		struct ObjectSlot
		{
			uint64_t   guid = 0;
			glm::vec3  position{0.0f};
			glm::quat  rotation{1, 0, 0, 0};
			float      scale = 1.0f;
			std::string modelPath;
			float      worldMin[3] = {0, 0, 0};
			float      worldMax[3] = {0, 0, 0};
			uint64_t   modelMtime = 0;
			uint64_t   modelSize  = 0;
			std::shared_ptr<Onyx::Model> cachedModel; // lazy-loaded
		};

		uint64_t FileMtime(const std::filesystem::path& p)
		{
			std::error_code ec;
			auto t = std::filesystem::last_write_time(p, ec);
			if (ec) return 0;
			return static_cast<uint64_t>(t.time_since_epoch().count());
		}

		uint64_t FileSize(const std::filesystem::path& p)
		{
			std::error_code ec;
			auto s = std::filesystem::file_size(p, ec);
			return ec ? 0ull : static_cast<uint64_t>(s);
		}

		// ---------- Triangle emitters ---------------------------------------------------
		void AppendTerrainChunk(const MMO::TerrainChunkData& data,
								int32_t chunkX, int32_t chunkZ,
								GeometryAccumulator& acc)
		{
			if (data.heightmap.empty())
				return;

			constexpr int RES = MMO::TERRAIN_CHUNK_RESOLUTION;
			constexpr int QUADS = RES - 1;
			constexpr int HOLE_GRID = MMO::TERRAIN_HOLE_GRID_SIZE;
			constexpr float CELL = MMO::TERRAIN_CHUNK_SIZE / static_cast<float>(QUADS);

			const float originX = static_cast<float>(chunkX) * MMO::TERRAIN_CHUNK_SIZE;
			const float originZ = static_cast<float>(chunkZ) * MMO::TERRAIN_CHUNK_SIZE;

			const int32_t baseVert = static_cast<int32_t>(acc.vertices.size() / 3);
			for (int z = 0; z < RES; ++z)
			{
				for (int x = 0; x < RES; ++x)
				{
					const float h = data.heightmap[z * RES + x];
					acc.AddVertex(originX + x * CELL, h, originZ + z * CELL);
				}
			}
			for (int z = 0; z < QUADS; ++z)
			{
				for (int x = 0; x < QUADS; ++x)
				{
					int holeX = std::min(x * HOLE_GRID / QUADS, HOLE_GRID - 1);
					int holeZ = std::min(z * HOLE_GRID / QUADS, HOLE_GRID - 1);
					if (data.holeMask & (1ULL << (holeZ * HOLE_GRID + holeX)))
						continue;

					const int32_t i00 = baseVert + z * RES + x;
					const int32_t i10 = baseVert + z * RES + (x + 1);
					const int32_t i01 = baseVert + (z + 1) * RES + x;
					const int32_t i11 = baseVert + (z + 1) * RES + (x + 1);

					acc.AddTriangle(i00, i01, i10);
					acc.AddTriangle(i10, i01, i11);
				}
			}
		}

		void AppendObjectMesh(const ObjectSlot& slot, GeometryAccumulator& acc)
		{
			if (!slot.cachedModel)
				return;

			glm::mat4 t(1.0f);
			t = glm::translate(t, slot.position);
			t = t * glm::mat4_cast(slot.rotation);
			t = glm::scale(t, glm::vec3(slot.scale));

			for (const auto& mesh : slot.cachedModel->GetMeshes())
			{
				if (mesh.m_Vertices.empty() || mesh.m_Indices.empty())
					continue;

				const int32_t baseVert = static_cast<int32_t>(acc.vertices.size() / 3);
				for (const auto& v : mesh.m_Vertices)
				{
					glm::vec4 local(v.position[0], v.position[1], v.position[2], 1.0f);
					glm::vec4 world = t * local;
					acc.AddVertex(world.x, world.y, world.z);
				}
				for (size_t i = 0; i + 2 < mesh.m_Indices.size(); i += 3)
				{
					acc.AddTriangle(baseVert + static_cast<int32_t>(mesh.m_Indices[i]),
									baseVert + static_cast<int32_t>(mesh.m_Indices[i + 1]),
									baseVert + static_cast<int32_t>(mesh.m_Indices[i + 2]));
				}
			}
		}

		// Load model on demand and compute its world AABB. Cached for the lifetime
		// of the bake; identical paths share one Model + one parsed mesh set.
		bool ResolveObjectMesh(ObjectSlot& slot,
							   std::unordered_map<std::string, std::shared_ptr<Onyx::Model>>& cache)
		{
			if (slot.modelPath.empty())
				return false;
			auto it = cache.find(slot.modelPath);
			if (it != cache.end())
			{
				slot.cachedModel = it->second;
			}
			else
			{
				try
				{
					slot.cachedModel = std::make_shared<Onyx::Model>(
						slot.modelPath.c_str(), /*loadTextures=*/false);
				}
				catch (...)
				{
					std::cerr << "[NavMesh] failed to load model: " << slot.modelPath << '\n';
					slot.cachedModel.reset();
				}
				cache.emplace(slot.modelPath, slot.cachedModel);
			}
			if (!slot.cachedModel)
				return false;

			// World AABB = model AABB transformed by (T * R * S).
			glm::mat4 t(1.0f);
			t = glm::translate(t, slot.position);
			t = t * glm::mat4_cast(slot.rotation);
			t = glm::scale(t, glm::vec3(slot.scale));

			float minB[3] = {std::numeric_limits<float>::max(),
							 std::numeric_limits<float>::max(),
							 std::numeric_limits<float>::max()};
			float maxB[3] = {std::numeric_limits<float>::lowest(),
							 std::numeric_limits<float>::lowest(),
							 std::numeric_limits<float>::lowest()};
			bool any = false;
			for (const auto& mesh : slot.cachedModel->GetMeshes())
			{
				if (mesh.m_Vertices.empty()) continue;
				const glm::vec3 lmin = mesh.GetBoundsMin();
				const glm::vec3 lmax = mesh.GetBoundsMax();
				const glm::vec3 corners[8] = {
					{lmin.x, lmin.y, lmin.z}, {lmax.x, lmin.y, lmin.z},
					{lmin.x, lmax.y, lmin.z}, {lmax.x, lmax.y, lmin.z},
					{lmin.x, lmin.y, lmax.z}, {lmax.x, lmin.y, lmax.z},
					{lmin.x, lmax.y, lmax.z}, {lmax.x, lmax.y, lmax.z}};
				for (const auto& c : corners)
				{
					glm::vec4 w = t * glm::vec4(c, 1.0f);
					minB[0] = std::min(minB[0], w.x);
					minB[1] = std::min(minB[1], w.y);
					minB[2] = std::min(minB[2], w.z);
					maxB[0] = std::max(maxB[0], w.x);
					maxB[1] = std::max(maxB[1], w.y);
					maxB[2] = std::max(maxB[2], w.z);
					any = true;
				}
			}
			if (!any)
			{
				slot.worldMin[0] = slot.position.x; slot.worldMin[1] = slot.position.y; slot.worldMin[2] = slot.position.z;
				slot.worldMax[0] = slot.position.x; slot.worldMax[1] = slot.position.y; slot.worldMax[2] = slot.position.z;
			}
			else
			{
				std::memcpy(slot.worldMin, minB, sizeof(minB));
				std::memcpy(slot.worldMax, maxB, sizeof(maxB));
			}
			return true;
		}

		bool RangesOverlap(float aMin, float aMax, float bMin, float bMax)
		{
			return aMin < bMax && bMin < aMax;
		}

		// ---------- Prior .nav reader (for tile reuse) ---------------------------------
		struct PriorTile
		{
			uint64_t             contentHash = 0;
			std::vector<uint8_t> data;
		};

		uint64_t TileKey(int32_t tx, int32_t ty)
		{
			return (static_cast<uint64_t>(static_cast<uint32_t>(tx)) << 32) |
				   static_cast<uint64_t>(static_cast<uint32_t>(ty));
		}

		// Read the existing .nav file (if any) and return a tx/ty -> (hash, bytes) map.
		// On parse error the returned map is empty — the bake just rebuilds every tile.
		std::unordered_map<uint64_t, PriorTile> ReadPriorTiles(const std::filesystem::path& path)
		{
			std::unordered_map<uint64_t, PriorTile> result;

			std::error_code ec;
			if (!std::filesystem::exists(path, ec))
				return result;

			std::ifstream in(path, std::ios::binary);
			if (!in.is_open())
				return result;

			MMO::NavFileHeader header{};
			in.read(reinterpret_cast<char*>(&header), sizeof(header));
			if (!in.good() || in.gcount() != static_cast<std::streamsize>(sizeof(header)))
				return result;
			if (header.magic != MMO::NAV_FILE_MAGIC || header.version != MMO::NAV_FILE_VERSION)
				return result;

			for (uint32_t i = 0; i < header.tileCount; ++i)
			{
				MMO::NavTileRecord rec{};
				in.read(reinterpret_cast<char*>(&rec), sizeof(rec));
				if (!in.good() || in.gcount() != static_cast<std::streamsize>(sizeof(rec)))
					return result;

				PriorTile pt;
				pt.contentHash = rec.contentHash;
				pt.data.resize(rec.dataSize);
				if (rec.dataSize > 0)
				{
					in.read(reinterpret_cast<char*>(pt.data.data()), rec.dataSize);
					if (!in.good() || static_cast<uint32_t>(in.gcount()) != rec.dataSize)
						return result;
				}
				result.emplace(TileKey(rec.tileX, rec.tileY), std::move(pt));
			}
			return result;
		}

	} // namespace

	NavMeshBakeStats NavMeshBakeService::Bake(EditorWorldSystem& worldSystem,
											  MMO::EditorWorld* editorWorld,
											  uint32_t mapId,
											  const std::filesystem::path& outputDir,
											  const NavMeshBuildSettings& settings)
	{
		NavMeshBakeStats stats;

		// Output path first — used for both the prior-tile read and the final write.
		char mapDirBuf[64];
		std::snprintf(mapDirBuf, sizeof(mapDirBuf), "maps/%03u", mapId);
		const std::filesystem::path navPath = outputDir / mapDirBuf / "navmesh.nav";
		stats.outputPath = navPath;

		// Save dirty chunks so the on-disk state matches the in-memory edits.
		// Mtimes are sampled *after* this for change detection.
		worldSystem.SaveDirtyChunks();

		// ---------- Catalogue every chunk on disk ----------
		std::vector<ChunkSlot> chunks;
		chunks.reserve(worldSystem.GetKnownChunkKeys().size());

		float worldMin[3] = {std::numeric_limits<float>::max(),
							 std::numeric_limits<float>::max(),
							 std::numeric_limits<float>::max()};
		float worldMax[3] = {std::numeric_limits<float>::lowest(),
							 std::numeric_limits<float>::lowest(),
							 std::numeric_limits<float>::lowest()};

		worldSystem.ForEachKnownChunkTransient(
			[&](WorldChunk* chunk, int32_t cx, int32_t cz) {
				if (!chunk || !chunk->GetTerrain())
					return;
				const auto& data = chunk->GetTerrain()->GetData();
				if (data.heightmap.empty())
					return;

				ChunkSlot s;
				s.cx = cx;
				s.cz = cz;
				s.minX = static_cast<float>(cx) * MMO::TERRAIN_CHUNK_SIZE;
				s.maxX = s.minX + MMO::TERRAIN_CHUNK_SIZE;
				s.minZ = static_cast<float>(cz) * MMO::TERRAIN_CHUNK_SIZE;
				s.maxZ = s.minZ + MMO::TERRAIN_CHUNK_SIZE;
				s.minY = data.minHeight;
				s.maxY = data.maxHeight > data.minHeight ? data.maxHeight : data.minHeight + 1.0f;
				s.filePath = worldSystem.GetChunkFilePathFor(cx, cz);
				s.fileMtime = FileMtime(s.filePath);
				s.fileSize  = FileSize(s.filePath);
				chunks.push_back(std::move(s));

				worldMin[0] = std::min(worldMin[0], s.minX);
				worldMin[2] = std::min(worldMin[2], s.minZ);
				worldMax[0] = std::max(worldMax[0], s.maxX);
				worldMax[2] = std::max(worldMax[2], s.maxZ);
				worldMin[1] = std::min(worldMin[1], s.minY);
				worldMax[1] = std::max(worldMax[1], s.maxY);
			});

		stats.chunksUsed = static_cast<int>(chunks.size());

		// ---------- Catalogue every static object with a model ----------
		std::vector<ObjectSlot> objects;
		std::unordered_map<std::string, std::shared_ptr<Onyx::Model>> modelCache;
		if (editorWorld)
		{
			for (const auto& obj : editorWorld->GetStaticObjects())
			{
				if (!obj || obj->GetModelPath().empty())
					continue;
				ObjectSlot s;
				s.guid = obj->GetGuid();
				s.position = obj->GetPosition();
				s.rotation = obj->GetRotation();
				s.scale = obj->GetScale();
				s.modelPath = obj->GetModelPath();
				s.modelMtime = FileMtime(s.modelPath);
				s.modelSize  = FileSize(s.modelPath);
				if (!ResolveObjectMesh(s, modelCache))
					continue;
				// Expand world bounds to include the object.
				worldMin[0] = std::min(worldMin[0], s.worldMin[0]);
				worldMin[1] = std::min(worldMin[1], s.worldMin[1]);
				worldMin[2] = std::min(worldMin[2], s.worldMin[2]);
				worldMax[0] = std::max(worldMax[0], s.worldMax[0]);
				worldMax[1] = std::max(worldMax[1], s.worldMax[1]);
				worldMax[2] = std::max(worldMax[2], s.worldMax[2]);
				objects.push_back(std::move(s));
			}
		}
		stats.objectsUsed = static_cast<int>(objects.size());

		if (chunks.empty() && objects.empty())
		{
			stats.errorMessage = "No geometry to bake (no chunks on disk and no static objects with meshes)";
			return stats;
		}

		// ---------- Compute the tile grid ----------
		const float tileSize = settings.tileSize > 0.0f ? settings.tileSize : 32.0f;
		// Snap origin to integer tile multiples so adding chunks later doesn't
		// reshuffle every tile's identity.
		const float originX = std::floor(worldMin[0] / tileSize) * tileSize;
		const float originZ = std::floor(worldMin[2] / tileSize) * tileSize;
		const int   tilesX = std::max(1, static_cast<int>(std::ceil((worldMax[0] - originX) / tileSize)));
		const int   tilesZ = std::max(1, static_cast<int>(std::ceil((worldMax[2] - originZ) / tileSize)));

		// Border in world units: enough cells beyond the tile bounds for Recast
		// to compute correct contours along the seam.
		const float border = (settings.walkableRadius + 3.0f) * settings.cellSize;

		// ---------- Prior tile bytes (for reuse) ----------
		auto priorTiles = ReadPriorTiles(navPath);

		// ---------- Bake each tile ----------
		std::vector<NavTileBlob> tileBlobs;
		tileBlobs.reserve(tilesX * tilesZ);
		std::string firstError;

		for (int ty = 0; ty < tilesZ; ++ty)
		{
			for (int tx = 0; tx < tilesX; ++tx)
			{
				const float tileMinX = originX + tx * tileSize;
				const float tileMaxX = tileMinX + tileSize;
				const float tileMinZ = originZ + ty * tileSize;
				const float tileMaxZ = tileMinZ + tileSize;

				const float bakeMinX = tileMinX - border;
				const float bakeMaxX = tileMaxX + border;
				const float bakeMinZ = tileMinZ - border;
				const float bakeMaxZ = tileMaxZ + border;

				// ---- Hash tile content from input metadata ----
				uint64_t hash = FNV_OFFSET;
				HashValue(hash, tx);
				HashValue(hash, ty);
				HashValue(hash, tileSize);
				HashValue(hash, originX);
				HashValue(hash, originZ);
				HashValue(hash, settings.cellSize);
				HashValue(hash, settings.cellHeight);
				HashValue(hash, settings.walkableSlopeAngle);
				HashValue(hash, settings.walkableHeight);
				HashValue(hash, settings.walkableClimb);
				HashValue(hash, settings.walkableRadius);

				int chunksTouched = 0;
				int objectsTouched = 0;
				for (const auto& c : chunks)
				{
					if (!RangesOverlap(c.minX, c.maxX, bakeMinX, bakeMaxX) ||
						!RangesOverlap(c.minZ, c.maxZ, bakeMinZ, bakeMaxZ))
						continue;
					HashValue(hash, c.cx);
					HashValue(hash, c.cz);
					HashValue(hash, c.fileMtime);
					HashValue(hash, c.fileSize);
					chunksTouched++;
				}
				for (const auto& o : objects)
				{
					if (!RangesOverlap(o.worldMin[0], o.worldMax[0], bakeMinX, bakeMaxX) ||
						!RangesOverlap(o.worldMin[2], o.worldMax[2], bakeMinZ, bakeMaxZ))
						continue;
					HashValue(hash, o.guid);
					HashBytes(hash, &o.position, sizeof(o.position));
					HashBytes(hash, &o.rotation, sizeof(o.rotation));
					HashValue(hash, o.scale);
					HashValue(hash, o.modelMtime);
					HashValue(hash, o.modelSize);
					objectsTouched++;
				}

				if (chunksTouched == 0 && objectsTouched == 0)
					continue; // empty tile, skip emitting a record

				// ---- Reuse if hash matches the prior bake ----
				auto priorIt = priorTiles.find(TileKey(tx, ty));
				if (priorIt != priorTiles.end() &&
					priorIt->second.contentHash == hash &&
					!priorIt->second.data.empty())
				{
					NavTileBlob blob;
					blob.tileX = tx;
					blob.tileY = ty;
					blob.contentHash = hash;
					blob.data = priorIt->second.data;
					tileBlobs.push_back(std::move(blob));
					stats.tilesReused++;
					continue;
				}

				// ---- Gather geometry inside the bake region ----
				GeometryAccumulator acc;

				// Terrain chunks: temp-load any not currently in memory.
				worldSystem.ForEachKnownChunkTransient(
					[&](WorldChunk* chunk, int32_t cx, int32_t cz) {
						const float cMinX = static_cast<float>(cx) * MMO::TERRAIN_CHUNK_SIZE;
						const float cMaxX = cMinX + MMO::TERRAIN_CHUNK_SIZE;
						const float cMinZ = static_cast<float>(cz) * MMO::TERRAIN_CHUNK_SIZE;
						const float cMaxZ = cMinZ + MMO::TERRAIN_CHUNK_SIZE;
						if (!RangesOverlap(cMinX, cMaxX, bakeMinX, bakeMaxX) ||
							!RangesOverlap(cMinZ, cMaxZ, bakeMinZ, bakeMaxZ))
							return;
						if (!chunk || !chunk->GetTerrain()) return;
						const auto& data = chunk->GetTerrain()->GetData();
						AppendTerrainChunk(data, cx, cz, acc);
					});

				// Static objects.
				for (const auto& o : objects)
				{
					if (!RangesOverlap(o.worldMin[0], o.worldMax[0], bakeMinX, bakeMaxX) ||
						!RangesOverlap(o.worldMin[2], o.worldMax[2], bakeMinZ, bakeMaxZ))
						continue;
					AppendObjectMesh(o, acc);
				}

				if (acc.Empty())
					continue;

				// ---- Build ----
				NavMeshBuildInput input;
				input.vertices = std::move(acc.vertices);
				input.indices = std::move(acc.indices);
				input.boundsMin[0] = bakeMinX;
				input.boundsMin[1] = worldMin[1] - 0.5f;
				input.boundsMin[2] = bakeMinZ;
				input.boundsMax[0] = bakeMaxX;
				input.boundsMax[1] = worldMax[1] + 1.0f;
				input.boundsMax[2] = bakeMaxZ;
				input.tileBoundsMin[0] = tileMinX;
				input.tileBoundsMin[1] = worldMin[1] - 0.5f;
				input.tileBoundsMin[2] = tileMinZ;
				input.tileBoundsMax[0] = tileMaxX;
				input.tileBoundsMax[1] = worldMax[1] + 1.0f;
				input.tileBoundsMax[2] = tileMaxZ;

				NavMeshBuildSettings tileSettings = settings;
				tileSettings.tileX = tx;
				tileSettings.tileY = ty;

				const auto build = BuildNavMesh(input, tileSettings);
				if (!build.success)
				{
					if (firstError.empty())
					{
						firstError = "Tile (" + std::to_string(tx) + "," + std::to_string(ty) +
									 "): " + build.errorMessage;
					}
					continue;
				}
				stats.triangleCount += build.triangleCount;
				stats.polyCount += build.polyCount;

				NavTileBlob blob;
				blob.tileX = tx;
				blob.tileY = ty;
				blob.contentHash = hash;
				blob.data = std::move(build.navData);
				tileBlobs.push_back(std::move(blob));
				stats.tilesRebuilt++;
			}
		}

		// Re-add reused tiles' poly count contribution by lifting it from the prior
		// data. We don't re-decode the tile to count polys; instead we use the
		// existing total — close enough for the stats line.
		// (Kept simple: stats.polyCount only tracks newly built tiles.)

		stats.tileCount = static_cast<int>(tileBlobs.size());
		if (tileBlobs.empty())
		{
			stats.errorMessage = firstError.empty()
									 ? "Bake produced zero tiles"
									 : firstError;
			return stats;
		}

		// ---------- Write ----------
		NavFileWriteParams writeParams;
		writeParams.origin[0] = originX;
		writeParams.origin[1] = worldMin[1] - 0.5f;
		writeParams.origin[2] = originZ;
		writeParams.tileWidth = tileSize;
		writeParams.tileHeight = tileSize;
		writeParams.maxTiles = 0; // builder rounds up to next pow2
		writeParams.maxPolys = 0; // builder defaults to 256

		if (!WriteNavMeshFile(navPath, writeParams, tileBlobs))
		{
			stats.errorMessage = "Failed to write nav file: " + navPath.string();
			return stats;
		}

		stats.success = true;
		return stats;
	}

} // namespace Editor3D
