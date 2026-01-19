#pragma once

#include "WorldObject.h"
#include "StaticObject.h"  // For TextureType enum
#include <vector>

namespace MMO {

class SpawnPoint : public WorldObject {
public:
    SpawnPoint(uint64_t guid, const std::string& name = "")
        : WorldObject(guid, WorldObjectType::SPAWN_POINT, name) {}

    // Creature template reference
    void SetCreatureTemplateId(uint32_t id) { m_CreatureTemplateId = id; }
    uint32_t GetCreatureTemplateId() const { return m_CreatureTemplateId; }

    // Preview model (for editor visualization of the creature)
    void SetModelPath(const std::string& path) { m_ModelPath = path; }
    const std::string& GetModelPath() const { return m_ModelPath; }

    // Textures for preview model
    void SetTexturePath(TextureType type, const std::string& path) {
        m_TexturePaths[static_cast<size_t>(type)] = path;
    }
    const std::string& GetTexturePath(TextureType type) const {
        return m_TexturePaths[static_cast<size_t>(type)];
    }

    void SetDiffuseTexture(const std::string& path) { SetTexturePath(TextureType::DIFFUSE, path); }
    const std::string& GetDiffuseTexture() const { return GetTexturePath(TextureType::DIFFUSE); }

    void SetNormalTexture(const std::string& path) { SetTexturePath(TextureType::NORMAL, path); }
    const std::string& GetNormalTexture() const { return GetTexturePath(TextureType::NORMAL); }

    // Spawn behavior
    void SetRespawnTime(float seconds) { m_RespawnTime = seconds; }
    float GetRespawnTime() const { return m_RespawnTime; }

    void SetWanderRadius(float radius) { m_WanderRadius = radius; }
    float GetWanderRadius() const { return m_WanderRadius; }

    void SetMaxCount(uint32_t count) { m_MaxCount = count; }
    uint32_t GetMaxCount() const { return m_MaxCount; }

    // Patrol path (waypoint GUIDs)
    void SetPatrolPath(const std::vector<uint64_t>& path) { m_PatrolPath = path; }
    const std::vector<uint64_t>& GetPatrolPath() const { return m_PatrolPath; }
    void AddWaypointToPath(uint64_t waypointGuid) { m_PatrolPath.push_back(waypointGuid); }
    void ClearPatrolPath() { m_PatrolPath.clear(); }

    // Active state (for editor preview)
    void SetPreviewEnabled(bool enabled) { m_PreviewEnabled = enabled; }
    bool IsPreviewEnabled() const { return m_PreviewEnabled; }

    const char* GetTypeName() const override { return "Spawn Point"; }

    std::unique_ptr<WorldObject> Clone(uint64_t newGuid) const override {
        auto copy = std::make_unique<SpawnPoint>(newGuid, GetName() + " Copy");
        copy->SetPosition(GetPosition());
        copy->SetRotation(GetRotation());
        copy->SetScale(GetScale());
        copy->m_CreatureTemplateId = m_CreatureTemplateId;
        copy->m_RespawnTime = m_RespawnTime;
        copy->m_WanderRadius = m_WanderRadius;
        copy->m_MaxCount = m_MaxCount;
        copy->m_PatrolPath = m_PatrolPath;
        copy->m_ModelPath = m_ModelPath;
        for (size_t i = 0; i < static_cast<size_t>(TextureType::COUNT); i++) {
            copy->m_TexturePaths[i] = m_TexturePaths[i];
        }
        copy->m_PreviewEnabled = m_PreviewEnabled;
        return copy;
    }

private:
    uint32_t m_CreatureTemplateId = 0;
    float m_RespawnTime = 60.0f;
    float m_WanderRadius = 0.0f;
    uint32_t m_MaxCount = 1;
    std::vector<uint64_t> m_PatrolPath;

    // Preview model for editor
    std::string m_ModelPath;
    std::string m_TexturePaths[static_cast<size_t>(TextureType::COUNT)];

    // Editor state
    bool m_PreviewEnabled = false;
};

} // namespace MMO
