#pragma once

#include "WorldObject.h"

namespace MMO {

class InstancePortal : public WorldObject {
public:
    InstancePortal(uint64_t guid, const std::string& name = "")
        : WorldObject(guid, WorldObjectType::INSTANCE_PORTAL, name) {}

    // Dungeon reference
    void SetDungeonId(uint32_t id) { m_DungeonId = id; }
    uint32_t GetDungeonId() const { return m_DungeonId; }

    void SetDungeonName(const std::string& name) { m_DungeonName = name; }
    const std::string& GetDungeonName() const { return m_DungeonName; }

    // Party requirements
    void SetMinPlayers(uint32_t min) { m_MinPlayers = min; }
    uint32_t GetMinPlayers() const { return m_MinPlayers; }

    void SetMaxPlayers(uint32_t max) { m_MaxPlayers = max; }
    uint32_t GetMaxPlayers() const { return m_MaxPlayers; }

    // Level requirements
    void SetMinLevel(uint32_t level) { m_MinLevel = level; }
    uint32_t GetMinLevel() const { return m_MinLevel; }

    void SetMaxLevel(uint32_t level) { m_MaxLevel = level; }
    uint32_t GetMaxLevel() const { return m_MaxLevel; }

    // Exit location (where players go when leaving instance)
    void SetExitPosition(const glm::vec3& pos) { m_ExitPosition = pos; }
    const glm::vec3& GetExitPosition() const { return m_ExitPosition; }

    void SetExitMapId(uint32_t mapId) { m_ExitMapId = mapId; }
    uint32_t GetExitMapId() const { return m_ExitMapId; }

    // Visual
    void SetInteractionRadius(float radius) { m_InteractionRadius = radius; }
    float GetInteractionRadius() const { return m_InteractionRadius; }

    const char* GetTypeName() const override { return "Instance Portal"; }

    std::unique_ptr<WorldObject> Clone(uint64_t newGuid) const override {
        auto copy = std::make_unique<InstancePortal>(newGuid, GetName() + " Copy");
        copy->SetPosition(GetPosition());
        copy->SetRotation(GetRotation());
        copy->SetScale(GetScale());
        copy->m_DungeonId = m_DungeonId;
        copy->m_DungeonName = m_DungeonName;
        copy->m_MinPlayers = m_MinPlayers;
        copy->m_MaxPlayers = m_MaxPlayers;
        copy->m_MinLevel = m_MinLevel;
        copy->m_MaxLevel = m_MaxLevel;
        copy->m_ExitPosition = m_ExitPosition;
        copy->m_ExitMapId = m_ExitMapId;
        copy->m_InteractionRadius = m_InteractionRadius;
        return copy;
    }

private:
    uint32_t m_DungeonId = 0;
    std::string m_DungeonName;

    uint32_t m_MinPlayers = 1;
    uint32_t m_MaxPlayers = 4;
    uint32_t m_MinLevel = 1;
    uint32_t m_MaxLevel = 100;

    glm::vec3 m_ExitPosition = glm::vec3(0.0f);
    uint32_t m_ExitMapId = 0;

    float m_InteractionRadius = 3.0f;
};

} // namespace MMO
