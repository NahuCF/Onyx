#pragma once

#include "IsometricCamera.h"
#include "../Terrain/ClientTerrainSystem.h"
#include "../GameClient.h"
#include <Onyx.h>
#include <glm/glm.hpp>
#include <memory>

namespace MMO {

class GameRenderer {
public:
    GameRenderer();
    ~GameRenderer();

    void Init();

    void BeginFrame(const glm::vec3& playerPos, float dt, uint32_t viewportWidth, uint32_t viewportHeight);
    void RenderTerrain(ClientTerrainSystem& terrain);
    void RenderEntities(const LocalPlayer& player, const std::unordered_map<EntityId, RemoteEntity>& entities,
                        const std::vector<GameClient::ClientPortal>& portals,
                        const std::vector<GameClient::ClientProjectile>& projectiles,
                        const ClientTerrainSystem& terrain);
    void EndFrame();

    IsometricCamera& GetCamera() { return m_Camera; }
    const IsometricCamera& GetCamera() const { return m_Camera; }

    // Project world position to screen coordinates (relative to viewport)
    glm::vec2 ProjectToScreen(const glm::vec3& worldPos, float viewportWidth, float viewportHeight) const;

private:
    void DrawCube(const glm::vec3& position, const glm::vec3& scale, const glm::vec4& color);
    void InitCubeMesh();

    IsometricCamera m_Camera;

    std::unique_ptr<Onyx::Shader> m_TerrainShader;
    std::unique_ptr<Onyx::Shader> m_EntityShader;

    // Cube mesh for entities
    std::unique_ptr<Onyx::VertexArray> m_CubeVAO;
    std::unique_ptr<Onyx::VertexBuffer> m_CubeVBO;
    std::unique_ptr<Onyx::IndexBuffer> m_CubeEBO;
    uint32_t m_CubeIndexCount = 0;

    // Default texture for terrain (white 1x1 as fallback)
    std::unique_ptr<Onyx::Texture> m_WhiteTexture;

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
