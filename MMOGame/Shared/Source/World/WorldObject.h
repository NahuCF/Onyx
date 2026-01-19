#pragma once

#include "Transform.h"
#include <string>
#include <cstdint>
#include <memory>

namespace MMO {

enum class WorldObjectType : uint8_t {
    NONE = 0,
    STATIC_OBJECT,      // Buildings, props, rocks, trees
    SPAWN_POINT,        // Creature spawn locations
    LIGHT,              // Point, spot, directional lights
    PARTICLE_EMITTER,   // VFX placement
    TRIGGER_VOLUME,     // Invisible trigger areas
    INSTANCE_PORTAL,    // Dungeon entrance/exit
    SOUND_EMITTER,      // Ambient sound sources
    WAYPOINT,           // Patrol path points
    GROUP,              // Container for organizing objects
};

class WorldObject {
public:
    WorldObject(uint64_t guid, WorldObjectType type, const std::string& name = "")
        : m_Guid(guid)
        , m_Type(type)
        , m_Name(name) {}

    virtual ~WorldObject() = default;

    // Identification
    uint64_t GetGuid() const { return m_Guid; }
    WorldObjectType GetObjectType() const { return m_Type; }

    const std::string& GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }

    // Transform
    Transform& GetTransform() { return m_Transform; }
    const Transform& GetTransform() const { return m_Transform; }

    void SetPosition(const glm::vec3& pos) { m_Transform.position = pos; }
    const glm::vec3& GetPosition() const { return m_Transform.position; }

    void SetRotation(const glm::quat& rot) { m_Transform.rotation = rot; }
    const glm::quat& GetRotation() const { return m_Transform.rotation; }

    void SetScale(float scale) { m_Transform.scale = scale; }
    float GetScale() const { return m_Transform.scale; }

    void SetEulerAngles(const glm::vec3& euler) { m_Transform.SetEulerAngles(euler); }
    glm::vec3 GetEulerAngles() const { return m_Transform.GetEulerAngles(); }

    // Local transform matrix (relative to parent)
    glm::mat4 GetLocalMatrix() const { return m_Transform.GetMatrix(); }

    // Parent-child hierarchy
    void SetParent(uint64_t parentGuid) { m_ParentGuid = parentGuid; }
    uint64_t GetParentGuid() const { return m_ParentGuid; }
    bool HasParent() const { return m_ParentGuid != 0; }

    // Editor state
    bool IsSelected() const { return m_Selected; }
    void SetSelected(bool selected) { m_Selected = selected; }

    bool IsVisible() const { return m_Visible; }
    void SetVisible(bool visible) { m_Visible = visible; }

    bool IsLocked() const { return m_Locked; }
    void SetLocked(bool locked) { m_Locked = locked; }

    // Type name for UI
    virtual const char* GetTypeName() const { return "WorldObject"; }

    // Clone for copy/paste - each subclass implements deep copy
    virtual std::unique_ptr<WorldObject> Clone(uint64_t newGuid) const = 0;

protected:
    uint64_t m_Guid;
    WorldObjectType m_Type;
    std::string m_Name;
    Transform m_Transform;
    uint64_t m_ParentGuid = 0;  // 0 = no parent (root level)

    // Editor state (not serialized to game data)
    bool m_Selected = false;
    bool m_Visible = true;
    bool m_Locked = false;
};

} // namespace MMO
