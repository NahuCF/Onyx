#include "EditorTerrainSystem.h"
#include <Terrain/ChunkIO.h>
#include <World/WorldObjectData.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

using Shared::WorldToChunkX;
using Shared::WorldToChunkZ;

namespace Editor3D {

	EditorTerrainSystem::EditorTerrainSystem()
	{
	}

	EditorTerrainSystem::~EditorTerrainSystem()
	{
		Shutdown();
	}

	void EditorTerrainSystem::Init(const std::string& chunksDirectory)
	{
		m_ProjectPath = chunksDirectory.empty() ? "." : chunksDirectory;

		std::filesystem::create_directories(m_ProjectPath);

		if (std::filesystem::exists(m_ProjectPath))
		{
			for (const auto& entry : std::filesystem::directory_iterator(m_ProjectPath))
			{
				if (entry.path().extension() == ".terrain")
				{
					std::string stem = entry.path().stem().string();
					if (stem.rfind("chunk_", 0) != 0)
						continue;

					std::string coords = stem.substr(6);
					size_t sep = coords.find('_');
					if (sep == std::string::npos)
						continue;

					int32_t cx = std::stoi(coords.substr(0, sep));
					int32_t cz = std::stoi(coords.substr(sep + 1));

					m_KnownChunkFiles.insert(MakeChunkKey(cx, cz));
				}
			}
		}
	}

	void EditorTerrainSystem::Shutdown()
	{
		SaveDirtyChunks();
		m_Chunks.clear();
		m_LoadQueue.clear();
		m_KnownChunkFiles.clear();
		m_MaterialLayerMap.clear();
	}

	void EditorTerrainSystem::Update(const glm::vec3& cameraPos, const glm::mat4& viewProj, float deltaTime)
	{
		m_CameraPosition = cameraPos;
		m_Frustum.Update(viewProj);

		m_LoadQueue.clear();

		int centerChunkX = WorldToChunkX(cameraPos.x);
		int centerChunkZ = WorldToChunkZ(cameraPos.z);
		int loadRadius = static_cast<int>(std::ceil(m_Settings.loadDistance / CHUNK_SIZE));

		for (int dz = -loadRadius; dz <= loadRadius; dz++)
		{
			for (int dx = -loadRadius; dx <= loadRadius; dx++)
			{
				int32_t chunkX = centerChunkX + dx;
				int32_t chunkZ = centerChunkZ + dz;

				float dist = CalculateChunkDistance(chunkX, chunkZ);
				if (dist > m_Settings.loadDistance)
					continue;

				int32_t key = MakeChunkKey(chunkX, chunkZ);
				if (m_Chunks.find(key) != m_Chunks.end())
					continue;
				if (m_KnownChunkFiles.find(key) == m_KnownChunkFiles.end())
					continue;

				ChunkLoadRequest req;
				req.chunkX = chunkX;
				req.chunkZ = chunkZ;
				req.priority = dist;
				m_LoadQueue.push_back(req);
			}
		}

		std::sort(m_LoadQueue.begin(), m_LoadQueue.end(),
				  [](const ChunkLoadRequest& a, const ChunkLoadRequest& b) {
					  return a.priority < b.priority;
				  });

		ProcessLoadQueue();
		UnloadDistantChunks();

		for (auto& [key, chunk] : m_Chunks)
		{
			if (chunk->GetState() == ChunkState::Loaded)
			{
				chunk->CreateGPUResources();
			}
		}

		m_Stats.totalChunks = (int)m_Chunks.size();
		m_Stats.loadedChunks = 0;
		m_Stats.visibleChunks = 0;
		m_Stats.dirtyChunks = 0;

		for (auto& [key, chunk] : m_Chunks)
		{
			if (chunk->GetState() == ChunkState::Active)
			{
				m_Stats.loadedChunks++;
				if (IsChunkVisible(chunk->GetChunkX(), chunk->GetChunkZ()))
				{
					m_Stats.visibleChunks++;
				}
			}
			if (chunk->IsModified())
			{
				m_Stats.dirtyChunks++;
			}
		}
	}

	void EditorTerrainSystem::Render(Shader* terrainShader, const glm::mat4& viewProj,
									 PerChunkCallback perChunkSetup)
	{
		if (!terrainShader)
			return;

		static const glm::mat4 identity(1.0f);
		terrainShader->SetMat4("u_Model", identity);

		for (auto& [key, chunk] : m_Chunks)
		{
			if (chunk->GetState() != ChunkState::Active)
				continue;
			if (!m_Settings.enableFrustumCulling || IsChunkVisible(chunk->GetChunkX(), chunk->GetChunkZ()))
			{
				if (perChunkSetup)
				{
					perChunkSetup(chunk.get(), terrainShader);
				}
				chunk->Draw(terrainShader);
			}
		}
	}

	void EditorTerrainSystem::SetDefaultMaterialIds(const std::string ids[MAX_TERRAIN_LAYERS])
	{
		for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
		{
			m_DefaultMaterialIds[i] = ids[i];
			if (!ids[i].empty())
			{
				m_MaterialLayerMap[ids[i]] = i;
			}
		}
		for (auto& [key, chunk] : m_Chunks)
		{
			ApplyDefaultMaterials(chunk.get());
		}
	}

