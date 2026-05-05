#pragma once

#include "../GameClient.h"
#include "../Terrain/ClientTerrainSystem.h"
#include "IsometricCamera.h"
#include <Model/OmdlFormat.h>
#include <Onyx.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>

namespace MMO {

	struct RuntimeModel
	{
		std::unique_ptr<Onyx::VertexArray> vao;
		std::unique_ptr<Onyx::VertexBuffer> vbo;
		std::unique_ptr<Onyx::IndexBuffer> ebo;
		std::vector<OmdlMeshInfo> meshes;
		std::vector<std::unique_ptr<Onyx::Texture>> albedoTextures;
		uint32_t totalIndices = 0;
		uint32_t indexType = 0;       // GL_UNSIGNED_INT or GL_UNSIGNED_SHORT
		uint32_t indexByteSize = 4;   // 4 (u32) or 2 (u16)
	};

	struct StaticWorldObject
	{
		RuntimeModel* model = nullptr;
		glm::mat4 modelMatrix = glm::mat4(1.0f);
	};

	class GameRenderer
	{
	public:
		GameRenderer();
		~GameRenderer();

		void Init();

		void BeginFrame(const glm::vec3& playerPos, float dt, uint32_t viewportWidth, uint32_t viewportHeight);
		void RenderTerrain(ClientTerrainSystem& terrain);
		void RenderStaticObjects();
		void RenderEntities(const LocalPlayer& player, const std::unordered_map<EntityId, RemoteEntity>& entities,
							const std::vector<GameClient::ClientPortal>& portals,
							const std::vector<GameClient::ClientProjectile>& projectiles,
							const ClientTerrainSystem& terrain);
		void EndFrame();

		void LoadStaticObjects(const ClientTerrainSystem& terrain, const std::string& dataDir);

		IsometricCamera& GetCamera() { return m_Camera; }
		const IsometricCamera& GetCamera() const { return m_Camera; }

		glm::vec2 ProjectToScreen(const glm::vec3& worldPos, float viewportWidth, float viewportHeight) const;

	private:
		void DrawCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec4& color);
		void InitCubeMesh();
		RuntimeModel* LoadRuntimeModel(const std::string& path);

		IsometricCamera m_Camera;

		std::unique_ptr<Onyx::Shader> m_TerrainShader;
		std::unique_ptr<Onyx::Shader> m_EntityShader;
		std::unique_ptr<Onyx::Shader> m_ModelShader;

		// Cube mesh for entities
		std::unique_ptr<Onyx::VertexArray> m_CubeVAO;
		std::unique_ptr<Onyx::VertexBuffer> m_CubeVBO;
		std::unique_ptr<Onyx::IndexBuffer> m_CubeEBO;
		uint32_t m_CubeIndexCount = 0;

		// Default texture for terrain (white 1x1 as fallback)
		std::unique_ptr<Onyx::Texture> m_WhiteTexture;

		// .omdl model cache and static objects
		std::unordered_map<std::string, std::unique_ptr<RuntimeModel>> m_ModelCache;
		std::vector<StaticWorldObject> m_StaticObjects;

		// Directional light (hardcoded sun)
		glm::vec3 m_SunDirection = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
		glm::vec3 m_SunColor = glm::vec3(1.0f, 0.95f, 0.9f);
		float m_AmbientStrength = 0.35f;

		uint32_t m_ViewportWidth = 1;
		uint32_t m_ViewportHeight = 1;

		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		glm::mat4 m_ProjMatrix = glm::mat4(1.0f);

		bool m_Initialized = false;
	};

} // namespace MMO
