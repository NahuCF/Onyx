#include "GameRenderer.h"
#include <GL/glew.h>
#include <Graphics/PostProcess/SSAOEffect.h>
#include <Model/OmdlReader.h>
#include <filesystem>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

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

		m_BlitShader = std::make_unique<Onyx::Shader>(
			"MMOGame/Client/assets/shaders/blit.vert",
			"MMOGame/Client/assets/shaders/blit.frag");

		// Engine SceneRenderer drives static-mesh draws, batching, frustum
		// culling, and shadows. Shaders come from the engine asset dir.
		m_SceneRenderer = std::make_unique<Onyx::SceneRenderer>();
		m_SceneRenderer->Init(&Onyx::Application::GetInstance().GetAssetManager(),
							  "Onyx/Assets/Shaders/");
		m_SceneRenderer->SetShadowsEnabled(true);

		// Create 1x1 white texture as diffuse fallback
		m_WhiteTexture = Onyx::Texture::CreateSolidColor(255, 255, 255, 255);

		// Init meshes / quads
		InitCubeMesh();
		InitBlitQuad();

		// Scene framebuffer + post-process stack mirror the editor's setup so
		// the client gets the same MSAA + SSAO. Both lazy-resize to viewport.
		m_SceneFramebuffer = std::make_unique<Onyx::Framebuffer>();
		m_PostProcessStack.Init();
		m_PostProcessStack.AddEffect<Onyx::SSAOEffect>();
		m_PostProcessReady = true;

		m_Initialized = true;
		std::cout << "[GameRenderer] Initialized (SceneRenderer + MSAA " << m_MSAASamples
				  << "x + post-process)" << '\n';
	}

	void GameRenderer::BeginFrame(const glm::vec3& playerPos, float dt,
								  uint32_t viewportWidth, uint32_t viewportHeight)
	{
		if (!m_Initialized)
			return;

		m_ViewportWidth = viewportWidth;
		m_ViewportHeight = viewportHeight;

		EnsureFramebuffer(viewportWidth, viewportHeight);

		// Update camera
		m_Camera.SetTarget(playerPos);
		m_Camera.Update(dt);

		float aspect = static_cast<float>(m_ViewportWidth) / std::max(1u, m_ViewportHeight);
		m_ViewMatrix = m_Camera.GetViewMatrix();
		m_ProjMatrix = m_Camera.GetProjectionMatrix(aspect);

		// Push lights + matrices into the SceneRenderer for this frame.
		Onyx::DirectionalLight sun;
		sun.direction = m_SunDirection;
		sun.color = m_SunColor;
		sun.enabled = true;
		m_SceneRenderer->SetDirectionalLight(sun);
		m_SceneRenderer->SetAmbientStrength(m_AmbientStrength);
		m_SceneRenderer->Begin(m_ViewMatrix, m_ProjMatrix, m_Camera.GetPosition());

		// Submit static objects + run shadow pass UP FRONT so the cascade depth
		// textures are populated before the terrain samples them. The terrain
		// uses its own shader (still its own pass) and binds the shadow map at
		// slot 6 — shadows must be written before RenderTerrain runs.
		for (const auto& obj : m_StaticObjects)
		{
			if (!obj.model)
				continue;
			const size_t meshCount = obj.model->GetMeshCount();
			for (size_t i = 0; i < meshCount; i++)
			{
				m_SceneRenderer->SubmitStatic(obj.model,
											  static_cast<uint32_t>(i),
											  obj.modelMatrix,
											  /*albedoPath=*/"",
											  /*normalPath=*/"");
			}
		}
		m_SceneRenderer->RenderShadows();

		// Render scene into the MSAA framebuffer; post-process + blit happen in EndFrame.
		m_SceneFramebuffer->Bind();
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

		// Same texture-slot layout as Editor3D's RenderTerrain so the terrain
		// shader is shared verbatim: 0=heightmap, 1/2=splatmaps, 3/4/5=texture
		// arrays, 6=cascaded shadow map.
		m_SceneRenderer->UploadLightUniforms(m_TerrainShader.get());
		m_TerrainShader->SetInt("u_Heightmap", 0);
		m_TerrainShader->SetInt("u_Splatmap0", 1);
		m_TerrainShader->SetInt("u_Splatmap1", 2);
		m_TerrainShader->SetInt("u_DiffuseArray", 3);
		m_TerrainShader->SetInt("u_NormalArray", 4);
		m_TerrainShader->SetInt("u_RMAArray", 5);
		m_TerrainShader->SetInt("u_ShadowMap", 6);

		// CHUNK_RESOLUTION + 2 padding == 67. Texel size is the inverse so the
		// shader's offset math works for sobel filtering.
		const float paddedRes = static_cast<float>(TERRAIN_CHUNK_RESOLUTION + 2);
		m_TerrainShader->SetFloat("u_HeightmapTexelSize", 1.0f / paddedRes);
		m_TerrainShader->SetFloat("u_ChunkSize", TERRAIN_CHUNK_SIZE);
		m_TerrainShader->SetInt("u_UsePixelNormals", 1);
		m_TerrainShader->SetInt("u_EnablePBR", 0);

		m_SceneRenderer->UploadShadowUniforms(m_TerrainShader.get(), 6);

		if (auto* dArr = m_TerrainMaterials.GetDiffuseArray())
			dArr->Bind(3);
		if (auto* nArr = m_TerrainMaterials.GetNormalArray())
			nArr->Bind(4);
		if (auto* rArr = m_TerrainMaterials.GetRMAArray())
			rArr->Bind(5);

		terrain.Render(m_TerrainShader.get(),
					   [this](const ClientTerrainChunk& chunk, Onyx::Shader* shader) {
						   const float originX = static_cast<float>(chunk.data.chunkX) * TERRAIN_CHUNK_SIZE;
						   const float originZ = static_cast<float>(chunk.data.chunkZ) * TERRAIN_CHUNK_SIZE;
						   shader->SetVec2("u_ChunkOrigin", originX, originZ);

						   int arrayIndices[TERRAIN_MAX_LAYERS];
						   float tilingScales[TERRAIN_MAX_LAYERS];
						   float normalStrengths[TERRAIN_MAX_LAYERS];
						   for (int i = 0; i < TERRAIN_MAX_LAYERS; i++)
						   {
							   const auto& matId = chunk.data.materialIds[i];
							   arrayIndices[i] = m_TerrainMaterials.GetMaterialArrayIndex(matId);
							   tilingScales[i] = m_TerrainMaterials.GetTilingScale(matId);
							   normalStrengths[i] = m_TerrainMaterials.GetNormalStrength(matId);
						   }
						   shader->SetIntArray("u_LayerArrayIndex[0]", arrayIndices, TERRAIN_MAX_LAYERS);
						   shader->SetFloatArray("u_LayerTiling[0]", tilingScales, TERRAIN_MAX_LAYERS);
						   shader->SetFloatArray("u_LayerNormalStrength[0]", normalStrengths, TERRAIN_MAX_LAYERS);
					   });

		m_TerrainShader->UnBind();
	}

	void GameRenderer::LoadTerrainMaterials(const std::string& dataDir)
	{
		// Scan Data/materials/terrain/*.terrainmat → vector<Material> →
		// Onyx::TerrainMaterialLibrary::Build. The JSON shape mirrors the
		// editor's MaterialSerializer::SaveMaterial; we accept both "albedo"
		// and the legacy "diffuse" key for backward compat.
		m_TerrainMaterials.SetAssetRoot(dataDir);

		std::vector<Onyx::Material> materials;
		std::string dir = dataDir + "/materials/terrain";
		if (!std::filesystem::exists(dir))
		{
			std::cout << "[GameRenderer] No terrain materials dir: " << dir << '\n';
			m_TerrainMaterials.Build(materials);
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(dir))
		{
			if (entry.path().extension() != ".terrainmat")
				continue;

			std::ifstream file(entry.path());
			if (!file.is_open())
				continue;

			nlohmann::json j;
			try
			{
				file >> j;
			}
			catch (const nlohmann::json::parse_error& e)
			{
				std::cerr << "[GameRenderer] Parse error in " << entry.path()
						  << ": " << e.what() << '\n';
				continue;
			}

			Onyx::Material mat;
			mat.id = j.value("id", entry.path().stem().string());
			mat.name = j.value("name", "Unnamed");
			mat.albedoPath = j.contains("albedo") ? j.value("albedo", "")
												  : j.value("diffuse", "");
			mat.normalPath = j.value("normal", "");
			mat.rmaPath = j.value("rma", "");
			mat.tilingScale = j.value("tilingScale", 8.0f);
			mat.normalStrength = j.value("normalStrength", 1.0f);
			materials.push_back(std::move(mat));
		}

		m_TerrainMaterials.Build(materials);
		std::cout << "[GameRenderer] Loaded " << materials.size()
				  << " terrain materials from " << dir << '\n';
	}

	void GameRenderer::RenderStaticObjects()
	{
		if (!m_Initialized || m_StaticObjects.empty())
			return;

		// Submission and shadow pass run in BeginFrame (so the terrain pass
		// can sample the cascades). All that's left is the lit batched draws.
		m_SceneRenderer->RenderBatches();
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

		Onyx::RenderCommand::ResetState();
		m_SceneFramebuffer->UnBind();

		// Resolve MSAA into a regular texture so post-process / blit can sample.
		m_SceneFramebuffer->Resolve();

		uint32_t finalTexture = m_SceneFramebuffer->GetColorBufferID();
		if (m_PostProcessReady && m_PostProcessStack.HasEnabledEffects())
		{
			finalTexture = m_PostProcessStack.Execute(
				m_SceneFramebuffer->GetColorBufferID(),
				m_SceneFramebuffer->GetFrameBufferID(),
				m_ViewportWidth, m_ViewportHeight,
				m_ProjMatrix);
		}

		// Composite the final image to the backbuffer so ImGui (drawn next)
		// renders on top. Depth must stay disabled here — we're just sampling
		// a texture into a fullscreen quad.
		BlitTextureToBackbuffer(finalTexture);

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

	void GameRenderer::EnsureFramebuffer(uint32_t viewportWidth, uint32_t viewportHeight)
	{
		if (!m_SceneFramebuffer)
			return;
		if (m_SceneFramebuffer->GetWidth() == viewportWidth &&
			m_SceneFramebuffer->GetHeight() == viewportHeight &&
			m_SceneFramebuffer->GetSamples() == m_MSAASamples)
		{
			return;
		}
		m_SceneFramebuffer->Create(viewportWidth, viewportHeight, m_MSAASamples);
		m_PostProcessStack.Resize(viewportWidth, viewportHeight);
	}

	void GameRenderer::InitBlitQuad()
	{
		// Fullscreen NDC quad with UVs. Two triangles, six indices.
		float quadVerts[] = {
			// pos.xy   uv.xy
			-1.0f, -1.0f, 0.0f, 0.0f,
			 1.0f, -1.0f, 1.0f, 0.0f,
			 1.0f,  1.0f, 1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f, 1.0f,
		};
		uint32_t quadIndices[] = {0, 1, 2, 0, 2, 3};

		Onyx::RenderCommand::ResetState();

		m_BlitVAO = std::make_unique<Onyx::VertexArray>();
		m_BlitVBO = std::make_unique<Onyx::VertexBuffer>(quadVerts, sizeof(quadVerts));
		m_BlitEBO = std::make_unique<Onyx::IndexBuffer>(quadIndices, sizeof(quadIndices));

		Onyx::VertexLayout layout({
			{Onyx::VertexAttributeType::Float2}, // position
			{Onyx::VertexAttributeType::Float2}	 // uv
		});
		m_BlitVAO->SetVertexBuffer(m_BlitVBO.get());
		m_BlitVAO->SetLayout(layout);
		m_BlitVAO->SetIndexBuffer(m_BlitEBO.get());
		m_BlitVAO->UnBind();
	}

	void GameRenderer::BlitTextureToBackbuffer(uint32_t textureId)
	{
		// Default framebuffer (the GLFW backbuffer)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		Onyx::RenderCommand::SetViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
		Onyx::RenderCommand::DisableDepthTest();
		Onyx::RenderCommand::DisableCulling();

		m_BlitShader->Bind();
		m_BlitShader->SetInt("u_Source", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureId);

		m_BlitVAO->Bind();
		Onyx::RenderCommand::DrawIndexed(*m_BlitVAO, 6);
		m_BlitVAO->UnBind();

		m_BlitShader->UnBind();
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