	void EditorTerrainSystem::ApplyDefaultMaterials(TerrainChunk* chunk)
	{
		const auto& data = chunk->GetData();
		bool allEmpty = true;
		for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
		{
			if (!data.materialIds[i].empty())
			{
				allEmpty = false;
				break;
			}
		}
		if (!allEmpty)
			return;

		std::string materials[MAX_TERRAIN_LAYERS];
		for (auto& [matId, layer] : m_MaterialLayerMap)
		{
			if (layer >= 0 && layer < MAX_TERRAIN_LAYERS)
			{
				materials[layer] = matId;
			}
		}
		for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
		{
			if (!materials[i].empty())
			{
				chunk->SetLayerMaterial(i, materials[i]);
			}
		}
	}

	int EditorTerrainSystem::ResolveGlobalLayer(const std::string& materialId)
	{
		auto it = m_MaterialLayerMap.find(materialId);
		if (it != m_MaterialLayerMap.end())
		{
			return it->second;
		}

		bool layerUsed[MAX_TERRAIN_LAYERS] = {};
		for (auto& [mat, layer] : m_MaterialLayerMap)
		{
			if (layer >= 0 && layer < MAX_TERRAIN_LAYERS)
			{
				layerUsed[layer] = true;
			}
		}
		for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
		{
			if (!layerUsed[i])
			{
				m_MaterialLayerMap[materialId] = i;
				return i;
			}
		}

		float globalSums[MAX_TERRAIN_LAYERS] = {};
		for (auto& [key, chunk] : m_Chunks)
		{
			const auto& data = chunk->GetData();
			if (data.splatmap.empty())
				continue;
			for (int i = 0; i < SPLATMAP_TEXELS; i++)
			{
				for (int c = 0; c < MAX_TERRAIN_LAYERS; c++)
				{
					globalSums[c] += data.splatmap[i * MAX_TERRAIN_LAYERS + c];
				}
			}
		}
		int minLayer = 0;
		float minSum = globalSums[0];
		for (int i = 1; i < MAX_TERRAIN_LAYERS; i++)
		{
			if (globalSums[i] < minSum)
			{
				minSum = globalSums[i];
				minLayer = i;
			}
		}

		for (auto mapIt = m_MaterialLayerMap.begin(); mapIt != m_MaterialLayerMap.end();)
		{
			if (mapIt->second == minLayer)
			{
				mapIt = m_MaterialLayerMap.erase(mapIt);
			}
			else
			{
				++mapIt;
			}
		}

		m_MaterialLayerMap[materialId] = minLayer;
		return minLayer;
	}

	void EditorTerrainSystem::SetNormalMode(bool sobel, bool smooth)
	{
		if (m_SobelNormals == sobel && m_SmoothNormals == smooth)
			return;

		m_SobelNormals = sobel;
		m_SmoothNormals = smooth;

		for (auto& [key, chunk] : m_Chunks)
		{
			chunk->SetNormalMode(sobel, smooth);
		}
	}

	void EditorTerrainSystem::SetDiamondGrid(bool enabled)
	{
		if (m_DiamondGrid == enabled)
			return;

		m_DiamondGrid = enabled;

		for (auto& [key, chunk] : m_Chunks)
		{
			chunk->SetDiamondGrid(enabled);
		}
	}

	void EditorTerrainSystem::SetMeshResolution(int resolution)
	{
		if (m_MeshResolution == resolution)
			return;

		m_MeshResolution = resolution;

		for (auto& [key, chunk] : m_Chunks)
		{
			chunk->SetMeshResolution(resolution);
		}
	}

	float EditorTerrainSystem::GetHeightAt(float worldX, float worldZ)
	{
		float height = 0.0f;
		GetHeightAt(worldX, worldZ, height);
		return height;
	}

	bool EditorTerrainSystem::GetHeightAt(float worldX, float worldZ, float& outHeight)
	{
		int32_t chunkX = WorldToChunkX(worldX);
		int32_t chunkZ = WorldToChunkZ(worldZ);

		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end() || it->second->GetState() != ChunkState::Active)
		{
			outHeight = 0.0f;
			return false;
		}

		float localX = worldX - chunkX * CHUNK_SIZE;
		float localZ = worldZ - chunkZ * CHUNK_SIZE;

