#include "EditorWorldSystem.h"
#include "../Export/MigrationSqlWriter.h"
#include "EditorWorld.h"
#include <Core/Application.h>
#include <Graphics/AssetManager.h>
#include <Model/OmdlFormat.h>
#include <Model/OmdlWriter.h>
#include <Terrain/ChunkFileWriter.h>
#include <World/PlayerSpawn.h>
#include <World/SpawnPoint.h>
#include <World/StaticObject.h>
#include <World/WorldObjectData.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

using Shared::WorldToChunkX;
using Shared::WorldToChunkZ;

namespace Editor3D {

	EditorWorldSystem::EditorWorldSystem()
	{
		m_MeshGenThread = std::thread(&EditorWorldSystem::MeshGenThreadFunc, this);
	}

	EditorWorldSystem::~EditorWorldSystem()
	{
		Shutdown();
	}

	void EditorWorldSystem::Init(const std::string& chunksDirectory)
	{
		m_ProjectPath = chunksDirectory.empty() ? "." : chunksDirectory;

		std::filesystem::create_directories(m_ProjectPath);

		if (std::filesystem::exists(m_ProjectPath))
		{
			for (const auto& entry : std::filesystem::directory_iterator(m_ProjectPath))
			{
				if (entry.path().extension() != ".chunk")
					continue;

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

	void EditorWorldSystem::Shutdown()
	{
		// Stop background mesh generation thread
		m_ShutdownMeshGen.store(true);
		m_MeshGenCV.notify_one();
		if (m_MeshGenThread.joinable())
			m_MeshGenThread.join();

		SaveDirtyChunks();
		m_Chunks.clear();
		m_LoadQueue.clear();
		m_KnownChunkFiles.clear();
		m_MaterialLayerMap.clear();
		m_ObjectChunkMap.clear();
		m_PreloadedChunkKeys.clear();
	}

	void EditorWorldSystem::Update(const glm::vec3& cameraPos, const glm::mat4& viewProj, float deltaTime)
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
		PreloadNearbyModels();

		// Queue any Loaded chunks for background mesh generation
		for (auto& [key, chunk] : m_Chunks)
		{
			TerrainChunk* terrain = chunk->GetTerrain();
			if (terrain->GetState() == ChunkState::Loaded)
			{
				QueueMeshGeneration(key, terrain, false);
			}
		}

		// Upload ready meshes to GPU (fast — only GL calls, no CPU mesh work)
		ProcessReadyMeshes();

		m_Stats.totalChunks = (int)m_Chunks.size();
		m_Stats.loadedChunks = 0;
		m_Stats.visibleChunks = 0;
		m_Stats.dirtyChunks = 0;

		for (auto& [key, chunk] : m_Chunks)
		{
			auto state = chunk->GetTerrain()->GetState();
			if (state == ChunkState::Active)
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

	void EditorWorldSystem::RenderTerrain(Shader* terrainShader, const glm::mat4& viewProj,
										  PerChunkCallback perChunkSetup)
	{
		if (!terrainShader)
			return;

		static const glm::mat4 identity(1.0f);
		terrainShader->SetMat4("u_Model", identity);

		int dirtyRegens = 0;
		for (auto& [key, chunk] : m_Chunks)
		{
			TerrainChunk* terrain = chunk->GetTerrain();
			if (terrain->GetState() != ChunkState::Active)
				continue;
			if (!m_Settings.enableFrustumCulling || IsChunkVisible(chunk->GetChunkX(), chunk->GetChunkZ()))
			{
				if (perChunkSetup)
				{
					perChunkSetup(terrain, terrainShader);
				}
				// Throttle dirty mesh regeneration to max 2 per frame
				bool allowRegen = true;
				if (terrain->IsDirty())
				{
					allowRegen = (dirtyRegens < 2);
					if (allowRegen)
						dirtyRegens++;
				}
				terrain->Draw(terrainShader, allowRegen);
			}
		}
	}

	std::vector<EditorLight> EditorWorldSystem::GatherVisibleLights(const glm::vec3& cameraPos, float maxDistance) const
	{
		std::vector<EditorLight> result;

		for (const auto& [key, chunk] : m_Chunks)
		{
			if (!chunk->IsReady())
				continue;

			for (const auto& light : chunk->GetLights())
			{
				float dist = glm::distance(light.position, cameraPos);
				if (dist <= light.range + maxDistance)
				{
					result.push_back(light);
				}
			}
		}

		// Sort by distance to camera
		std::sort(result.begin(), result.end(),
				  [&cameraPos](const EditorLight& a, const EditorLight& b) {
					  return glm::distance(a.position, cameraPos) < glm::distance(b.position, cameraPos);
				  });

		return result;
	}

	WorldChunk* EditorWorldSystem::GetChunk(int32_t cx, int32_t cz)
	{
		int32_t key = MakeChunkKey(cx, cz);
		auto it = m_Chunks.find(key);
		return (it != m_Chunks.end()) ? it->second.get() : nullptr;
	}

	void EditorWorldSystem::SetDefaultMaterialIds(const std::string ids[MAX_TERRAIN_LAYERS])
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

	void EditorWorldSystem::ApplyDefaultMaterials(WorldChunk* chunk)
	{
		TerrainChunk* terrain = chunk->GetTerrain();
		const auto& data = terrain->GetData();
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
				terrain->SetLayerMaterial(i, materials[i]);
			}
		}
	}

	int EditorWorldSystem::ResolveGlobalLayer(const std::string& materialId)
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
			const auto& data = chunk->GetTerrain()->GetData();
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

	void EditorWorldSystem::SetNormalMode(bool sobel, bool smooth)
	{
		if (m_SobelNormals == sobel && m_SmoothNormals == smooth)
			return;
		m_SobelNormals = sobel;
		m_SmoothNormals = smooth;
		for (auto& [key, chunk] : m_Chunks)
		{
			chunk->GetTerrain()->SetNormalMode(sobel, smooth);
		}
	}

	void EditorWorldSystem::SetDiamondGrid(bool enabled)
	{
		if (m_DiamondGrid == enabled)
			return;
		m_DiamondGrid = enabled;
		for (auto& [key, chunk] : m_Chunks)
		{
			chunk->GetTerrain()->SetDiamondGrid(enabled);
		}
	}

	void EditorWorldSystem::SetMeshResolution(int resolution)
	{
		if (m_MeshResolution == resolution)
			return;
		m_MeshResolution = resolution;
		for (auto& [key, chunk] : m_Chunks)
		{
			chunk->GetTerrain()->SetMeshResolution(resolution);
		}
	}

	float EditorWorldSystem::GetHeightAt(float worldX, float worldZ)
	{
		float height = 0.0f;
		GetHeightAt(worldX, worldZ, height);
		return height;
	}

	bool EditorWorldSystem::GetHeightAt(float worldX, float worldZ, float& outHeight)
	{
		int32_t chunkX = WorldToChunkX(worldX);
		int32_t chunkZ = WorldToChunkZ(worldZ);

		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end() || it->second->GetTerrain()->GetState() != ChunkState::Active)
		{
			outHeight = 0.0f;
			return false;
		}

		float localX = worldX - chunkX * CHUNK_SIZE;
		float localZ = worldZ - chunkZ * CHUNK_SIZE;

		outHeight = it->second->GetTerrain()->GetHeightAt(localX, localZ);
		return true;
	}

	void EditorWorldSystem::SetHeightAt(float worldX, float worldZ, float height)
	{
		int32_t chunkX = WorldToChunkX(worldX);
		int32_t chunkZ = WorldToChunkZ(worldZ);

		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end() || it->second->GetTerrain()->GetState() != ChunkState::Active)
			return;
		TerrainChunk* terrain = it->second->GetTerrain();

		float localX = worldX - chunkX * CHUNK_SIZE;
		float localZ = worldZ - chunkZ * CHUNK_SIZE;

		int ix = static_cast<int>(localX / CHUNK_SIZE * (CHUNK_RESOLUTION - 1));
		int iz = static_cast<int>(localZ / CHUNK_SIZE * (CHUNK_RESOLUTION - 1));
		ix = std::clamp(ix, 0, CHUNK_RESOLUTION - 1);
		iz = std::clamp(iz, 0, CHUNK_RESOLUTION - 1);

		auto& data = terrain->GetDataMutable();
		if (data.heightmap.empty())
		{
			data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
		}
		data.heightmap[iz * CHUNK_RESOLUTION + ix] = height;
		data.CalculateBounds();
	}

