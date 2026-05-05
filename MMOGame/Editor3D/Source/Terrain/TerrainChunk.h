#pragma once

#include <Onyx.h>
#include <Terrain/TerrainData.h>
#include <Terrain/TerrainMeshGenerator.h>
#include <atomic>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Editor3D {

	using namespace Onyx;

	// Backward-compatible aliases for shared constants
	constexpr float CHUNK_SIZE = MMO::TERRAIN_CHUNK_SIZE;
	constexpr int CHUNK_RESOLUTION = MMO::TERRAIN_CHUNK_RESOLUTION;
	constexpr int CHUNK_HEIGHTMAP_SIZE = MMO::TERRAIN_CHUNK_HEIGHTMAP_SIZE;
	constexpr int HOLE_GRID_SIZE = MMO::TERRAIN_HOLE_GRID_SIZE;
	constexpr int MAX_TERRAIN_LAYERS = MMO::TERRAIN_MAX_LAYERS;
	constexpr int SPLATMAP_RESOLUTION = MMO::TERRAIN_SPLATMAP_RESOLUTION;
	constexpr int SPLATMAP_TEXELS = MMO::TERRAIN_SPLATMAP_TEXELS;

	enum class ChunkState
	{
		Unloaded,
		Loaded,
		Preparing,	 // CPU mesh generation in progress (background thread)
		ReadyForGPU, // CPU mesh data ready, waiting for GL upload
		Active
	};

	// Use shared types
	using TerrainChunkData = MMO::TerrainChunkData;
	using PreparedMeshData = MMO::TerrainMeshData;

	class TerrainChunk
	{
	public:
		TerrainChunk(int32_t chunkX, int32_t chunkZ);
		~TerrainChunk();

		int32_t GetChunkX() const { return m_ChunkX; }
		int32_t GetChunkZ() const { return m_ChunkZ; }

		glm::vec3 GetWorldPosition() const
		{
			return glm::vec3(m_ChunkX * CHUNK_SIZE, 0.0f, m_ChunkZ * CHUNK_SIZE);
		}

		glm::vec3 GetBoundsMin() const
		{
			return glm::vec3(m_ChunkX * CHUNK_SIZE, m_Data.minHeight - 1.0f, m_ChunkZ * CHUNK_SIZE);
		}
		glm::vec3 GetBoundsMax() const
		{
			float maxY = m_Data.maxHeight > m_Data.minHeight + 1.0f ? m_Data.maxHeight : m_Data.minHeight + 1.0f;
			return glm::vec3((m_ChunkX + 1) * CHUNK_SIZE, maxY, (m_ChunkZ + 1) * CHUNK_SIZE);
		}

		ChunkState GetState() const { return m_State; }
		bool IsReady() const { return m_State == ChunkState::Active; }

		void Load(const std::string& basePath);
		void LoadFromData(const TerrainChunkData& data);
		void Unload();
		void CreateGPUResources(); // Legacy: CPU + GPU in one call (main thread)
		void DestroyGPUResources();

		using HeightSampler = std::function<float(int, int, int, int)>;
		void SetHeightSampler(HeightSampler sampler) { m_HeightSampler = std::move(sampler); }

		// Split mesh generation: CPU work (thread-safe) + GPU upload (main thread only)
		// Pass null HeightSampler for background thread (uses clamped edge heights).
		void PrepareMeshCPU(PreparedMeshData& out, const HeightSampler& heightSampler) const;
		void UploadPreparedMesh(PreparedMeshData& data);

		void SetState(ChunkState state) { m_State = state; }

		const TerrainChunkData& GetData() const { return m_Data; }
		TerrainChunkData& GetDataMutable()
		{
			m_Dirty = true;
			m_Modified = true;
			return m_Data;
		}
		TerrainChunkData& GetSplatmapMutable()
		{
			m_SplatmapDirty = true;
			m_Modified = true;
			return m_Data;
		}

		float GetHeightAt(float localX, float localZ) const;

		const std::string& GetLayerMaterial(int layer) const;
		void SetLayerMaterial(int layer, const std::string& materialId);
		int FindMaterialLayer(const std::string& materialId) const;
		int FindUnusedLayer() const;
		int FindLeastUsedLayer() const;

		void MarkSplatmapDirty()
		{
			m_SplatmapDirty = true;
			m_Modified = true;
		}
		void MarkMeshDirty() { m_Dirty = true; }

		void Draw(Shader* shader, bool allowRegenerate = true);

		bool IsDirty() const { return m_Dirty; }
		void ClearDirty() { m_Dirty = false; }

		bool IsModified() const { return m_Modified; }
		void ClearModified() { m_Modified = false; }

		void SetNormalMode(bool sobel, bool smooth);
		void SetDiamondGrid(bool enabled);
		void SetMeshResolution(int resolution);
		int GetMeshResolution() const { return m_MeshResolution; }

	private:
		int32_t m_ChunkX, m_ChunkZ;
		ChunkState m_State = ChunkState::Unloaded;
		bool m_Dirty = false;
		bool m_Modified = false;
		bool m_SplatmapDirty = false;
		bool m_SobelNormals = true;
		bool m_SmoothNormals = true;
		bool m_DiamondGrid = true;
		int m_MeshResolution = CHUNK_RESOLUTION;
		HeightSampler m_HeightSampler;

		TerrainChunkData m_Data;

		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<VertexBuffer> m_VBO;
		std::unique_ptr<IndexBuffer> m_EBO;
		std::unique_ptr<Texture> m_HeightmapTexture;
		std::unique_ptr<Texture> m_SplatmapTexture0;
		std::unique_ptr<Texture> m_SplatmapTexture1;

		uint32_t m_IndexCount = 0;

		void GenerateMesh();
		void UpdateSplatmapTexture();
	};

} // namespace Editor3D
