#pragma once

#include "WorldObject.h"

namespace MMO {

enum class TriggerShape : uint8_t {
    BOX = 0,
    SPHERE,
    CAPSULE,
};

enum class TriggerEvent : uint8_t {
    ON_ENTER = 0,
    ON_EXIT,
    ON_STAY,
};

class TriggerVolume : public WorldObject {
public:
    TriggerVolume(uint64_t guid, const std::string& name = "")
        : WorldObject(guid, WorldObjectType::TRIGGER_VOLUME, name) {}

    // Shape
    void SetShape(TriggerShape shape) { m_Shape = shape; }
    TriggerShape GetShape() const { return m_Shape; }

    void SetHalfExtents(const glm::vec3& halfExtents) { m_HalfExtents = halfExtents; }
    const glm::vec3& GetHalfExtents() const { return m_HalfExtents; }

    void SetRadius(float radius) { m_Radius = radius; }
    float GetRadius() const { return m_Radius; }

    // Trigger behavior
    void SetTriggerOnce(bool once) { m_TriggerOnce = once; }
    bool IsTriggerOnce() const { return m_TriggerOnce; }

    void SetTriggerEvent(TriggerEvent event) { m_TriggerEvent = event; }
    TriggerEvent GetTriggerEvent() const { return m_TriggerEvent; }

    // Script/Event reference
    void SetScriptName(const std::string& name) { m_ScriptName = name; }
    const std::string& GetScriptName() const { return m_ScriptName; }

    void SetEventId(uint32_t id) { m_EventId = id; }
    uint32_t GetEventId() const { return m_EventId; }

    // Filter - what can trigger this
    void SetTriggerPlayers(bool players) { m_TriggerPlayers = players; }
    bool TriggerPlayers() const { return m_TriggerPlayers; }

    void SetTriggerCreatures(bool creatures) { m_TriggerCreatures = creatures; }
    bool TriggerCreatures() const { return m_TriggerCreatures; }

    const char* GetTypeName() const override { return "Trigger Volume"; }

    std::unique_ptr<WorldObject> Clone(uint64_t newGuid) const override {
        auto copy = std::make_unique<TriggerVolume>(newGuid, GetName() + " Copy");
        copy->SetPosition(GetPosition());
        copy->SetRotation(GetRotation());
        copy->SetScale(GetScale());
        copy->m_Shape = m_Shape;
        copy->m_HalfExtents = m_HalfExtents;
        copy->m_Radius = m_Radius;
        copy->m_TriggerOnce = m_TriggerOnce;
        copy->m_TriggerEvent = m_TriggerEvent;
        copy->m_ScriptName = m_ScriptName;
        copy->m_EventId = m_EventId;
        copy->m_TriggerPlayers = m_TriggerPlayers;
        copy->m_TriggerCreatures = m_TriggerCreatures;
        return copy;
    }

private:
    // Shape
    TriggerShape m_Shape = TriggerShape::BOX;
    glm::vec3 m_HalfExtents = glm::vec3(1.0f);  // For BOX
    float m_Radius = 1.0f;                       // For SPHERE/CAPSULE

    // Behavior
    bool m_TriggerOnce = false;
    TriggerEvent m_TriggerEvent = TriggerEvent::ON_ENTER;

    // Event
    std::string m_ScriptName;
    uint32_t m_EventId = 0;

    // Filter
    bool m_TriggerPlayers = true;
    bool m_TriggerCreatures = false;
};

} // namespace MMO
