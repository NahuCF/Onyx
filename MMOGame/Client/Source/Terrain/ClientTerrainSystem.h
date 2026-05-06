#pragma once

#include <Onyx.h>
#include <Terrain/ChunkFileReader.h>
#include <Terrain/TerrainData.h>
#include <Terrain/TerrainMeshGenerator.h>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace MMO {

	struct ClientTerrainChunk
	{
		TerrainChunkData data;
		std::vector<ChunkObjectData> objects;

		std::unique_ptr<Onyx::VertexArray> vao;
		std::unique_ptr<Onyx::VertexBuffer> vbo;
		std::unique_ptr<Onyx::IndexBuffer> ebo;
		std::unique_ptr<Onyx::Texture> splatmapTexture0;
		std::unique_ptr<Onyx::Texture> splatmapTexture1;
		std::unique_ptr<Onyx::Texture> heightmapTexture; // padded float heightmap for pixel-normal sampling

		uint32_t indexCount = 0;
		int paddedHeightmapResolution = 0;
		glm::mat4 modelMatrix = glm::mat4(1.0f);
	};

	class ClientTerrainSystem
	{
	public:
		using PerChunkSetup = std::function<void(const ClientTerrainChunk&, Onyx::Shader*)>;

		ClientTerrainSystem() = default;
		~ClientTerrainSystem() = default;

		void LoadZone(uint32_t mapId, const std::string& basePath);
		void UnloadZone();

		// Per-chunk callback runs after the heightmap texture is bound but before
		// the draw call, mirroring EditorWorldSystem::RenderTerrain so the client
		// can push per-layer uniforms (array index, tiling, normal strength) the
		// same way the editor does.
		void Render(Onyx::Shader* shader, const PerChunkSetup& perChunkSetup = nullptr);

		float GetHeightAt(float worldX, float worldZ) const;
		bool HasChunks() const { return !m_Chunks.empty(); }

		const std::vector<ChunkObjectData>& GetAllObjects() const { return m_AllObjects; }

	private:
		void CreateChunkGPU(ClientTerrainChunk& chunk);
		void GenerateMesh(ClientTerrainChunk& chunk);
		void UploadSplatmapTextures(ClientTerrainChunk& chunk);
		void UploadHeightmapTexture(ClientTerrainChunk& chunk, const TerrainMeshData& meshData);

		// Key: packed (chunkX, chunkZ)
		static int64_t PackKey(int32_t cx, int32_t cz)
		{
			return (static_cast<int64_t>(cx) << 32) | (static_cast<uint32_t>(cz));
		}

		std::unordered_map<int64_t, std::unique_ptr<ClientTerrainChunk>> m_Chunks;
		std::vector<ChunkObjectData> m_AllObjects;
		uint32_t m_MapId = 0;
	};

} // namespace MMO
