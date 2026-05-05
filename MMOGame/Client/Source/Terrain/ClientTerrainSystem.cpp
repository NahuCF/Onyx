#include "ClientTerrainSystem.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace MMO {

	void ClientTerrainSystem::LoadZone(uint32_t mapId, const std::string& basePath)
	{
		UnloadZone();
		m_MapId = mapId;

		std::string chunksDir = basePath + "/chunks";

		if (!std::filesystem::exists(chunksDir))
		{
			std::cout << "[ClientTerrain] No chunks directory: " << chunksDir << std::endl;
			return;
		}

		int loadedCount = 0;
		for (const auto& entry : std::filesystem::directory_iterator(chunksDir))
		{
			if (entry.path().extension() != ".chunk")
				continue;

			ChunkFileData fileData;
			if (!LoadChunkFile(entry.path().string(), fileData))
			{
				std::cout << "[ClientTerrain] Failed to load: " << entry.path() << std::endl;
				continue;
			}

			auto chunk = std::make_unique<ClientTerrainChunk>();
			chunk->data = std::move(fileData.terrain);
			chunk->data.CalculateBounds();
			chunk->objects = std::move(fileData.objects);

			CreateChunkGPU(*chunk);

			int64_t key = PackKey(chunk->data.chunkX, chunk->data.chunkZ);
			m_Chunks[key] = std::move(chunk);
			loadedCount++;
		}

		// Build flat object list from all chunks
		m_AllObjects.clear();
		for (const auto& [key, chunk] : m_Chunks)
		{
			for (const auto& obj : chunk->objects)
			{
				m_AllObjects.push_back(obj);
			}
		}

		std::cout << "[ClientTerrain] Loaded " << loadedCount << " chunks, "
				  << m_AllObjects.size() << " objects for map " << mapId << std::endl;
	}

	void ClientTerrainSystem::UnloadZone()
	{
		m_Chunks.clear();
		m_AllObjects.clear();
		m_MapId = 0;
	}

	void ClientTerrainSystem::CreateChunkGPU(ClientTerrainChunk& chunk)
	{
		GenerateMesh(chunk);
		UploadSplatmapTextures(chunk);
	}

	void ClientTerrainSystem::GenerateMesh(ClientTerrainChunk& chunk)
	{
		if (chunk.data.heightmap.empty())
			return;

		// Use shared generator — defaults: 65 res, sobel normals, no diamond grid
		TerrainMeshOptions opts;
		TerrainMeshData meshData;
		GenerateTerrainMesh(chunk.data, opts, meshData);

		chunk.indexCount = meshData.indexCount;

		Onyx::RenderCommand::ResetState();

		chunk.vao = std::make_unique<Onyx::VertexArray>();
		chunk.vbo = std::make_unique<Onyx::VertexBuffer>(meshData.vertices.data(), meshData.vertices.size() * sizeof(float));
		chunk.ebo = std::make_unique<Onyx::IndexBuffer>(meshData.indices.data(), meshData.indices.size() * sizeof(uint32_t));

		Onyx::VertexLayout layout({
			{Onyx::VertexAttributeType::Float3}, // position
			{Onyx::VertexAttributeType::Float3}, // normal
			{Onyx::VertexAttributeType::Float2}	 // texcoord
		});

		chunk.vao->SetVertexBuffer(chunk.vbo.get());
		chunk.vao->SetLayout(layout);
		chunk.vao->SetIndexBuffer(chunk.ebo.get());
		chunk.vao->UnBind();

		// Model matrix is identity (vertices are already in world space)
		chunk.modelMatrix = glm::mat4(1.0f);
	}

	void ClientTerrainSystem::UploadSplatmapTextures(ClientTerrainChunk& chunk)
	{
		if (chunk.data.splatmap.empty())
			return;

		std::vector<uint8_t> rgba0, rgba1;
		SplitSplatmapToRGBA(chunk.data.splatmap, rgba0, rgba1);

		chunk.splatmapTexture0 = Onyx::Texture::CreateFromData(
			rgba0.data(), TERRAIN_SPLATMAP_RESOLUTION, TERRAIN_SPLATMAP_RESOLUTION, 4);
		chunk.splatmapTexture1 = Onyx::Texture::CreateFromData(
			rgba1.data(), TERRAIN_SPLATMAP_RESOLUTION, TERRAIN_SPLATMAP_RESOLUTION, 4);
	}

	void ClientTerrainSystem::Render(Onyx::Shader* shader)
	{
		for (auto& [key, chunk] : m_Chunks)
		{
			if (!chunk->vao || chunk->indexCount == 0)
				continue;

			shader->SetMat4("u_Model", chunk->modelMatrix);

			if (chunk->splatmapTexture0)
			{
				chunk->splatmapTexture0->Bind(0);
				shader->SetInt("u_Splatmap0", 0);
			}
			if (chunk->splatmapTexture1)
			{
				chunk->splatmapTexture1->Bind(1);
				shader->SetInt("u_Splatmap1", 1);
			}

			chunk->vao->Bind();
			Onyx::RenderCommand::DrawIndexed(*chunk->vao, chunk->indexCount);
			chunk->vao->UnBind();
		}
	}

	float ClientTerrainSystem::GetHeightAt(float worldX, float worldZ) const
	{
		int cx = WorldToChunkCoord(worldX);
		int cz = WorldToChunkCoord(worldZ);

		int64_t key = PackKey(cx, cz);
		auto it = m_Chunks.find(key);
		if (it == m_Chunks.end())
			return 0.0f;

		float localX = worldX - cx * TERRAIN_CHUNK_SIZE;
		float localZ = worldZ - cz * TERRAIN_CHUNK_SIZE;

		return GetTerrainHeight(it->second->data, localX, localZ);
	}

} // namespace MMO
