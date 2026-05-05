#include "GameRenderer.h"
#include <GL/glew.h>
#include <Model/OmdlReader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace MMO {

	GameRenderer::GameRenderer() = default;

	GameRenderer::~GameRenderer() = default;

	void GameRenderer::Init()
	{
		// Load shaders
		m_TerrainShader = std::make_unique<Onyx::Shader>(
			"MMOGame/Client/assets/shaders/terrain.vert",
			"MMOGame/Client/assets/shaders/terrain.frag");

		m_EntityShader = std::make_unique<Onyx::Shader>(
			"MMOGame/Client/assets/shaders/entity.vert",
			"MMOGame/Client/assets/shaders/entity.frag");

		m_ModelShader = std::make_unique<Onyx::Shader>(
			"MMOGame/Client/assets/shaders/model.vert",
			"MMOGame/Client/assets/shaders/model.frag");

		// Create 1x1 white texture as diffuse fallback
		m_WhiteTexture = Onyx::Texture::CreateSolidColor(255, 255, 255, 255);

		// Init cube mesh for entity rendering
		InitCubeMesh();

		m_Initialized = true;
		std::cout << "[GameRenderer] Initialized (direct backbuffer)" << '\n';
	}

	void GameRenderer::BeginFrame(const glm::vec3& playerPos, float dt,
								  uint32_t viewportWidth, uint32_t viewportHeight)
	{
		if (!m_Initialized)
			return;

		m_ViewportWidth = viewportWidth;
		m_ViewportHeight = viewportHeight;

		// Update camera
		m_Camera.SetTarget(playerPos);
		m_Camera.Update(dt);

		float aspect = static_cast<float>(m_ViewportWidth) / std::max(1u, m_ViewportHeight);
		m_ViewMatrix = m_Camera.GetViewMatrix();
		m_ProjMatrix = m_Camera.GetProjectionMatrix(aspect);

		// Render directly to the default framebuffer (backbuffer)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		Onyx::RenderCommand::SetViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
		Onyx::RenderCommand::SetClearColor(0.4f, 0.6f, 0.9f, 1.0f); // Sky blue
		Onyx::RenderCommand::Clear();
		Onyx::RenderCommand::EnableDepthTest();
		Onyx::RenderCommand::EnableCulling();
	}

	void GameRenderer::RenderTerrain(ClientTerrainSystem& terrain)
	{
		if (!m_Initialized || !terrain.HasChunks())
			return;

		m_TerrainShader->Bind();
		m_TerrainShader->SetMat4("u_View", m_ViewMatrix);
		m_TerrainShader->SetMat4("u_Projection", m_ProjMatrix);
		m_TerrainShader->SetVec3("u_LightDir", m_SunDirection);
		m_TerrainShader->SetVec3("u_LightColor", m_SunColor);
		m_TerrainShader->SetFloat("u_AmbientStrength", m_AmbientStrength);

		terrain.Render(m_TerrainShader.get());
		m_TerrainShader->UnBind();
	}

	void GameRenderer::RenderStaticObjects()
	{
		if (!m_Initialized || m_StaticObjects.empty())
			return;

		m_ModelShader->Bind();
		m_ModelShader->SetMat4("u_View", m_ViewMatrix);
		m_ModelShader->SetMat4("u_Projection", m_ProjMatrix);
		m_ModelShader->SetVec3("u_LightDir", m_SunDirection);
		m_ModelShader->SetVec3("u_LightColor", m_SunColor);
		m_ModelShader->SetFloat("u_AmbientStrength", m_AmbientStrength);
		m_ModelShader->SetInt("u_AlbedoMap", 0);

		for (const auto& obj : m_StaticObjects)
		{
			if (!obj.model)
				continue;

			m_ModelShader->SetMat4("u_Model", obj.modelMatrix);
			obj.model->vao->Bind();

			for (size_t i = 0; i < obj.model->meshes.size(); i++)
			{
				const auto& mesh = obj.model->meshes[i];

				// Bind per-mesh albedo texture
				if (i < obj.model->albedoTextures.size() && obj.model->albedoTextures[i])
				{
					obj.model->albedoTextures[i]->Bind(0);
				}
				else
				{
					m_WhiteTexture->Bind(0);
				}

				glDrawElementsBaseVertex(
					GL_TRIANGLES,
					static_cast<GLsizei>(mesh.indexCount),
					obj.model->indexType,
					reinterpret_cast<void*>(static_cast<uintptr_t>(mesh.firstIndex * obj.model->indexByteSize)),
					mesh.baseVertex);
			}

			obj.model->vao->UnBind();
		}

		m_ModelShader->UnBind();
	}

	void GameRenderer::LoadStaticObjects(const ClientTerrainSystem& terrain, const std::string& dataDir)
	{
		m_StaticObjects.clear();

		const auto& objects = terrain.GetAllObjects();
		std::cout << "[GameRenderer] Loading " << objects.size() << " static objects..." << '\n';

		for (const auto& obj : objects)
		{
			if (obj.modelPath.empty())
				continue;

			std::string fullPath = dataDir + "/" + obj.modelPath;
			RuntimeModel* model = LoadRuntimeModel(fullPath);
			if (!model)
				continue;

			// Build model matrix from position, rotation (euler radians), scale
			glm::mat4 mat = glm::mat4(1.0f);
			mat = glm::translate(mat, glm::vec3(obj.position[0], obj.position[1], obj.position[2]));
			mat = glm::rotate(mat, obj.rotation[1], glm::vec3(0, 1, 0)); // Y first
			mat = glm::rotate(mat, obj.rotation[0], glm::vec3(1, 0, 0)); // X
			mat = glm::rotate(mat, obj.rotation[2], glm::vec3(0, 0, 1)); // Z
			mat = glm::scale(mat, glm::vec3(obj.scale[0], obj.scale[1], obj.scale[2]));

			StaticWorldObject swo;
			swo.model = model;
			swo.modelMatrix = mat;
			m_StaticObjects.push_back(swo);
		}

		std::cout << "[GameRenderer] Loaded " << m_StaticObjects.size() << " static objects, "
				  << m_ModelCache.size() << " unique models" << '\n';
	}

	RuntimeModel* GameRenderer::LoadRuntimeModel(const std::string& path)
	{
		// Check cache
		auto it = m_ModelCache.find(path);
		if (it != m_ModelCache.end())
		{
			return it->second.get();
		}

		// Read .omdl file via memory mapping (zero-copy view).
		OmdlMapped data;
		if (!ReadOmdl(path, data))
		{
			std::cerr << "[GameRenderer] Failed to load .omdl: " << path << '\n';
			return nullptr;
		}

		auto model = std::make_unique<RuntimeModel>();
		model->meshes = std::move(data.meshes);
		model->totalIndices = data.header.totalIndices;
		const bool u16 = (data.header.flags & OMDL_FLAG_U16_INDICES) != 0;
		model->indexType = u16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
		model->indexByteSize = u16 ? 2 : 4;

		Onyx::RenderCommand::ResetState();

		// Create GPU buffers directly from the mapping. glBufferData copies
		// straight from the mapped page → VRAM, no std::vector middleman.
		model->vao = std::make_unique<Onyx::VertexArray>();
		model->vbo = std::make_unique<Onyx::VertexBuffer>(
			data.vertexData, static_cast<uint32_t>(data.vertexBytes));
		model->ebo = std::make_unique<Onyx::IndexBuffer>(
			data.indexData, static_cast<uint32_t>(data.indexBytes));

		Onyx::VertexLayout layout = Onyx::MeshVertex::GetLayout();

		model->vao->SetVertexBuffer(model->vbo.get());
		model->vao->SetLayout(layout);
		model->vao->SetIndexBuffer(model->ebo.get());
		model->vao->UnBind();

		// Load per-mesh albedo textures
		// Resolve paths relative to the Data/ directory
		std::string dataDir;
		{
			// Extract base dir: path is like "Data/models/foo.omdl", we want "Data/"
			auto pos = path.find("models/");
			if (pos != std::string::npos)
			{
				dataDir = path.substr(0, pos); // "Data/"
			}
		}

		model->albedoTextures.resize(model->meshes.size());
		for (size_t i = 0; i < model->meshes.size(); i++)
		{
			const auto& meshInfo = model->meshes[i];
			if (!meshInfo.albedoPath.empty())
			{
				std::string texPath = dataDir + meshInfo.albedoPath;
				auto tex = std::make_unique<Onyx::Texture>(texPath.c_str());
				if (tex->GetTextureID() != 0)
				{
					model->albedoTextures[i] = std::move(tex);
				}
			}
		}

		RuntimeModel* ptr = model.get();
		m_ModelCache[path] = std::move(model);

		std::cout << "[GameRenderer] Loaded .omdl: " << path
				  << " (" << data.header.meshCount << " meshes, "
				  << data.header.totalVertices << " verts)" << '\n';

		return ptr;
	}

	void GameRenderer::RenderEntities(const LocalPlayer& player,
									  const std::unordered_map<EntityId, RemoteEntity>& entities,
									  const std::vector<GameClient::ClientPortal>& portals,
									  const std::vector<GameClient::ClientProjectile>& projectiles,
									  const ClientTerrainSystem& terrain)
	{
		if (!m_Initialized)
			return;

		m_EntityShader->Bind();
		m_EntityShader->SetMat4("u_View", m_ViewMatrix);
		m_EntityShader->SetMat4("u_Projection", m_ProjMatrix);
		m_EntityShader->SetVec3("u_LightDir", m_SunDirection);
		m_EntityShader->SetVec3("u_LightColor", m_SunColor);
		m_EntityShader->SetFloat("u_AmbientStrength", m_AmbientStrength);

		// Draw local player
		{
			// Server-authored Z (from PlayerSpawn) takes precedence over terrain snap.
			float h = (player.height != 0.0f) ? player.height
											  : terrain.GetHeightAt(player.position.x, player.position.y);
			glm::vec3 pos3D(player.position.x, h, player.position.y);

			glm::vec4 color;
			switch (player.characterClass)
			{
			case CharacterClass::WARRIOR:
				color = glm::vec4(0.4f, 0.6f, 1.0f, 1.0f);
				break;
			case CharacterClass::WITCH:
				color = glm::vec4(0.8f, 0.4f, 1.0f, 1.0f);
				break;
			default:
				color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				break;
			}
			DrawCube(pos3D, glm::vec3(0.5f, 1.0f, 0.5f), color);
		}

		// Draw remote entities
		for (const auto& [id, entity] : entities)
		{
			float h = (entity.height != 0.0f) ? entity.height
											  : terrain.GetHeightAt(entity.position.x, entity.position.y);
			glm::vec3 pos3D(entity.position.x, h, entity.position.y);

			glm::vec4 color;
			bool isDead = entity.health <= 0;

			switch (entity.type)
			{
			case EntityType::PLAYER:
				color = glm::vec4(0.0f, 0.8f, 0.0f, 1.0f);
				break;
			case EntityType::MOB:
				if (isDead)
					color = glm::vec4(0.3f, 0.25f, 0.25f, 1.0f);
				else
					color = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);
				break;
			default:
				color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
				break;
			}

			float cubeHeight = isDead ? 0.3f : 1.0f;
			DrawCube(pos3D, glm::vec3(0.5f, cubeHeight, 0.5f), color);

			if (id == player.targetId)
			{
				Onyx::RenderCommand::SetWireframeMode(true);
				DrawCube(pos3D, glm::vec3(0.6f, cubeHeight + 0.1f, 0.6f),
						 glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
				Onyx::RenderCommand::SetWireframeMode(false);
			}
		}

		// Draw portals
		for (const auto& portal : portals)
		{
			float h = terrain.GetHeightAt(portal.position.x, portal.position.y);
			glm::vec3 pos3D(portal.position.x, h, portal.position.y);
			DrawCube(pos3D, glm::vec3(portal.size.x * 0.5f, 2.0f, portal.size.y * 0.5f),
					 glm::vec4(0.4f, 0.8f, 1.0f, 0.6f));
		}

		// Draw projectiles as small cubes
		for (const auto& proj : projectiles)
		{
			float h = terrain.GetHeightAt(proj.position.x, proj.position.y);
			glm::vec3 pos3D(proj.position.x, h + 1.0f, proj.position.y);
			DrawCube(pos3D, glm::vec3(0.2f, 0.2f, 0.2f),
					 glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
		}

		m_EntityShader->UnBind();
	}

	void GameRenderer::EndFrame()
	{
		if (!m_Initialized)
			return;

		Onyx::RenderCommand::DisableDepthTest();
	}

	glm::vec2 GameRenderer::ProjectToScreen(const glm::vec3& worldPos,
											float viewportWidth, float viewportHeight) const
	{
		glm::vec4 clip = m_ProjMatrix * m_ViewMatrix * glm::vec4(worldPos, 1.0f);
		if (clip.w <= 0.0f)
			return glm::vec2(-1.0f);

		glm::vec3 ndc = glm::vec3(clip) / clip.w;

		float screenX = (ndc.x * 0.5f + 0.5f) * viewportWidth;
		float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportHeight;

		return glm::vec2(screenX, screenY);
	}

	void GameRenderer::DrawCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec4& color)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position + glm::vec3(0, scale.y, 0));
		model = glm::scale(model, scale);

		m_EntityShader->SetMat4("u_Model", model);
		m_EntityShader->SetVec4("u_Color", color.r, color.g, color.b, color.a);

		m_CubeVAO->Bind();
		Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, m_CubeIndexCount);
		m_CubeVAO->UnBind();
	}

	void GameRenderer::InitCubeMesh()
	{
		float vertices[] = {
			// Front face (normal: 0,0,1)
			-1,
			-1,
			1,
			0,
			0,
			1,
			1,
			-1,
			1,
			0,
			0,
			1,
			1,
			1,
			1,
			0,
			0,
			1,
			-1,
			1,
			1,
			0,
			0,
			1,
			// Back face (normal: 0,0,-1)
			-1,
			-1,
			-1,
			0,
			0,
			-1,
			-1,
			1,
			-1,
			0,
			0,
			-1,
			1,
			1,
			-1,
			0,
			0,
			-1,
			1,
			-1,
			-1,
			0,
			0,
			-1,
			// Top face (normal: 0,1,0)
			-1,
			1,
			-1,
			0,
			1,
			0,
			-1,
			1,
			1,
			0,
			1,
			0,
			1,
			1,
			1,
			0,
			1,
			0,
			1,
			1,
			-1,
			0,
			1,
			0,
			// Bottom face (normal: 0,-1,0)
			-1,
			-1,
			-1,
			0,
			-1,
			0,
			1,
			-1,
			-1,
			0,
			-1,
			0,
			1,
			-1,
			1,
			0,
			-1,
			0,
			-1,
			-1,
			1,
			0,
			-1,
			0,
			// Right face (normal: 1,0,0)
			1,
			-1,
			-1,
			1,
			0,
			0,
			1,
			1,
			-1,
			1,
			0,
			0,
			1,
			1,
			1,
			1,
			0,
			0,
			1,
			-1,
			1,
			1,
			0,
			0,
			// Left face (normal: -1,0,0)
			-1,
			-1,
			-1,
			-1,
			0,
			0,
			-1,
			-1,
			1,
			-1,
			0,
			0,
			-1,
			1,
			1,
			-1,
			0,
			0,
			-1,
			1,
			-1,
			-1,
			0,
			0,
		};

		uint32_t indices[] = {
			0, 1, 2, 0, 2, 3,
			4, 5, 6, 4, 6, 7,
			8, 9, 10, 8, 10, 11,
			12, 13, 14, 12, 14, 15,
			16, 17, 18, 16, 18, 19,
			20, 21, 22, 20, 22, 23};

		m_CubeIndexCount = 36;

		Onyx::RenderCommand::ResetState();

		m_CubeVAO = std::make_unique<Onyx::VertexArray>();
		m_CubeVBO = std::make_unique<Onyx::VertexBuffer>(vertices, sizeof(vertices));
		m_CubeEBO = std::make_unique<Onyx::IndexBuffer>(indices, sizeof(indices));

		Onyx::VertexLayout layout({
			{Onyx::VertexAttributeType::Float3}, // position
			{Onyx::VertexAttributeType::Float3}	 // normal
		});

		m_CubeVAO->SetVertexBuffer(m_CubeVBO.get());
		m_CubeVAO->SetLayout(layout);
		m_CubeVAO->SetIndexBuffer(m_CubeEBO.get());
		m_CubeVAO->UnBind();
	}

} // namespace MMO
