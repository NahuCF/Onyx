#include "GameRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace MMO {

GameRenderer::GameRenderer() {}

GameRenderer::~GameRenderer() {}

void GameRenderer::Init() {
    // Load shaders
    m_TerrainShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Client/assets/shaders/terrain.vert",
        "MMOGame/Client/assets/shaders/terrain.frag");

    m_EntityShader = std::make_unique<Onyx::Shader>(
        "MMOGame/Client/assets/shaders/entity.vert",
        "MMOGame/Client/assets/shaders/entity.frag");

    // Create 1x1 white texture as diffuse fallback
    m_WhiteTexture = Onyx::Texture::CreateSolidColor(255, 255, 255, 255);

    // Init cube mesh for entity rendering
    InitCubeMesh();

    m_Initialized = true;
    std::cout << "[GameRenderer] Initialized (direct backbuffer)" << std::endl;
}

void GameRenderer::BeginFrame(const glm::vec3& playerPos, float dt,
                               uint32_t viewportWidth, uint32_t viewportHeight) {
    if (!m_Initialized) return;

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
    Onyx::RenderCommand::SetClearColor(0.4f, 0.6f, 0.9f, 1.0f);  // Sky blue
    Onyx::RenderCommand::Clear();
    Onyx::RenderCommand::EnableDepthTest();
    Onyx::RenderCommand::EnableCulling();
}

void GameRenderer::RenderTerrain(ClientTerrainSystem& terrain) {
    if (!m_Initialized || !terrain.HasChunks()) return;

    m_TerrainShader->Bind();
    m_TerrainShader->SetMat4("u_View", m_ViewMatrix);
    m_TerrainShader->SetMat4("u_Projection", m_ProjMatrix);
    m_TerrainShader->SetVec3("u_LightDir", m_SunDirection);
    m_TerrainShader->SetVec3("u_LightColor", m_SunColor);
    m_TerrainShader->SetFloat("u_AmbientStrength", m_AmbientStrength);

    terrain.Render(m_TerrainShader.get());
    m_TerrainShader->UnBind();
}

void GameRenderer::RenderEntities(const LocalPlayer& player,
                                   const std::unordered_map<EntityId, RemoteEntity>& entities,
                                   const std::vector<GameClient::ClientPortal>& portals,
                                   const std::vector<GameClient::ClientProjectile>& projectiles,
                                   const ClientTerrainSystem& terrain) {
    if (!m_Initialized) return;

    m_EntityShader->Bind();
    m_EntityShader->SetMat4("u_View", m_ViewMatrix);
    m_EntityShader->SetMat4("u_Projection", m_ProjMatrix);
    m_EntityShader->SetVec3("u_LightDir", m_SunDirection);
    m_EntityShader->SetVec3("u_LightColor", m_SunColor);
    m_EntityShader->SetFloat("u_AmbientStrength", m_AmbientStrength);

    // Draw local player
    {
        float h = terrain.GetHeightAt(player.position.x, player.position.y);
        glm::vec3 pos3D(player.position.x, h, player.position.y);

        glm::vec4 color;
        switch (player.characterClass) {
            case CharacterClass::WARRIOR:
                color = glm::vec4(0.4f, 0.6f, 1.0f, 1.0f);  // Blue
                break;
            case CharacterClass::WITCH:
                color = glm::vec4(0.8f, 0.4f, 1.0f, 1.0f);  // Purple
                break;
            default:
                color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
        }
        DrawCube(pos3D, glm::vec3(0.5f, 1.0f, 0.5f), color);
    }

    // Draw remote entities
    for (const auto& [id, entity] : entities) {
        float h = terrain.GetHeightAt(entity.position.x, entity.position.y);
        glm::vec3 pos3D(entity.position.x, h, entity.position.y);

        glm::vec4 color;
        bool isDead = entity.health <= 0;

        switch (entity.type) {
            case EntityType::PLAYER:
                color = glm::vec4(0.0f, 0.8f, 0.0f, 1.0f);  // Green
                break;
            case EntityType::MOB:
                if (isDead)
                    color = glm::vec4(0.3f, 0.25f, 0.25f, 1.0f);  // Dark gray
                else
                    color = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);  // Red
                break;
            default:
                color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                break;
        }

        float cubeHeight = isDead ? 0.3f : 1.0f;
        DrawCube(pos3D, glm::vec3(0.5f, cubeHeight, 0.5f), color);

        // Draw target indicator as wireframe
        if (id == player.targetId) {
            // Draw a slightly larger wireframe cube
            Onyx::RenderCommand::SetWireframeMode(true);
            DrawCube(pos3D, glm::vec3(0.6f, cubeHeight + 0.1f, 0.6f),
                     glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
            Onyx::RenderCommand::SetWireframeMode(false);
        }
    }

    // Draw portals
    for (const auto& portal : portals) {
        float h = terrain.GetHeightAt(portal.position.x, portal.position.y);
        glm::vec3 pos3D(portal.position.x, h, portal.position.y);
        DrawCube(pos3D, glm::vec3(portal.size.x * 0.5f, 2.0f, portal.size.y * 0.5f),
                 glm::vec4(0.4f, 0.8f, 1.0f, 0.6f));
    }

    // Draw projectiles as small cubes
    for (const auto& proj : projectiles) {
        float h = terrain.GetHeightAt(proj.position.x, proj.position.y);
        glm::vec3 pos3D(proj.position.x, h + 1.0f, proj.position.y);
        DrawCube(pos3D, glm::vec3(0.2f, 0.2f, 0.2f),
                 glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
    }

    m_EntityShader->UnBind();
}

void GameRenderer::EndFrame() {
    if (!m_Initialized) return;

    Onyx::RenderCommand::DisableDepthTest();
}

glm::vec2 GameRenderer::ProjectToScreen(const glm::vec3& worldPos,
                                          float viewportWidth, float viewportHeight) const {
    glm::vec4 clip = m_ProjMatrix * m_ViewMatrix * glm::vec4(worldPos, 1.0f);
    if (clip.w <= 0.0f) return glm::vec2(-1.0f);

    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    float screenX = (ndc.x * 0.5f + 0.5f) * viewportWidth;
    float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportHeight;

    return glm::vec2(screenX, screenY);
}

void GameRenderer::DrawCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec4& color) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position + glm::vec3(0, scale.y, 0));  // Bottom at ground level
    model = glm::scale(model, scale);

    m_EntityShader->SetMat4("u_Model", model);
    m_EntityShader->SetVec4("u_Color", color.r, color.g, color.b, color.a);

    m_CubeVAO->Bind();
    Onyx::RenderCommand::DrawIndexed(*m_CubeVAO, m_CubeIndexCount);
    m_CubeVAO->UnBind();
}