		outHeight = it->second->GetHeightAt(localX, localZ);
		return true;
	}

	void EditorTerrainSystem::SetHeightAt(float worldX, float worldZ, float height)
	{
		int32_t chunkX = WorldToChunkX(worldX);
		int32_t chunkZ = WorldToChunkZ(worldZ);

		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end() || it->second->GetState() != ChunkState::Active)
			return;
		TerrainChunk* chunk = it->second.get();

		float localX = worldX - chunkX * CHUNK_SIZE;
		float localZ = worldZ - chunkZ * CHUNK_SIZE;

		int ix = static_cast<int>(localX / CHUNK_SIZE * (CHUNK_RESOLUTION - 1));
		int iz = static_cast<int>(localZ / CHUNK_SIZE * (CHUNK_RESOLUTION - 1));

		ix = std::clamp(ix, 0, CHUNK_RESOLUTION - 1);
		iz = std::clamp(iz, 0, CHUNK_RESOLUTION - 1);

		auto& data = chunk->GetDataMutable();
		if (data.heightmap.empty())
		{
			data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
		}

		data.heightmap[iz * CHUNK_RESOLUTION + ix] = height;
		data.CalculateBounds();
	}

	void EditorTerrainSystem::RaiseTerrain(float worldX, float worldZ, float radius, float amount)
	{
		TerrainBrush brush;
		brush.radius = radius;
		brush.strength = amount;

		ApplyBrush(worldX, worldZ, radius, [&brush, worldX, worldZ](TerrainChunk* chunk, int localX, int localZ, float weight) {
			auto& data = chunk->GetDataMutable();
			if (data.heightmap.empty())
			{
				data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
			}
			int idx = localZ * CHUNK_RESOLUTION + localX;
			data.heightmap[idx] += brush.GetWeight(weight);
		});
	}

	void EditorTerrainSystem::LowerTerrain(float worldX, float worldZ, float radius, float amount)
	{
		RaiseTerrain(worldX, worldZ, radius, -amount);
	}

	float EditorTerrainSystem::GetChunkHeight(int cx, int cz, int lx, int lz) const
	{
		const int last = CHUNK_RESOLUTION - 1;
		if (lx < 0)
		{
			cx--;
			lx = last + lx;
		}
		else if (lx > last)
		{
			cx++;
			lx = lx - last;
		}
		if (lz < 0)
		{
			cz--;
			lz = last + lz;
		}
		else if (lz > last)
		{
			cz++;
			lz = lz - last;
		}

		int32_t key = MakeChunkKey(cx, cz);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end())
			return 0.0f;
		const auto& data = it->second->GetData();
		if (data.heightmap.empty())
			return 0.0f;
		return data.heightmap[lz * CHUNK_RESOLUTION + lx];
	}

	void EditorTerrainSystem::SmoothTerrain(float worldX, float worldZ, float radius, float strength)
	{
		TerrainBrush brush;
		brush.radius = radius;
		brush.strength = strength;
		brush.falloff = TerrainBrush::Falloff::Smooth;

		struct SmoothVertex
		{
			TerrainChunk* chunk;
			int lx, lz;
			float average;
			float weight;
		};
		std::vector<SmoothVertex> vertices;

		int minChunkX = WorldToChunkX(worldX - radius);
		int maxChunkX = WorldToChunkX(worldX + radius);
		int minChunkZ = WorldToChunkZ(worldZ - radius);
		int maxChunkZ = WorldToChunkZ(worldZ + radius);

		if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ))
			return;

		for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
			{
				TerrainChunk* chunk = m_Chunks[MakeChunkKey(cx, cz)].get();

				auto& data = chunk->GetData();
				if (data.heightmap.empty())
					continue;

				float chunkWorldX = cx * CHUNK_SIZE;
				float chunkWorldZ = cz * CHUNK_SIZE;

				for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
				{
					for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
					{
						float vertWorldX = chunkWorldX + (lx / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;
						float vertWorldZ = chunkWorldZ + (lz / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;

						float dx = vertWorldX - worldX;
						float dz = vertWorldZ - worldZ;
						float dist = std::sqrt(dx * dx + dz * dz);

						if (dist > radius)
							continue;

						float sum = GetChunkHeight(cx, cz, lx - 1, lz) + GetChunkHeight(cx, cz, lx + 1, lz) + GetChunkHeight(cx, cz, lx, lz - 1) + GetChunkHeight(cx, cz, lx, lz + 1);
						float avg = sum * 0.25f;

						SmoothVertex sv;
						sv.chunk = chunk;
						sv.lx = lx;
						sv.lz = lz;
						sv.average = avg;
						sv.weight = brush.GetWeight(dist);
						vertices.push_back(sv);
					}
				}
			}
		}

		for (auto& sv : vertices)
		{
			auto& data = sv.chunk->GetDataMutable();
			int idx = sv.lz * CHUNK_RESOLUTION + sv.lx;
			data.heightmap[idx] = glm::mix(data.heightmap[idx], sv.average, sv.weight);
		}

		StitchEdges(minChunkX, maxChunkX, minChunkZ, maxChunkZ);

		for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
			{
				int32_t key = MakeChunkKey(cx, cz);
				auto it = m_Chunks.find(key);
				if (it != m_Chunks.end())
				{
					it->second->GetDataMutable().CalculateBounds();
				}
				DirtyNeighborChunks(cx, cz);
			}
		}
	}

	void EditorTerrainSystem::FlattenTerrain(float worldX, float worldZ, float radius, float targetHeight, float hardness)
	{
		float innerRadius = radius * std::clamp(hardness, 0.0f, 0.99f);
		float transitionWidth = radius - innerRadius;

		ApplyBrush(worldX, worldZ, radius, [targetHeight, innerRadius, transitionWidth](TerrainChunk* chunk, int localX, int localZ, float dist) {
			auto& data = chunk->GetDataMutable();
			if (data.heightmap.empty())
			{
				data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
			}
			int idx = localZ * CHUNK_RESOLUTION + localX;
			float currentHeight = data.heightmap[idx];

			if (dist <= innerRadius)
			{
				data.heightmap[idx] = targetHeight;
			}
			else
			{
				float t = (dist - innerRadius) / transitionWidth;
				t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
				float desired = glm::mix(targetHeight, currentHeight, t);
				if (std::abs(desired - targetHeight) < std::abs(currentHeight - targetHeight))
				{
					data.heightmap[idx] = desired;
				}
			}
		});
	}

	void EditorTerrainSystem::RampTerrain(float startX, float startZ, float startHeight,
										  float endX, float endZ, float endHeight, float width)
	{
		float dirX = endX - startX;
		float dirZ = endZ - startZ;
		float length = std::sqrt(dirX * dirX + dirZ * dirZ);
		if (length < 0.001f)
			return;

		float ndX = dirX / length;
		float ndZ = dirZ / length;
		float perpX = -ndZ;
		float perpZ = ndX;

		float minWX = std::min(startX, endX) - width;
		float maxWX = std::max(startX, endX) + width;
		float minWZ = std::min(startZ, endZ) - width;
		float maxWZ = std::max(startZ, endZ) + width;

		int minChunkX = WorldToChunkX(minWX);
		int maxChunkX = WorldToChunkX(maxWX);
		int minChunkZ = WorldToChunkZ(minWZ);
		int maxChunkZ = WorldToChunkZ(maxWZ);

		if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ))
			return;

		for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
			{
				TerrainChunk* chunk = m_Chunks[MakeChunkKey(cx, cz)].get();

				float chunkWorldX = cx * CHUNK_SIZE;
				float chunkWorldZ = cz * CHUNK_SIZE;

				for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
				{
					for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
					{
						float vertX = chunkWorldX + (lx / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;
						float vertZ = chunkWorldZ + (lz / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;

						float toVertX = vertX - startX;
						float toVertZ = vertZ - startZ;

						float t = (toVertX * ndX + toVertZ * ndZ) / length;
						float d = std::abs(toVertX * perpX + toVertZ * perpZ);

						if (t < 0.0f || t > 1.0f || d > width)
							continue;

						float targetH = startHeight + t * (endHeight - startHeight);

						float edgeFade = 1.0f;
						float fadeZone = width * 0.3f;
						if (d > width - fadeZone)
						{
							edgeFade = 1.0f - (d - (width - fadeZone)) / fadeZone;
						}

						auto& data = chunk->GetDataMutable();
						if (data.heightmap.empty())
						{
							data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
						}
						int idx = lz * CHUNK_RESOLUTION + lx;
						data.heightmap[idx] = glm::mix(data.heightmap[idx], targetH, edgeFade);
					}
				}

				chunk->GetDataMutable().CalculateBounds();
			}
		}

		StitchEdges(minChunkX, maxChunkX, minChunkZ, maxChunkZ);

		for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
			{
				DirtyNeighborChunks(cx, cz);
			}
		}
	}

	void EditorTerrainSystem::PaintTexture(float worldX, float worldZ, float radius, int layer, float strength)
	{
		if (layer < 0 || layer >= MAX_TERRAIN_LAYERS)
			return;
		PaintSplatmapLayer(worldX, worldZ, radius, layer, strength);
	}

	void EditorTerrainSystem::PaintMaterial(float worldX, float worldZ, float radius,
											const std::string& materialId, float strength)
	{
		if (materialId.empty())
			return;

		int globalLayer = ResolveGlobalLayer(materialId);

		int minChunkX = WorldToChunkX(worldX - radius);
		int maxChunkX = WorldToChunkX(worldX + radius);
		int minChunkZ = WorldToChunkZ(worldZ - radius);
		int maxChunkZ = WorldToChunkZ(worldZ + radius);

		if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ))
			return;

		for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
			{
				TerrainChunk* chunk = m_Chunks[MakeChunkKey(cx, cz)].get();
				if (chunk->FindMaterialLayer(materialId) != globalLayer)
				{
					chunk->SetLayerMaterial(globalLayer, materialId);
				}
			}
		}

		PaintSplatmapLayer(worldX, worldZ, radius, globalLayer, strength);
	}

	void EditorTerrainSystem::PaintSplatmapLayer(float worldX, float worldZ, float radius, int layer, float strength)
	{
		TerrainBrush brush;
		brush.radius = radius;
		brush.strength = strength;

		int minChunkX = WorldToChunkX(worldX - radius);
		int maxChunkX = WorldToChunkX(worldX + radius);
		int minChunkZ = WorldToChunkZ(worldZ - radius);
		int maxChunkZ = WorldToChunkZ(worldZ + radius);

		if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ))
			return;

		for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
			{
				TerrainChunk* chunk = m_Chunks[MakeChunkKey(cx, cz)].get();

				float chunkWorldX = cx * CHUNK_SIZE;
				float chunkWorldZ = cz * CHUNK_SIZE;

				auto& data = chunk->GetSplatmapMutable();
				if (data.splatmap.empty())
				{
					data.splatmap.resize(SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS, 0);
					for (int i = 0; i < SPLATMAP_TEXELS; i++)
					{
						data.splatmap[i * MAX_TERRAIN_LAYERS + 0] = 255;
					}
				}

				for (int sz = 0; sz < SPLATMAP_RESOLUTION; sz++)
				{
					for (int sx = 0; sx < SPLATMAP_RESOLUTION; sx++)
					{
						float texelWorldX = chunkWorldX + (sx + 0.5f) / (float)SPLATMAP_RESOLUTION * CHUNK_SIZE;
						float texelWorldZ = chunkWorldZ + (sz + 0.5f) / (float)SPLATMAP_RESOLUTION * CHUNK_SIZE;

						float dx = texelWorldX - worldX;
						float dz = texelWorldZ - worldZ;
						float dist = std::sqrt(dx * dx + dz * dz);
						if (dist > radius)
							continue;

						int idx = (sz * SPLATMAP_RESOLUTION + sx) * MAX_TERRAIN_LAYERS;
						float weight = brush.GetWeight(dist);

						float layerWeights[MAX_TERRAIN_LAYERS];
						for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
						{
							layerWeights[i] = data.splatmap[idx + i] / 255.0f;
						}

						layerWeights[layer] += weight;

						float total = 0.0f;
						for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
							total += layerWeights[i];
						if (total > 0.0f)
						{
							for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
							{
								data.splatmap[idx + i] = static_cast<uint8_t>((layerWeights[i] / total) * 255.0f);
							}
						}
					}
				}
			}
		}
	}

	void EditorTerrainSystem::SetHole(float worldX, float worldZ, bool isHole)
	{
		int32_t chunkX = WorldToChunkX(worldX);
		int32_t chunkZ = WorldToChunkZ(worldZ);

		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end() || it->second->GetState() != ChunkState::Active)
			return;
		TerrainChunk* chunk = it->second.get();

		float localX = worldX - chunkX * CHUNK_SIZE;
		float localZ = worldZ - chunkZ * CHUNK_SIZE;

		int holeX = static_cast<int>(localX / CHUNK_SIZE * HOLE_GRID_SIZE);
		int holeZ = static_cast<int>(localZ / CHUNK_SIZE * HOLE_GRID_SIZE);
		holeX = std::clamp(holeX, 0, HOLE_GRID_SIZE - 1);
		holeZ = std::clamp(holeZ, 0, HOLE_GRID_SIZE - 1);

		auto& data = chunk->GetDataMutable();
		uint64_t bit = 1ULL << (holeZ * HOLE_GRID_SIZE + holeX);

		if (isHole)
		{
			data.holeMask |= bit;
		}
		else
		{
			data.holeMask &= ~bit;
		}
	}

	void EditorTerrainSystem::EnsureChunkLoaded(int32_t chunkX, int32_t chunkZ)
	{
		TerrainChunk* chunk = GetOrCreateChunk(chunkX, chunkZ);
		if (chunk && chunk->GetState() != ChunkState::Active)
		{
			LoadChunkFromDisk(chunk);
			chunk->CreateGPUResources();
		}
	}

	TerrainChunk* EditorTerrainSystem::CreateChunk(int32_t chunkX, int32_t chunkZ)
	{
		return GetOrCreateChunk(chunkX, chunkZ);
	}

	void EditorTerrainSystem::DeleteChunk(int32_t chunkX, int32_t chunkZ)
	{
		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it != m_Chunks.end())
		{
			std::string path = GetChunkFilePath(chunkX, chunkZ);
			std::remove(path.c_str());
			m_Chunks.erase(it);
			m_KnownChunkFiles.erase(key);
		}
	}

	void EditorTerrainSystem::SaveDirtyChunks()
	{
		for (auto& [key, chunk] : m_Chunks)
		{
			if (chunk->IsModified())
			{
				SaveChunkToDisk(chunk.get());
				chunk->ClearModified();
			}
		}
	}

	void EditorTerrainSystem::SaveChunk(int32_t chunkX, int32_t chunkZ)
	{
		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it != m_Chunks.end())
		{
			SaveChunkToDisk(it->second.get());
			it->second->ClearModified();
		}
	}

	void EditorTerrainSystem::BeginEdit(float worldX, float worldZ, float radius)
	{
		m_CurrentEditSnapshot = std::make_unique<EditSnapshot>();

		int minChunkX = WorldToChunkX(worldX - radius);
		int maxChunkX = WorldToChunkX(worldX + radius);
		int minChunkZ = WorldToChunkZ(worldZ - radius);
		int maxChunkZ = WorldToChunkZ(worldZ + radius);

		for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
			{
				int32_t key = MakeChunkKey(cx, cz);
				auto it = m_Chunks.find(key);
				if (it != m_Chunks.end())
				{
					m_CurrentEditSnapshot->chunkSnapshots.emplace_back(key, it->second->GetData());
				}
			}
		}
	}

	void EditorTerrainSystem::EndEdit()
	{
		m_CurrentEditSnapshot.reset();
	}

	void EditorTerrainSystem::CancelEdit()
	{
		if (!m_CurrentEditSnapshot)
			return;

		for (auto& [key, data] : m_CurrentEditSnapshot->chunkSnapshots)
		{
			auto it = m_Chunks.find(key);
			if (it != m_Chunks.end())
			{
				it->second->LoadFromData(data);
			}
		}

		m_CurrentEditSnapshot.reset();
	}

	float EditorTerrainSystem::CalculateChunkDistance(int32_t chunkX, int32_t chunkZ) const
	{
		float centerX = (chunkX + 0.5f) * CHUNK_SIZE;
		float centerZ = (chunkZ + 0.5f) * CHUNK_SIZE;
		float dx = centerX - m_CameraPosition.x;
		float dz = centerZ - m_CameraPosition.z;
		return std::sqrt(dx * dx + dz * dz);
	}

	bool EditorTerrainSystem::IsChunkVisible(int32_t chunkX, int32_t chunkZ) const
	{
		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end())
			return false;

		return m_Frustum.IsBoxVisible(it->second->GetBoundsMin(), it->second->GetBoundsMax());
	}

	void EditorTerrainSystem::DirtyNeighborChunks(int32_t cx, int32_t cz)
	{
		static const int offsets[][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
		for (auto& off : offsets)
		{
			auto it = m_Chunks.find(MakeChunkKey(cx + off[0], cz + off[1]));
			if (it != m_Chunks.end() && it->second->GetState() == ChunkState::Active)
			{
				it->second->MarkMeshDirty();
			}
		}
	}

	void EditorTerrainSystem::ProcessLoadQueue()
	{
		int loaded = 0;
		while (!m_LoadQueue.empty() && loaded < m_Settings.maxChunksPerFrame)
		{
			auto req = m_LoadQueue.front();
			m_LoadQueue.pop_front();

			int32_t key = MakeChunkKey(req.chunkX, req.chunkZ);
			if (m_Chunks.find(key) != m_Chunks.end())
				continue;

			auto chunk = std::make_unique<TerrainChunk>(req.chunkX, req.chunkZ);
			chunk->SetNormalMode(m_SobelNormals, m_SmoothNormals);
			chunk->SetDiamondGrid(m_DiamondGrid);
			chunk->SetMeshResolution(m_MeshResolution);
			chunk->SetHeightSampler([this](int cx, int cz, int lx, int lz) {
				return GetChunkHeight(cx, cz, lx, lz);
			});
			LoadChunkFromDisk(chunk.get());
			ApplyDefaultMaterials(chunk.get());
			m_Chunks[key] = std::move(chunk);
			CopyBoundaryFromNeighbors(m_Chunks[key].get());
			m_Chunks[key]->CreateGPUResources();
			m_Chunks[key]->ClearModified();
			DirtyNeighborChunks(req.chunkX, req.chunkZ);
			loaded++;
		}
	}

	void EditorTerrainSystem::UnloadDistantChunks()
	{
		std::vector<int32_t> toUnload;

		for (auto& [key, chunk] : m_Chunks)
		{
			float dist = CalculateChunkDistance(chunk->GetChunkX(), chunk->GetChunkZ());
			if (dist > m_Settings.unloadDistance)
			{
				toUnload.push_back(key);
			}
		}

		for (int32_t key : toUnload)
		{
			auto& chunk = m_Chunks[key];
			if (chunk->IsModified())
			{
				SaveChunkToDisk(chunk.get());
				chunk->ClearModified();
			}
			m_Chunks.erase(key);
		}
	}

	std::string EditorTerrainSystem::GetChunkFilePath(int32_t chunkX, int32_t chunkZ) const
	{
		return m_ProjectPath + "/chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".terrain";
	}

	void EditorTerrainSystem::LoadChunkFromDisk(TerrainChunk* chunk)
	{
		std::string path = GetChunkFilePath(chunk->GetChunkX(), chunk->GetChunkZ());
		chunk->Load(path);

		auto& data = chunk->GetDataMutable();

		for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
		{
			if (!data.materialIds[i].empty())
			{
				if (m_MaterialLayerMap.find(data.materialIds[i]) == m_MaterialLayerMap.end())
				{
					m_MaterialLayerMap[data.materialIds[i]] = i;
				}
			}
		}

		if (!data.splatmap.empty())
		{
			int remap[MAX_TERRAIN_LAYERS];
			for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
				remap[i] = i;
			bool needsRemap = false;

			for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
			{
				if (!data.materialIds[i].empty())
				{
					auto it = m_MaterialLayerMap.find(data.materialIds[i]);
					if (it != m_MaterialLayerMap.end() && it->second != i)
					{
						remap[i] = it->second;
						needsRemap = true;
					}
				}
			}

			if (needsRemap)
			{
				std::vector<uint8_t> newSplatmap(data.splatmap.size(), 0);
				for (int i = 0; i < SPLATMAP_TEXELS; i++)
				{
					for (int c = 0; c < MAX_TERRAIN_LAYERS; c++)
					{
						newSplatmap[i * MAX_TERRAIN_LAYERS + remap[c]] = data.splatmap[i * MAX_TERRAIN_LAYERS + c];
					}
				}
				data.splatmap = std::move(newSplatmap);

				std::string newIds[MAX_TERRAIN_LAYERS];
				for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
				{
					newIds[remap[i]] = data.materialIds[i];
				}
				for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
				{
					data.materialIds[i] = newIds[i];
				}

				chunk->ClearModified();
			}
		}
	}

	void EditorTerrainSystem::SaveChunkToDisk(TerrainChunk* chunk)
	{
		std::string path = GetChunkFilePath(chunk->GetChunkX(), chunk->GetChunkZ());

		std::filesystem::create_directories(m_ProjectPath);

		const auto& data = chunk->GetData();

		if (data.heightmap.size() < CHUNK_HEIGHTMAP_SIZE)
			return;
		if (data.splatmap.size() < (size_t)(SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS))
			return;

		std::ofstream file(path, std::ios::binary);
		if (!file.is_open())
			return;

		m_KnownChunkFiles.insert(MakeChunkKey(chunk->GetChunkX(), chunk->GetChunkZ()));

		uint32_t magic = MMO::TERR_TAG;
		uint32_t version = MMO::TERRAIN_FORMAT_VERSION;
		file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
		file.write(reinterpret_cast<const char*>(&version), sizeof(version));
		file.write(reinterpret_cast<const char*>(&data.chunkX), sizeof(data.chunkX));
		file.write(reinterpret_cast<const char*>(&data.chunkZ), sizeof(data.chunkZ));

		file.write(reinterpret_cast<const char*>(data.heightmap.data()),
				   CHUNK_HEIGHTMAP_SIZE * sizeof(float));

		file.write(reinterpret_cast<const char*>(data.splatmap.data()),
				   SPLATMAP_TEXELS * MAX_TERRAIN_LAYERS);

		file.write(reinterpret_cast<const char*>(&data.holeMask), sizeof(data.holeMask));

		file.write(reinterpret_cast<const char*>(&data.minHeight), sizeof(data.minHeight));
		file.write(reinterpret_cast<const char*>(&data.maxHeight), sizeof(data.maxHeight));

		for (int i = 0; i < MAX_TERRAIN_LAYERS; i++)
		{
			MMO::WriteString(file, data.materialIds[i]);
		}

		file.close();
	}

	TerrainChunk* EditorTerrainSystem::GetOrCreateChunk(int32_t chunkX, int32_t chunkZ)
	{
		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it != m_Chunks.end())
		{
			return it->second.get();
		}

		auto chunk = std::make_unique<TerrainChunk>(chunkX, chunkZ);
		chunk->SetNormalMode(m_SobelNormals, m_SmoothNormals);
		chunk->SetDiamondGrid(m_DiamondGrid);
		chunk->SetMeshResolution(m_MeshResolution);
		chunk->SetHeightSampler([this](int cx, int cz, int lx, int lz) {
			return GetChunkHeight(cx, cz, lx, lz);
		});
		LoadChunkFromDisk(chunk.get());
		ApplyDefaultMaterials(chunk.get());
		CopyBoundaryFromNeighbors(chunk.get());
		chunk->CreateGPUResources();
		chunk->ClearModified();

		m_KnownChunkFiles.insert(key);

		TerrainChunk* ptr = chunk.get();
		m_Chunks[key] = std::move(chunk);
		DirtyNeighborChunks(chunkX, chunkZ);
		return ptr;
	}

	bool EditorTerrainSystem::EnsureChunksReady(int minCX, int maxCX, int minCZ, int maxCZ)
	{
		for (int cz = minCZ; cz <= maxCZ; cz++)
		{
			for (int cx = minCX; cx <= maxCX; cx++)
			{
				TerrainChunk* chunk = GetOrCreateChunk(cx, cz);
				if (!chunk || chunk->GetState() != ChunkState::Active)
				{
					return false;
				}
			}
		}
		return true;
	}

	void EditorTerrainSystem::StitchEdges(int minCX, int maxCX, int minCZ, int maxCZ)
	{
		for (int cz = minCZ; cz <= maxCZ; cz++)
		{
			for (int cx = minCX; cx <= maxCX; cx++)
			{
				int32_t key = MakeChunkKey(cx, cz);
				auto it = m_Chunks.find(key);
				if (it == m_Chunks.end())
					continue;
				auto& data = it->second->GetDataMutable();
				if (data.heightmap.empty())
					continue;

				if (cx + 1 <= maxCX)
				{
					auto rightIt = m_Chunks.find(MakeChunkKey(cx + 1, cz));
					if (rightIt != m_Chunks.end() && !rightIt->second->GetData().heightmap.empty())
					{
						auto& rdata = rightIt->second->GetDataMutable();
						for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
						{
							int thisIdx = lz * CHUNK_RESOLUTION + (CHUNK_RESOLUTION - 1);
							int rightIdx = lz * CHUNK_RESOLUTION;
							float avg = (data.heightmap[thisIdx] + rdata.heightmap[rightIdx]) * 0.5f;
							data.heightmap[thisIdx] = avg;
							rdata.heightmap[rightIdx] = avg;
						}
					}
				}

				if (cz + 1 <= maxCZ)
				{
					auto bottomIt = m_Chunks.find(MakeChunkKey(cx, cz + 1));
					if (bottomIt != m_Chunks.end() && !bottomIt->second->GetData().heightmap.empty())
					{
						auto& bdata = bottomIt->second->GetDataMutable();
						for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
						{
							int thisIdx = (CHUNK_RESOLUTION - 1) * CHUNK_RESOLUTION + lx;
							int bottomIdx = lx;
							float avg = (data.heightmap[thisIdx] + bdata.heightmap[bottomIdx]) * 0.5f;
							data.heightmap[thisIdx] = avg;
							bdata.heightmap[bottomIdx] = avg;
						}
					}
				}
			}
		}
	}

	void EditorTerrainSystem::CopyBoundaryFromNeighbors(TerrainChunk* chunk)
	{
		auto& data = chunk->GetDataMutable();
		if (data.heightmap.empty())
		{
			data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
		}

		int32_t cx = chunk->GetChunkX();
		int32_t cz = chunk->GetChunkZ();
		const int last = CHUNK_RESOLUTION - 1;

		auto leftIt = m_Chunks.find(MakeChunkKey(cx - 1, cz));
		if (leftIt != m_Chunks.end())
		{
			const auto& nd = leftIt->second->GetData();
			if (!nd.heightmap.empty())
			{
				for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
					data.heightmap[lz * CHUNK_RESOLUTION + 0] = nd.heightmap[lz * CHUNK_RESOLUTION + last];
			}
		}

		auto rightIt = m_Chunks.find(MakeChunkKey(cx + 1, cz));
		if (rightIt != m_Chunks.end())
		{
			const auto& nd = rightIt->second->GetData();
			if (!nd.heightmap.empty())
			{
				for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
					data.heightmap[lz * CHUNK_RESOLUTION + last] = nd.heightmap[lz * CHUNK_RESOLUTION + 0];
			}
		}

		auto topIt = m_Chunks.find(MakeChunkKey(cx, cz - 1));
		if (topIt != m_Chunks.end())
		{
			const auto& nd = topIt->second->GetData();
			if (!nd.heightmap.empty())
			{
				for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
					data.heightmap[0 * CHUNK_RESOLUTION + lx] = nd.heightmap[last * CHUNK_RESOLUTION + lx];
			}
		}

		auto bottomIt = m_Chunks.find(MakeChunkKey(cx, cz + 1));
		if (bottomIt != m_Chunks.end())
		{
			const auto& nd = bottomIt->second->GetData();
			if (!nd.heightmap.empty())
			{
				for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
					data.heightmap[last * CHUNK_RESOLUTION + lx] = nd.heightmap[0 * CHUNK_RESOLUTION + lx];
			}
		}

		data.CalculateBounds();
	}

	void EditorTerrainSystem::ApplyBrush(float worldX, float worldZ, float radius,
										 std::function<void(TerrainChunk*, int localX, int localZ, float weight)> operation)
	{

		int minChunkX = WorldToChunkX(worldX - radius);
		int maxChunkX = WorldToChunkX(worldX + radius);
		int minChunkZ = WorldToChunkZ(worldZ - radius);
		int maxChunkZ = WorldToChunkZ(worldZ + radius);

		if (!EnsureChunksReady(minChunkX, maxChunkX, minChunkZ, maxChunkZ))
			return;

		for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
			{
				int32_t key = MakeChunkKey(cx, cz);
				TerrainChunk* chunk = m_Chunks[key].get();

				float chunkWorldX = cx * CHUNK_SIZE;
				float chunkWorldZ = cz * CHUNK_SIZE;

				for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
				{
					for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
					{
						float vertWorldX = chunkWorldX + (lx / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;
						float vertWorldZ = chunkWorldZ + (lz / (float)(CHUNK_RESOLUTION - 1)) * CHUNK_SIZE;

						float dx = vertWorldX - worldX;
						float dz = vertWorldZ - worldZ;
						float dist = std::sqrt(dx * dx + dz * dz);

						operation(chunk, lx, lz, dist);
					}
				}

				chunk->GetDataMutable().CalculateBounds();
			}
		}

		StitchEdges(minChunkX, maxChunkX, minChunkZ, maxChunkZ);

		for (int cz = minChunkZ; cz <= maxChunkZ; cz++)
		{
			for (int cx = minChunkX; cx <= maxChunkX; cx++)
			{
				DirtyNeighborChunks(cx, cz);
			}
		}
	}

} // namespace Editor3D