	void EditorWorldSystem::RaiseTerrain(float worldX, float worldZ, float radius, float amount)
	{
		TerrainBrush brush;
		brush.radius = radius;
		brush.strength = amount;

		ApplyBrush(worldX, worldZ, radius, [&brush](TerrainChunk* chunk, int localX, int localZ, float weight) {
			auto& data = chunk->GetDataMutable();
			if (data.heightmap.empty())
			{
				data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
			}
			int idx = localZ * CHUNK_RESOLUTION + localX;
			data.heightmap[idx] += brush.GetWeight(weight);
		});
	}

	void EditorWorldSystem::LowerTerrain(float worldX, float worldZ, float radius, float amount)
	{
		RaiseTerrain(worldX, worldZ, radius, -amount);
	}

	float EditorWorldSystem::GetChunkHeight(int cx, int cz, int lx, int lz) const
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
		const auto& data = it->second->GetTerrain()->GetData();
		if (data.heightmap.empty())
			return 0.0f;
		return data.heightmap[lz * CHUNK_RESOLUTION + lx];
	}

	void EditorWorldSystem::SmoothTerrain(float worldX, float worldZ, float radius, float strength)
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
				TerrainChunk* terrain = m_Chunks[MakeChunkKey(cx, cz)]->GetTerrain();
				auto& data = terrain->GetData();
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
						sv.chunk = terrain;
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
					it->second->GetTerrain()->GetDataMutable().CalculateBounds();
				}
				DirtyNeighborChunks(cx, cz);
			}
		}
	}

	void EditorWorldSystem::FlattenTerrain(float worldX, float worldZ, float radius, float targetHeight, float hardness)
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

	void EditorWorldSystem::RampTerrain(float startX, float startZ, float startHeight,
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
				TerrainChunk* terrain = m_Chunks[MakeChunkKey(cx, cz)]->GetTerrain();

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

						auto& data = terrain->GetDataMutable();
						if (data.heightmap.empty())
						{
							data.heightmap.resize(CHUNK_HEIGHTMAP_SIZE, 0.0f);
						}
						int idx = lz * CHUNK_RESOLUTION + lx;
						data.heightmap[idx] = glm::mix(data.heightmap[idx], targetH, edgeFade);
					}
				}

				terrain->GetDataMutable().CalculateBounds();
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

	void EditorWorldSystem::PaintTexture(float worldX, float worldZ, float radius, int layer, float strength)
	{
		if (layer < 0 || layer >= MAX_TERRAIN_LAYERS)
			return;
		PaintSplatmapLayer(worldX, worldZ, radius, layer, strength);
	}

	void EditorWorldSystem::PaintMaterial(float worldX, float worldZ, float radius,
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
				TerrainChunk* terrain = m_Chunks[MakeChunkKey(cx, cz)]->GetTerrain();
				if (terrain->FindMaterialLayer(materialId) != globalLayer)
				{
					terrain->SetLayerMaterial(globalLayer, materialId);
				}
			}
		}

		PaintSplatmapLayer(worldX, worldZ, radius, globalLayer, strength);
	}

	void EditorWorldSystem::PaintSplatmapLayer(float worldX, float worldZ, float radius, int layer, float strength)
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
				TerrainChunk* terrain = m_Chunks[MakeChunkKey(cx, cz)]->GetTerrain();

				float chunkWorldX = cx * CHUNK_SIZE;
				float chunkWorldZ = cz * CHUNK_SIZE;

				auto& data = terrain->GetSplatmapMutable();
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

	void EditorWorldSystem::SetHole(float worldX, float worldZ, bool isHole)
	{
		int32_t chunkX = WorldToChunkX(worldX);
		int32_t chunkZ = WorldToChunkZ(worldZ);

		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end() || it->second->GetTerrain()->GetState() != ChunkState::Active)
			return;
		TerrainChunk* terrain = it->second->GetTerrain();

		float localX = worldX - chunkX * CHUNK_SIZE;
		float localZ = worldZ - chunkZ * CHUNK_SIZE;

		int holeX = static_cast<int>(localX / CHUNK_SIZE * HOLE_GRID_SIZE);
		int holeZ = static_cast<int>(localZ / CHUNK_SIZE * HOLE_GRID_SIZE);
		holeX = std::clamp(holeX, 0, HOLE_GRID_SIZE - 1);
		holeZ = std::clamp(holeZ, 0, HOLE_GRID_SIZE - 1);

		auto& data = terrain->GetDataMutable();
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

	void EditorWorldSystem::EnsureChunkLoaded(int32_t chunkX, int32_t chunkZ)
	{
		WorldChunk* chunk = GetOrCreateChunk(chunkX, chunkZ);
		if (chunk && chunk->GetTerrain()->GetState() != ChunkState::Active)
		{
			LoadChunkFromDisk(chunk);
			chunk->GetTerrain()->CreateGPUResources();
		}
	}

	WorldChunk* EditorWorldSystem::CreateChunk(int32_t chunkX, int32_t chunkZ)
	{
		return GetOrCreateChunk(chunkX, chunkZ);
	}

	void EditorWorldSystem::DeleteChunk(int32_t chunkX, int32_t chunkZ)
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

	void EditorWorldSystem::SaveDirtyChunks()
	{
		// Gather objects from EditorWorld into chunks before checking dirty flags.
		// This ensures chunks that gained/lost objects get marked dirty.
		if (m_EditorWorld)
		{
			// Find all chunks that currently contain objects
			std::unordered_set<int32_t> affectedChunks;
			for (const auto& obj : m_EditorWorld->GetStaticObjects())
			{
				int32_t cx = WorldToChunkX(obj->GetPosition().x);
				int32_t cz = WorldToChunkZ(obj->GetPosition().z);
				affectedChunks.insert(MakeChunkKey(cx, cz));
			}
			// Also include chunks that previously had objects (object may have moved away)
			for (const auto& [guid, chunkKey] : m_ObjectChunkMap)
			{
				affectedChunks.insert(chunkKey);
			}
			// Gather objects for affected chunks, auto-creating if needed
			for (int32_t key : affectedChunks)
			{
				auto it = m_Chunks.find(key);
				if (it == m_Chunks.end())
				{
					// Object in the void — create a default chunk to hold it
					int32_t cx = static_cast<int16_t>(static_cast<uint32_t>(key) >> 16);
					int32_t cz = static_cast<int16_t>(static_cast<uint32_t>(key) & 0xFFFF);
					GetOrCreateChunk(cx, cz);
					it = m_Chunks.find(key);
				}
				if (it != m_Chunks.end())
				{
					GatherObjectsForChunk(it->second.get());
				}
			}
		}

		for (auto& [key, chunk] : m_Chunks)
		{
			if (chunk->IsModified())
			{
				SaveChunkToDisk(chunk.get());
				chunk->ClearModified();
			}
		}
	}

	void EditorWorldSystem::SaveChunk(int32_t chunkX, int32_t chunkZ)
	{
		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it != m_Chunks.end())
		{
			SaveChunkToDisk(it->second.get());
			it->second->ClearModified();
		}
	}

	void EditorWorldSystem::BeginEdit(float worldX, float worldZ, float radius)
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
					m_CurrentEditSnapshot->chunkSnapshots.emplace_back(key, it->second->GetTerrain()->GetData());
				}
			}
		}
	}

	void EditorWorldSystem::EndEdit()
	{
		m_CurrentEditSnapshot.reset();
	}

	void EditorWorldSystem::CancelEdit()
	{
		if (!m_CurrentEditSnapshot)
			return;

		for (auto& [key, data] : m_CurrentEditSnapshot->chunkSnapshots)
		{
			auto it = m_Chunks.find(key);
			if (it != m_Chunks.end())
			{
				it->second->GetTerrain()->LoadFromData(data);
			}
		}

		m_CurrentEditSnapshot.reset();
	}

	float EditorWorldSystem::CalculateChunkDistance(int32_t chunkX, int32_t chunkZ) const
	{
		float centerX = (chunkX + 0.5f) * CHUNK_SIZE;
		float centerZ = (chunkZ + 0.5f) * CHUNK_SIZE;
		float dx = centerX - m_CameraPosition.x;
		float dz = centerZ - m_CameraPosition.z;
		return std::sqrt(dx * dx + dz * dz);
	}

	bool EditorWorldSystem::IsChunkVisible(int32_t chunkX, int32_t chunkZ) const
	{
		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end())
			return false;

		TerrainChunk* terrain = it->second->GetTerrain();
		return m_Frustum.IsBoxVisible(terrain->GetBoundsMin(), terrain->GetBoundsMax());
	}

	void EditorWorldSystem::DirtyNeighborChunks(int32_t cx, int32_t cz)
	{
		static const int offsets[][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
		for (auto& off : offsets)
		{
			auto it = m_Chunks.find(MakeChunkKey(cx + off[0], cz + off[1]));
			if (it != m_Chunks.end() && it->second->GetTerrain()->GetState() == ChunkState::Active)
			{
				it->second->GetTerrain()->MarkMeshDirty();
			}
		}
	}

	void EditorWorldSystem::ProcessLoadQueue()
	{
		int loaded = 0;
		while (!m_LoadQueue.empty() && loaded < m_Settings.maxChunksPerFrame)
		{
			auto req = m_LoadQueue.front();
			m_LoadQueue.pop_front();

			int32_t key = MakeChunkKey(req.chunkX, req.chunkZ);
			if (m_Chunks.find(key) != m_Chunks.end())
				continue;

			auto chunk = std::make_unique<WorldChunk>(req.chunkX, req.chunkZ);
			TerrainChunk* terrain = chunk->GetTerrain();
			terrain->SetNormalMode(m_SobelNormals, m_SmoothNormals);
			terrain->SetDiamondGrid(m_DiamondGrid);
			terrain->SetMeshResolution(m_MeshResolution);
			terrain->SetHeightSampler([this](int cx, int cz, int lx, int lz) {
				return GetChunkHeight(cx, cz, lx, lz);
			});
			LoadChunkFromDisk(chunk.get()); // Fast: just disk I/O (~1ms)
			ApplyDefaultMaterials(chunk.get());
			m_Chunks[key] = std::move(chunk);
			CopyBoundaryFromNeighbors(m_Chunks[key].get());
			// Don't call CreateGPUResources() here — mesh generation is queued
			// to background thread by Update(). GPU upload happens in ProcessReadyMeshes().
			m_Chunks[key]->ClearModified();
			DirtyNeighborChunks(req.chunkX, req.chunkZ);
			loaded++;
		}
	}

	void EditorWorldSystem::UnloadDistantChunks()
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
			RemoveChunkObjects(key);
			m_Chunks.erase(key);
		}
	}

	std::string EditorWorldSystem::GetChunkFilePath(int32_t chunkX, int32_t chunkZ) const
	{
		return m_ProjectPath + "/chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".chunk";
	}

	void EditorWorldSystem::LoadChunkFromDisk(WorldChunk* chunk)
	{
		std::string path = GetChunkFilePath(chunk->GetChunkX(), chunk->GetChunkZ());
		chunk->Load(path);
		SpawnObjectsFromChunk(chunk);

		TerrainChunk* terrain = chunk->GetTerrain();
		auto& data = terrain->GetDataMutable();

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

				terrain->ClearModified();
			}
		}
	}

	void EditorWorldSystem::SaveChunkToDisk(WorldChunk* chunk)
	{
		GatherObjectsForChunk(chunk);
		std::string path = GetChunkFilePath(chunk->GetChunkX(), chunk->GetChunkZ());
		chunk->Save(path);
		m_KnownChunkFiles.insert(MakeChunkKey(chunk->GetChunkX(), chunk->GetChunkZ()));
	}

	// ---- Runtime Export ----

	static std::string ModelPathToOmdlName(const std::string& modelPath)
	{
		std::filesystem::path p(modelPath);
		return p.stem().string() + ".omdl";
	}

	static bool CopyFileIfNeeded(const std::string& src, const std::string& dst)
	{
		if (std::filesystem::exists(dst))
			return true;
		std::filesystem::create_directories(std::filesystem::path(dst).parent_path());
		std::error_code ec;
		std::filesystem::copy_file(src, dst, std::filesystem::copy_options::skip_existing, ec);
		return !ec;
	}

	EditorWorldSystem::ExportResult EditorWorldSystem::ExportForRuntime(
		const std::string& outputDir, uint32_t mapId)
	{

		ExportResult result;

		// Save all dirty chunks first
		SaveDirtyChunks();

		// Also gather objects for all loaded chunks
		for (auto& [key, chunk] : m_Chunks)
		{
			GatherObjectsForChunk(chunk.get());
		}

		auto& assets = Onyx::Application::GetInstance().GetAssetManager();

		// Create output directories
		char mapDirBuf[64];
		snprintf(mapDirBuf, sizeof(mapDirBuf), "%s/maps/%03u/chunks", outputDir.c_str(), mapId);
		std::string runtimeChunksDir = mapDirBuf;
		std::string modelsDir = outputDir + "/models";
		std::string materialsDir = outputDir + "/materials";

		std::filesystem::create_directories(runtimeChunksDir);
		std::filesystem::create_directories(modelsDir);
		std::filesystem::create_directories(materialsDir);

		// Collect unique model paths across all chunks
		std::unordered_set<std::string> uniqueModels;
		for (const auto& [key, chunk] : m_Chunks)
		{
			for (const auto& obj : chunk->GetObjects())
			{
				if (!obj.modelPath.empty())
				{
					uniqueModels.insert(obj.modelPath);
				}
			}
		}

		// Also collect from editor world if available
		if (m_EditorWorld)
		{
			for (const auto& obj : m_EditorWorld->GetStaticObjects())
			{
				if (!obj->GetModelPath().empty())
				{
					uniqueModels.insert(obj->GetModelPath());
				}
			}
		}

		// Check for duplicate omdl names (different source paths → same stem)
		std::unordered_map<std::string, std::string> omdlNameToSource; // omdlName → first source path
		for (const std::string& modelPath : uniqueModels)
		{
			std::string omdlName = ModelPathToOmdlName(modelPath);
			auto it = omdlNameToSource.find(omdlName);
			if (it != omdlNameToSource.end() && it->second != modelPath)
			{
				result.errors.push_back("Duplicate model name '" + omdlName +
										"' from:\n  " + it->second + "\n  " + modelPath);
			}
			else
			{
				omdlNameToSource[omdlName] = modelPath;
			}
		}
		if (!result.errors.empty())
		{
			return result;
		}

		// Export models as .omdl
		std::unordered_map<std::string, std::string> modelPathRemap; // original → relative .omdl path

		for (const std::string& modelPath : uniqueModels)
		{
			std::string omdlName = ModelPathToOmdlName(modelPath);
			std::string omdlRelative = "models/" + omdlName;
			std::string omdlFullPath = modelsDir + "/" + omdlName;

			// The editor's render path loads models async, which uploads merged
			// GPU buffers and leaves per-mesh CPU `m_Vertices`/`m_Indices` /
			// `m_Textures` empty. The exporter needs CPU-side data, so always
			// re-parse via the synchronous `Model(path, true)` constructor for
			// export. `loadTextures=true` populates each mesh's m_Textures so the
			// material-copy pass below has paths to copy. This is a one-shot
			// local Model that bypasses the AssetManager cache; the editor keeps
			// using its GPU-only async copy for rendering.
			Onyx::Model exportModel(modelPath.c_str(), true);
			Onyx::Model* model = &exportModel;

			// Build .omdl data from model meshes
			MMO::OmdlData omdl;
			auto& meshes = model->GetMeshes();
			if (meshes.empty())
			{
				result.errors.push_back("Failed to parse model (no meshes): " + modelPath);
				continue;
			}

			uint32_t totalVertices = 0;
			uint32_t totalIndices = 0;

			// First pass: count totals
			for (const auto& mesh : meshes)
			{
				totalVertices += static_cast<uint32_t>(mesh.m_Vertices.size());
				totalIndices += static_cast<uint32_t>(mesh.m_Indices.size());
			}

			omdl.header.meshCount = static_cast<uint32_t>(meshes.size());
			omdl.header.totalVertices = totalVertices;
			omdl.header.totalIndices = totalIndices;
			// Compact small models with u16 indices — saves half the index blob.
			const bool useU16Idx = totalVertices < 65536u;
			omdl.header.flags = useU16Idx ? MMO::OMDL_FLAG_U16_INDICES : 0u;

			// Global bounds
			glm::vec3 globalMin(std::numeric_limits<float>::max());
			glm::vec3 globalMax(std::numeric_limits<float>::lowest());

			// Allocate blobs
			omdl.vertexBlob.resize(totalVertices * sizeof(Onyx::MeshVertex));
			omdl.indexBlob.resize(totalIndices * (useU16Idx ? 2 : 4));

			uint32_t vertexOffset = 0;
			uint32_t indexOffset = 0;

			for (size_t i = 0; i < meshes.size(); i++)
			{
				const auto& mesh = meshes[i];
				MMO::OmdlMeshInfo info;
				info.indexCount = static_cast<uint32_t>(mesh.m_Indices.size());
				info.firstIndex = indexOffset;
				info.baseVertex = static_cast<int32_t>(vertexOffset);

				// Mesh bounds
				info.boundsMin[0] = mesh.GetBoundsMin().x;
				info.boundsMin[1] = mesh.GetBoundsMin().y;
				info.boundsMin[2] = mesh.GetBoundsMin().z;
				info.boundsMax[0] = mesh.GetBoundsMax().x;
				info.boundsMax[1] = mesh.GetBoundsMax().y;
				info.boundsMax[2] = mesh.GetBoundsMax().z;

				globalMin = glm::min(globalMin, mesh.GetBoundsMin());
				globalMax = glm::max(globalMax, mesh.GetBoundsMax());

				// Texture paths — copy into materials/{modelStem}/.
				//
				// Assimp's `tex.path` is whatever the source asset embeds (often
				// a name from the original DCC tool that doesn't match what's
				// actually on disk — e.g. an FBX exported from Unreal references
				// `_Unreal_BaseColor.tga` while the artist ships `Albedo.png`).
				// Resolve in three steps:
				//   1. <modelDir>/<assimpPath>
				//   2. <modelDir>/<basename(assimpPath)>
				//   3. heuristic: any image in <modelDir> matching the slot kind.
				std::string modelDir = std::filesystem::path(modelPath).parent_path().string();
				std::string modelStem = std::filesystem::path(modelPath).stem().string();
				std::string matDir = materialsDir + "/" + modelStem;
				std::string matRelative = "materials/" + modelStem + "/";

				auto resolveTextureSrc = [&](const std::string& assimpPath,
											 const std::vector<std::string>& keywords) -> std::string {
					if (!assimpPath.empty())
					{
						std::string p1 = modelDir + "/" + assimpPath;
						if (std::filesystem::exists(p1))
							return p1;
						std::string p2 = modelDir + "/" + std::filesystem::path(assimpPath).filename().string();
						if (std::filesystem::exists(p2))
							return p2;
					}
					if (modelDir.empty() || !std::filesystem::is_directory(modelDir))
						return "";
					for (const auto& entry : std::filesystem::directory_iterator(modelDir))
					{
						if (!entry.is_regular_file())
							continue;
						std::string ext = entry.path().extension().string();
						std::transform(ext.begin(), ext.end(), ext.begin(),
									   [](unsigned char c) { return std::tolower(c); });
						if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".tga" && ext != ".bmp")
							continue;
						std::string name = entry.path().filename().string();
						std::string lower = name;
						std::transform(lower.begin(), lower.end(), lower.begin(),
									   [](unsigned char c) { return std::tolower(c); });
						for (const auto& kw : keywords)
						{
							if (lower.find(kw) != std::string::npos)
							{
								return entry.path().string();
							}
						}
					}
					return "";
				};

				auto copyTexture = [&](const std::string& srcPath) -> std::string {
					if (srcPath.empty())
						return "";
					std::string texName = std::filesystem::path(srcPath).filename().string();
					std::string destPath = matDir + "/" + texName;
					if (CopyFileIfNeeded(srcPath, destPath))
					{
						result.texturesCopied++;
					}
					return matRelative + texName;
				};

				const std::vector<std::string> diffuseKeywords =
					{"albedo", "basecolor", "base_color", "diffuse", "color"};
				const std::vector<std::string> normalKeywords =
					{"normal", "nrm"};

				for (const auto& tex : mesh.m_Textures)
				{
					if (tex.type == "texture_diffuse" && info.albedoPath.empty())
					{
						std::string src = resolveTextureSrc(tex.path, diffuseKeywords);
						info.albedoPath = copyTexture(src);
					}
					if (tex.type == "texture_normal" && info.normalPath.empty())
					{
						std::string src = resolveTextureSrc(tex.path, normalKeywords);
						info.normalPath = copyTexture(src);
					}
				}
				// If the asset exposed no texture slots at all, still try to find
				// sibling images by name — common when the FBX has a stub material.
				if (info.albedoPath.empty())
				{
					info.albedoPath = copyTexture(resolveTextureSrc("", diffuseKeywords));
				}
				if (info.normalPath.empty())
				{
					info.normalPath = copyTexture(resolveTextureSrc("", normalKeywords));
				}

				omdl.meshes.push_back(std::move(info));

				// Copy vertex data (already in v2 28-byte layout)
				size_t vertBytes = mesh.m_Vertices.size() * sizeof(Onyx::MeshVertex);
				memcpy(omdl.vertexBlob.data() + vertexOffset * sizeof(Onyx::MeshVertex),
					   mesh.m_Vertices.data(), vertBytes);
				vertexOffset += static_cast<uint32_t>(mesh.m_Vertices.size());

				// Copy index data — narrow to u16 when totalVertices < 65k
				if (useU16Idx)
				{
					uint16_t* dst = reinterpret_cast<uint16_t*>(omdl.indexBlob.data()) + indexOffset;
					for (size_t k = 0; k < mesh.m_Indices.size(); ++k)
					{
						dst[k] = static_cast<uint16_t>(mesh.m_Indices[k]);
					}
				}
				else
				{
					size_t idxBytes = mesh.m_Indices.size() * sizeof(uint32_t);
					memcpy(omdl.indexBlob.data() + indexOffset * sizeof(uint32_t),
						   mesh.m_Indices.data(), idxBytes);
				}
				indexOffset += static_cast<uint32_t>(mesh.m_Indices.size());
			}

			omdl.header.boundsMin[0] = globalMin.x;
			omdl.header.boundsMin[1] = globalMin.y;
			omdl.header.boundsMin[2] = globalMin.z;
			omdl.header.boundsMax[0] = globalMax.x;
			omdl.header.boundsMax[1] = globalMax.y;
			omdl.header.boundsMax[2] = globalMax.z;

			if (MMO::WriteOmdl(omdlFullPath, omdl))
			{
				modelPathRemap[modelPath] = omdlRelative;
				result.modelsExported++;
			}
			else
			{
				result.errors.push_back("Failed to write .omdl: " + omdlFullPath);
			}
		}

		// Export editor-assigned materials
		std::unordered_set<std::string> exportedMaterialIds;
		for (const auto& [key, chunk] : m_Chunks)
		{
			for (const auto& obj : chunk->GetObjects())
			{
				if (!obj.materialId.empty())
					exportedMaterialIds.insert(obj.materialId);
				for (const auto& mm : obj.meshMaterials)
				{
					if (!mm.materialId.empty())
						exportedMaterialIds.insert(mm.materialId);
				}
			}
		}

		for (const std::string& matId : exportedMaterialIds)
		{
			const Onyx::Material* mat = assets.GetMaterial(matId);
			if (!mat)
				continue;

			std::string matDir = materialsDir + "/" + matId;
			std::string matRelative = "materials/" + matId + "/";

			auto copyMatTexture = [&](const std::string& srcPath) {
				if (srcPath.empty())
					return;
				std::string texName = std::filesystem::path(srcPath).filename().string();
				std::string destPath = matDir + "/" + texName;
				if (CopyFileIfNeeded(srcPath, destPath))
				{
					result.texturesCopied++;
				}
			};

			copyMatTexture(mat->albedoPath);
			copyMatTexture(mat->normalPath);
			copyMatTexture(mat->rmaPath);
			result.materialsExported++;
		}

		// Export chunks as runtime .chunk files
		// We need to iterate ALL known chunk files, not just currently loaded chunks
		for (int32_t chunkKey : m_KnownChunkFiles)
		{
			// Decode chunk key
			int32_t cx = static_cast<int16_t>(chunkKey >> 16);
			int32_t cz = static_cast<int16_t>(chunkKey & 0xFFFF);

			// Load chunk if not already loaded
			auto it = m_Chunks.find(chunkKey);
			WorldChunk* chunk = nullptr;
			bool temporarilyLoaded = false;

			if (it != m_Chunks.end())
			{
				chunk = it->second.get();
			}
			else
			{
				// Temporarily load chunk from disk for export
				auto tempChunk = std::make_unique<WorldChunk>(cx, cz);
				std::string srcPath = GetChunkFilePath(cx, cz);
				tempChunk->Load(srcPath);
				chunk = tempChunk.get();
				temporarilyLoaded = true;
				// Store temporarily for iteration, will clean up after
				m_Chunks[chunkKey] = std::move(tempChunk);
			}

			if (!chunk)
				continue;

			// Build ChunkFileData for runtime
			MMO::ChunkFileData fileData;
			fileData.mapId = mapId;

			// Terrain
			if (chunk->GetTerrain())
			{
				fileData.terrain = chunk->GetTerrain()->GetData();
			}

			// Lights — convert EditorLight to ChunkLightData
			for (const auto& el : chunk->GetLights())
			{
				MMO::ChunkLightData ld;
				ld.type = static_cast<uint8_t>(el.type);
				ld.position[0] = el.position.x;
				ld.position[1] = el.position.y;
				ld.position[2] = el.position.z;
				ld.direction[0] = el.direction.x;
				ld.direction[1] = el.direction.y;
				ld.direction[2] = el.direction.z;
				ld.color[0] = el.color.x;
				ld.color[1] = el.color.y;
				ld.color[2] = el.color.z;
				ld.intensity = el.intensity;
				ld.range = el.range;
				ld.innerAngle = el.innerAngle;
				ld.outerAngle = el.outerAngle;
				ld.castShadows = el.castShadows;
				fileData.lights.push_back(ld);
			}

			// Objects — convert ChunkObject to ChunkObjectData with remapped model paths
			for (const auto& co : chunk->GetObjects())
			{
				MMO::ChunkObjectData od;

				// Remap model path to .omdl relative path
				auto remapIt = modelPathRemap.find(co.modelPath);
				if (remapIt != modelPathRemap.end())
				{
					od.modelPath = remapIt->second;
				}
				else
				{
					od.modelPath = co.modelPath; // fallback to original
				}

				od.position[0] = co.position.x;
				od.position[1] = co.position.y;
				od.position[2] = co.position.z;

				// Convert quaternion to euler angles (radians)
				glm::vec3 euler = glm::eulerAngles(co.rotation);
				od.rotation[0] = euler.x;
				od.rotation[1] = euler.y;
				od.rotation[2] = euler.z;

				od.scale[0] = co.scale;
				od.scale[1] = co.scale;
				od.scale[2] = co.scale;

				od.flags = co.castsShadow ? 1u : 0u;
				od.materialId = co.materialId;

				fileData.objects.push_back(od);
			}

			// Write runtime chunk
			char chunkFilename[64];
			snprintf(chunkFilename, sizeof(chunkFilename), "%s/chunk_%d_%d.chunk",
					 runtimeChunksDir.c_str(), cx, cz);

			if (MMO::WriteChunkFile(chunkFilename, fileData, cx, cz))
			{
				result.chunksExported++;
			}
			else
			{
				result.errors.push_back("Failed to write chunk: " + std::string(chunkFilename));
			}

			// Clean up temporarily loaded chunk
			if (temporarilyLoaded)
			{
				m_Chunks.erase(chunkKey);
			}
		}

		// Emit migration.sql for DB-bound entities (creature spawns + player spawns).
		if (m_EditorWorld)
		{
			std::filesystem::path sqlPath = std::filesystem::path(outputDir) / "migration.sql";
			std::filesystem::create_directories(sqlPath.parent_path());

			std::ofstream sqlOut(sqlPath, std::ios::binary | std::ios::trunc);
			if (!sqlOut.is_open())
			{
				result.errors.push_back("Failed to open migration.sql for writing: " + sqlPath.string());
			}
			else
			{
				sqlOut << "-- Generated by MMOEditor3D — do not edit by hand.\n"
					   << "-- Apply via Run Locally (editor) or psql -f migration.sql.\n\n";

				// creature_spawn — scoped to this map
				std::vector<const MMO::SpawnPoint*> spawnPtrs;
				spawnPtrs.reserve(m_EditorWorld->GetSpawnPoints().size());
				for (const auto& sp : m_EditorWorld->GetSpawnPoints())
				{
					spawnPtrs.push_back(sp.get());
				}
				MMO::MigrationSqlWriter::EmitCreatureSpawnsForMap(mapId, spawnPtrs, sqlOut);
				sqlOut << "\n";

				// player_create_info — global; only PlayerSpawns with a chosen combo
				std::vector<const MMO::PlayerSpawn*> playerPtrs;
				playerPtrs.reserve(m_EditorWorld->GetPlayerSpawns().size());
				for (const auto& ps : m_EditorWorld->GetPlayerSpawns())
				{
					if (!ps->GetRaceClassPairs().empty())
					{
						playerPtrs.push_back(ps.get());
					}
				}
				MMO::MigrationSqlWriter::EmitPlayerCreateInfo(playerPtrs, mapId, sqlOut);
			}
		}

		result.success = result.errors.empty();
		return result;
	}

	WorldChunk* EditorWorldSystem::GetOrCreateChunk(int32_t chunkX, int32_t chunkZ)
	{
		int32_t key = MakeChunkKey(chunkX, chunkZ);
		auto it = m_Chunks.find(key);
		if (it != m_Chunks.end())
		{
			return it->second.get();
		}

		auto chunk = std::make_unique<WorldChunk>(chunkX, chunkZ);
		TerrainChunk* terrain = chunk->GetTerrain();
		terrain->SetNormalMode(m_SobelNormals, m_SmoothNormals);
		terrain->SetDiamondGrid(m_DiamondGrid);
		terrain->SetMeshResolution(m_MeshResolution);
		terrain->SetHeightSampler([this](int cx, int cz, int lx, int lz) {
			return GetChunkHeight(cx, cz, lx, lz);
		});
		LoadChunkFromDisk(chunk.get());
		ApplyDefaultMaterials(chunk.get());
		CopyBoundaryFromNeighbors(chunk.get());
		terrain->CreateGPUResources();
		chunk->ClearModified();

		m_KnownChunkFiles.insert(key);

		WorldChunk* ptr = chunk.get();
		m_Chunks[key] = std::move(chunk);
		DirtyNeighborChunks(chunkX, chunkZ);
		return ptr;
	}

	bool EditorWorldSystem::EnsureChunksReady(int minCX, int maxCX, int minCZ, int maxCZ)
	{
		for (int cz = minCZ; cz <= maxCZ; cz++)
		{
			for (int cx = minCX; cx <= maxCX; cx++)
			{
				WorldChunk* chunk = GetOrCreateChunk(cx, cz);
				if (!chunk || chunk->GetTerrain()->GetState() != ChunkState::Active)
				{
					return false;
				}
			}
		}
		return true;
	}

	void EditorWorldSystem::StitchEdges(int minCX, int maxCX, int minCZ, int maxCZ)
	{
		for (int cz = minCZ; cz <= maxCZ; cz++)
		{
			for (int cx = minCX; cx <= maxCX; cx++)
			{
				int32_t key = MakeChunkKey(cx, cz);
				auto it = m_Chunks.find(key);
				if (it == m_Chunks.end())
					continue;
				auto& data = it->second->GetTerrain()->GetDataMutable();
				if (data.heightmap.empty())
					continue;

				if (cx + 1 <= maxCX)
				{
					auto rightIt = m_Chunks.find(MakeChunkKey(cx + 1, cz));
					if (rightIt != m_Chunks.end() && !rightIt->second->GetTerrain()->GetData().heightmap.empty())
					{
						auto& rdata = rightIt->second->GetTerrain()->GetDataMutable();
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
					if (bottomIt != m_Chunks.end() && !bottomIt->second->GetTerrain()->GetData().heightmap.empty())
					{
						auto& bdata = bottomIt->second->GetTerrain()->GetDataMutable();
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

	void EditorWorldSystem::CopyBoundaryFromNeighbors(WorldChunk* chunk)
	{
		TerrainChunk* terrain = chunk->GetTerrain();
		auto& data = terrain->GetDataMutable();
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
			const auto& nd = leftIt->second->GetTerrain()->GetData();
			if (!nd.heightmap.empty())
			{
				for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
					data.heightmap[lz * CHUNK_RESOLUTION + 0] = nd.heightmap[lz * CHUNK_RESOLUTION + last];
			}
		}

		auto rightIt = m_Chunks.find(MakeChunkKey(cx + 1, cz));
		if (rightIt != m_Chunks.end())
		{
			const auto& nd = rightIt->second->GetTerrain()->GetData();
			if (!nd.heightmap.empty())
			{
				for (int lz = 0; lz < CHUNK_RESOLUTION; lz++)
					data.heightmap[lz * CHUNK_RESOLUTION + last] = nd.heightmap[lz * CHUNK_RESOLUTION + 0];
			}
		}

		auto topIt = m_Chunks.find(MakeChunkKey(cx, cz - 1));
		if (topIt != m_Chunks.end())
		{
			const auto& nd = topIt->second->GetTerrain()->GetData();
			if (!nd.heightmap.empty())
			{
				for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
					data.heightmap[0 * CHUNK_RESOLUTION + lx] = nd.heightmap[last * CHUNK_RESOLUTION + lx];
			}
		}

		auto bottomIt = m_Chunks.find(MakeChunkKey(cx, cz + 1));
		if (bottomIt != m_Chunks.end())
		{
			const auto& nd = bottomIt->second->GetTerrain()->GetData();
			if (!nd.heightmap.empty())
			{
				for (int lx = 0; lx < CHUNK_RESOLUTION; lx++)
					data.heightmap[last * CHUNK_RESOLUTION + lx] = nd.heightmap[0 * CHUNK_RESOLUTION + lx];
			}
		}

		data.CalculateBounds();
	}

	// ---- Object ↔ Chunk Bridge ----

	static ChunkObject StaticObjectToChunkObject(const MMO::StaticObject* obj)
	{
		ChunkObject co;
		co.guid = obj->GetGuid();
		co.name = obj->GetName();
		co.position = obj->GetPosition();
		co.rotation = obj->GetRotation();
		co.scale = obj->GetScale();
		co.parentGuid = obj->GetParentGuid();
		co.modelPath = obj->GetModelPath();
		co.materialId = obj->GetMaterialId();

		// Collider
		const auto& col = obj->GetCollider();
		co.colliderType = static_cast<uint8_t>(col.type);
		co.colliderCenter = col.center;
		co.colliderHalfExtents = col.halfExtents;
		co.colliderRadius = col.radius;
		co.colliderHeight = col.height;

		// Rendering flags
		co.castsShadow = obj->CastsShadow();
		co.receivesLightmap = obj->ReceivesLightmap();
		co.lightmapIndex = obj->GetLightmapIndex();
		co.lightmapScaleOffset = obj->GetLightmapScaleOffset();

		// Mesh materials
		for (const auto& [meshName, mat] : obj->GetAllMeshMaterials())
		{
			ChunkObject::MeshMaterialEntry entry;
			entry.meshName = meshName;
			entry.materialId = mat.materialId;
			entry.positionOffset = mat.positionOffset;
			entry.rotationOffset = mat.rotationOffset;
			entry.scaleMultiplier = mat.scaleMultiplier;
			entry.visible = mat.visible;
			co.meshMaterials.push_back(std::move(entry));
		}

		// Animations
		co.animationPaths = obj->GetAnimationPaths();
		co.currentAnimation = obj->GetCurrentAnimation();
		co.animLoop = obj->GetAnimationLoop();
		co.animSpeed = obj->GetAnimationSpeed();

		return co;
	}

	static std::unique_ptr<MMO::StaticObject> ChunkObjectToStaticObject(const ChunkObject& co)
	{
		auto obj = std::make_unique<MMO::StaticObject>(co.guid, co.name);
		obj->SetPosition(co.position);
		obj->SetRotation(co.rotation);
		obj->SetScale(co.scale);
		obj->SetParent(co.parentGuid);
		obj->SetModelPath(co.modelPath);
		obj->SetMaterialId(co.materialId);

		// Collider
		MMO::ColliderData col;
		col.type = static_cast<MMO::ColliderType>(co.colliderType);
		col.center = co.colliderCenter;
		col.halfExtents = co.colliderHalfExtents;
		col.radius = co.colliderRadius;
		col.height = co.colliderHeight;
		obj->SetCollider(col);

		// Rendering flags
		obj->SetCastsShadow(co.castsShadow);
		obj->SetReceivesLightmap(co.receivesLightmap);
		obj->SetLightmapIndex(co.lightmapIndex);
		obj->SetLightmapScaleOffset(co.lightmapScaleOffset);

		// Mesh materials
		for (const auto& entry : co.meshMaterials)
		{
			MMO::MeshMaterial mat;
			mat.materialId = entry.materialId;
			mat.positionOffset = entry.positionOffset;
			mat.rotationOffset = entry.rotationOffset;
			mat.scaleMultiplier = entry.scaleMultiplier;
			mat.visible = entry.visible;
			obj->SetMeshMaterial(entry.meshName, mat);
		}

		// Animations
		for (const auto& path : co.animationPaths)
		{
			obj->AddAnimationPath(path);
		}
		obj->SetCurrentAnimation(co.currentAnimation);
		obj->SetAnimationLoop(co.animLoop);
		obj->SetAnimationSpeed(co.animSpeed);

		return obj;
	}

	void EditorWorldSystem::GatherObjectsForChunk(WorldChunk* chunk)
	{
		if (!m_EditorWorld)
			return;

		int32_t cx = chunk->GetChunkX();
		int32_t cz = chunk->GetChunkZ();
		chunk->GetObjects().clear();

		for (const auto& obj : m_EditorWorld->GetStaticObjects())
		{
			int32_t objCX = WorldToChunkX(obj->GetPosition().x);
			int32_t objCZ = WorldToChunkZ(obj->GetPosition().z);
			if (objCX == cx && objCZ == cz)
			{
				chunk->GetObjects().push_back(StaticObjectToChunkObject(obj.get()));
			}
		}
	}

	void EditorWorldSystem::SpawnObjectsFromChunk(WorldChunk* chunk)
	{
		if (!m_EditorWorld)
			return;

		int32_t key = MakeChunkKey(chunk->GetChunkX(), chunk->GetChunkZ());

		// Pre-request async model loading for all objects in this chunk
		auto& assets = Onyx::Application::GetInstance().GetAssetManager();
		for (const auto& co : chunk->GetObjects())
		{
			if (!co.modelPath.empty())
				assets.RequestModelAsync(co.modelPath, !co.animationPaths.empty());
		}

		for (const auto& co : chunk->GetObjects())
		{
			// Skip if this GUID already exists in the world (e.g., re-loading same chunk)
			if (m_EditorWorld->GetObject(co.guid))
				continue;

			auto obj = ChunkObjectToStaticObject(co);
			m_EditorWorld->EnsureGuidAbove(co.guid);
			m_EditorWorld->AddObject(std::move(obj));
			m_ObjectChunkMap[co.guid] = key;
		}
	}

	void EditorWorldSystem::RemoveChunkObjects(int32_t chunkKey)
	{
		if (!m_EditorWorld)
			return;

		std::vector<uint64_t> toRemove;
		for (auto& [guid, key] : m_ObjectChunkMap)
		{
			if (key == chunkKey)
			{
				toRemove.push_back(guid);
			}
		}

		for (uint64_t guid : toRemove)
		{
			m_EditorWorld->DeleteObject(guid);
			m_ObjectChunkMap.erase(guid);
		}
	}

	void EditorWorldSystem::ApplyBrush(float worldX, float worldZ, float radius,
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
				TerrainChunk* terrain = m_Chunks[key]->GetTerrain();

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

						operation(terrain, lx, lz, dist);
					}
				}

				terrain->GetDataMutable().CalculateBounds();
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

	// ---- Model Pre-loading ----

	std::vector<std::string> EditorWorldSystem::PeekChunkModelPaths(int32_t chunkX, int32_t chunkZ)
	{
		std::vector<std::string> paths;

		std::string filePath = GetChunkFilePath(chunkX, chunkZ);
		std::ifstream file(filePath, std::ios::binary);
		if (!file.is_open())
			return paths;

		// Read CHNK header
		uint32_t magic;
		file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
		if (magic != MMO::CHNK_MAGIC)
			return paths;

		uint32_t version;
		file.read(reinterpret_cast<char*>(&version), sizeof(version));
		if (version < 2)
			return paths; // v1 has no OBJS section

		uint32_t mapId;
		file.read(reinterpret_cast<char*>(&mapId), sizeof(mapId));

		int32_t cx, cz;
		file.read(reinterpret_cast<char*>(&cx), sizeof(cx));
		file.read(reinterpret_cast<char*>(&cz), sizeof(cz));

		uint32_t sectionCount;
		file.read(reinterpret_cast<char*>(&sectionCount), sizeof(sectionCount));

		// Scan sections looking for OBJS
		for (uint32_t s = 0; s < sectionCount && file.good(); s++)
		{
			uint32_t sectionType, sectionSize;
			file.read(reinterpret_cast<char*>(&sectionType), sizeof(sectionType));
			file.read(reinterpret_cast<char*>(&sectionSize), sizeof(sectionSize));
			auto sectionStart = file.tellg();

			if (sectionType == MMO::OBJS_TAG)
			{
				// Read just the model paths from the objects section
				uint32_t count = 0;
				file.read(reinterpret_cast<char*>(&count), sizeof(count));
				if (count > MMO::MAX_OBJECTS_PER_CHUNK)
					break;

				for (uint32_t i = 0; i < count && file.good(); i++)
				{
					// Skip: guid(8) + name string
					file.seekg(sizeof(uint64_t), std::ios::cur);
					uint16_t nameLen = 0;
					file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
					if (nameLen > 0)
						file.seekg(nameLen, std::ios::cur);

					// Skip: position(12) + rotation(16) + scale(4) + parentGuid(8)
					file.seekg(12 + 16 + 4 + 8, std::ios::cur);

					// Read modelPath
					uint16_t pathLen = 0;
					file.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
					if (pathLen > 0 && pathLen < 4096)
					{
						std::string modelPath(pathLen, '\0');
						file.read(modelPath.data(), pathLen);
						if (!modelPath.empty())
						{
							paths.push_back(std::move(modelPath));
						}
					}

					// Skip rest of this object — seek to next section
					// We don't know exact remaining size per object, so break after reading paths
					// Actually we need to skip remaining fields to get to next object
					// Skip: materialId string
					uint16_t matLen = 0;
					file.read(reinterpret_cast<char*>(&matLen), sizeof(matLen));
					if (matLen > 0)
						file.seekg(matLen, std::ios::cur);

					// Skip: collider(1+12+12+4+4) + flags(1) + lightmapIndex(4) + lightmapScaleOffset(16)
					file.seekg(1 + 12 + 12 + 4 + 4 + 1 + 4 + 16, std::ios::cur);

					// Skip: meshMaterials
					uint16_t meshMatCount = 0;
					file.read(reinterpret_cast<char*>(&meshMatCount), sizeof(meshMatCount));
					for (uint16_t m = 0; m < meshMatCount && file.good(); m++)
					{
						// meshName string + materialId string
						uint16_t len1 = 0, len2 = 0;
						file.read(reinterpret_cast<char*>(&len1), sizeof(len1));
						if (len1 > 0)
							file.seekg(len1, std::ios::cur);
						file.read(reinterpret_cast<char*>(&len2), sizeof(len2));
						if (len2 > 0)
							file.seekg(len2, std::ios::cur);
						// positionOffset(12) + rotationOffset(12) + scaleMultiplier(4) + visible(1)
						file.seekg(12 + 12 + 4 + 1, std::ios::cur);
					}

					// Skip: animations
					uint16_t animCount = 0;
					file.read(reinterpret_cast<char*>(&animCount), sizeof(animCount));
					for (uint16_t a = 0; a < animCount && file.good(); a++)
					{
						uint16_t aLen = 0;
						file.read(reinterpret_cast<char*>(&aLen), sizeof(aLen));
						if (aLen > 0)
							file.seekg(aLen, std::ios::cur);
					}
					// currentAnimation string + animLoop(1) + animSpeed(4)
					uint16_t curAnimLen = 0;
					file.read(reinterpret_cast<char*>(&curAnimLen), sizeof(curAnimLen));
					if (curAnimLen > 0)
						file.seekg(curAnimLen, std::ios::cur);
					file.seekg(1 + 4, std::ios::cur);
				}
				break; // Found OBJS, done
			}

			// Skip to next section
			file.seekg(sectionStart + static_cast<std::streamoff>(sectionSize));
		}

		return paths;
	}

	void EditorWorldSystem::PreloadNearbyModels()
	{
		int centerChunkX = WorldToChunkX(m_CameraPosition.x);
		int centerChunkZ = WorldToChunkZ(m_CameraPosition.z);
		int preloadRadius = static_cast<int>(std::ceil(m_Settings.preloadDistance / CHUNK_SIZE));

		auto& assets = Onyx::Application::GetInstance().GetAssetManager();

		for (int dz = -preloadRadius; dz <= preloadRadius; dz++)
		{
			for (int dx = -preloadRadius; dx <= preloadRadius; dx++)
			{
				int32_t chunkX = centerChunkX + dx;
				int32_t chunkZ = centerChunkZ + dz;
				int32_t key = MakeChunkKey(chunkX, chunkZ);

				// Skip if already peeked, already loaded, or no file on disk
				if (m_PreloadedChunkKeys.count(key))
					continue;
				if (m_Chunks.count(key))
					continue;
				if (!m_KnownChunkFiles.count(key))
					continue;

				float dist = CalculateChunkDistance(chunkX, chunkZ);
				if (dist > m_Settings.preloadDistance)
					continue;

				m_PreloadedChunkKeys.insert(key);

				auto modelPaths = PeekChunkModelPaths(chunkX, chunkZ);
				for (const auto& path : modelPaths)
				{
					assets.RequestModelAsync(path);
				}
			}
		}
	}

	// ---- Background Mesh Generation ----

	void EditorWorldSystem::MeshGenThreadFunc()
	{
		while (true)
		{
			std::shared_ptr<PendingMeshJob> job;
			{
				std::unique_lock<std::mutex> lock(m_MeshGenMutex);
				m_MeshGenCV.wait(lock, [this] {
					return m_ShutdownMeshGen.load() || !m_MeshGenQueue.empty();
				});
				if (m_ShutdownMeshGen.load() && m_MeshGenQueue.empty())
					return;
				if (m_MeshGenQueue.empty())
					continue;
				job = m_MeshGenQueue.front();
				m_MeshGenQueue.pop_front();
			}

			// CPU mesh generation — no GL calls, thread-safe with empty HeightSampler
			// (edge normals use clamped heights; corrected when neighbors regenerate)
			TerrainChunk::HeightSampler noSampler;
			job->terrain->PrepareMeshCPU(job->meshData, noSampler);

			// Move to ready queue for main thread GPU upload
			{
				std::lock_guard<std::mutex> lock(m_MeshGenMutex);
				m_MeshReadyQueue.push_back(std::move(job));
			}
		}
	}

	void EditorWorldSystem::QueueMeshGeneration(int32_t chunkKey, TerrainChunk* terrain, bool dirty)
	{
		// Don't queue if already queued (Preparing state)
		if (terrain->GetState() == ChunkState::Preparing)
			return;

		terrain->SetState(ChunkState::Preparing);

		auto job = std::make_shared<PendingMeshJob>();
		job->chunkKey = chunkKey;
		job->terrain = terrain;
		job->dirty = dirty;

		{
			std::lock_guard<std::mutex> lock(m_MeshGenMutex);
			m_MeshGenQueue.push_back(std::move(job));
		}
		m_MeshGenCV.notify_one();
	}

	void EditorWorldSystem::ProcessReadyMeshes()
	{
		std::deque<std::shared_ptr<PendingMeshJob>> ready;
		{
			std::lock_guard<std::mutex> lock(m_MeshGenMutex);
			ready.swap(m_MeshReadyQueue);
		}

		int uploaded = 0;
		for (auto& job : ready)
		{
			if (uploaded >= m_Settings.maxGPUUploadsPerFrame)
			{
				// Re-queue excess jobs for next frame
				std::lock_guard<std::mutex> lock(m_MeshGenMutex);
				m_MeshReadyQueue.push_back(std::move(job));
				continue;
			}

			job->terrain->UploadPreparedMesh(job->meshData);
			uploaded++;
		}
	}

} // namespace Editor3D