void GameRenderer::InitCubeMesh() {
    // Unit cube centered at origin, with normals
    float vertices[] = {
        // Front face (normal: 0,0,1)
        -1, -1,  1,   0, 0, 1,
         1, -1,  1,   0, 0, 1,
         1,  1,  1,   0, 0, 1,
        -1,  1,  1,   0, 0, 1,
        // Back face (normal: 0,0,-1)
        -1, -1, -1,   0, 0,-1,
        -1,  1, -1,   0, 0,-1,
         1,  1, -1,   0, 0,-1,
         1, -1, -1,   0, 0,-1,
        // Top face (normal: 0,1,0)
        -1,  1, -1,   0, 1, 0,
        -1,  1,  1,   0, 1, 0,
         1,  1,  1,   0, 1, 0,
         1,  1, -1,   0, 1, 0,
        // Bottom face (normal: 0,-1,0)
        -1, -1, -1,   0,-1, 0,
         1, -1, -1,   0,-1, 0,
         1, -1,  1,   0,-1, 0,
        -1, -1,  1,   0,-1, 0,
        // Right face (normal: 1,0,0)
         1, -1, -1,   1, 0, 0,
         1,  1, -1,   1, 0, 0,
         1,  1,  1,   1, 0, 0,
         1, -1,  1,   1, 0, 0,
        // Left face (normal: -1,0,0)
        -1, -1, -1,  -1, 0, 0,
        -1, -1,  1,  -1, 0, 0,
        -1,  1,  1,  -1, 0, 0,
        -1,  1, -1,  -1, 0, 0,
    };

    uint32_t indices[] = {
        0,1,2,  0,2,3,      // front
        4,5,6,  4,6,7,      // back
        8,9,10, 8,10,11,    // top
        12,13,14, 12,14,15, // bottom
        16,17,18, 16,18,19, // right
        20,21,22, 20,22,23  // left
    };

    m_CubeIndexCount = 36;

    Onyx::RenderCommand::ResetState();

    m_CubeVAO = std::make_unique<Onyx::VertexArray>();
    m_CubeVBO = std::make_unique<Onyx::VertexBuffer>(vertices, sizeof(vertices));
    m_CubeEBO = std::make_unique<Onyx::IndexBuffer>(indices, sizeof(indices));

    Onyx::VertexLayout layout({
        { Onyx::VertexAttributeType::Float3 },  // position
        { Onyx::VertexAttributeType::Float3 }   // normal
    });

    m_CubeVAO->SetVertexBuffer(m_CubeVBO.get());
    m_CubeVAO->SetLayout(layout);
    m_CubeVAO->SetIndexBuffer(m_CubeEBO.get());
    m_CubeVAO->UnBind();
}

} // namespace MMO
