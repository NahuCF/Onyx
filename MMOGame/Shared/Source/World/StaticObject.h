#pragma once

#include "WorldObject.h"
#include <unordered_map>
#include <vector>

namespace MMO {

enum class ColliderType : uint8_t {
    NONE = 0,
    BOX,
    SPHERE,
    CAPSULE,
    MESH,
};

struct ColliderData {
    ColliderType type = ColliderType::NONE;
    glm::vec3 center = glm::vec3(0.0f);     // Local offset
    glm::vec3 halfExtents = glm::vec3(1.0f); // For BOX
    float radius = 1.0f;                      // For SPHERE/CAPSULE
    float height = 2.0f;                      // For CAPSULE
    uint32_t meshId = 0;                      // For MESH collider
};

// Per-mesh material assignment
struct MeshMaterial {
    std::string materialId;   // Reference to MaterialLibrary
    // Per-mesh transform offset (relative to object)
    glm::vec3 positionOffset = glm::vec3(0.0f);
    glm::vec3 rotationOffset = glm::vec3(0.0f);  // Euler angles in degrees
    float scaleMultiplier = 1.0f;
    // Visibility (for hiding meshes when copying single mesh from model)
    bool visible = true;
};

class StaticObject : public WorldObject {
public:
    StaticObject(uint64_t guid, const std::string& name = "")
        : WorldObject(guid, WorldObjectType::STATIC_OBJECT, name) {}

    // Model
    void SetModelPath(const std::string& path) { m_ModelPath = path; }
    const std::string& GetModelPath() const { return m_ModelPath; }

    void SetModelId(uint32_t id) { m_ModelId = id; }
    uint32_t GetModelId() const { return m_ModelId; }

    // Material (reference to MaterialLibrary)
    void SetMaterialId(const std::string& id) { m_MaterialId = id; }
    const std::string& GetMaterialId() const { return m_MaterialId; }

    // Collision
    void SetCollider(const ColliderData& collider) { m_Collider = collider; }
    ColliderData& GetCollider() { return m_Collider; }
    const ColliderData& GetCollider() const { return m_Collider; }

    bool HasCollision() const { return m_Collider.type != ColliderType::NONE; }

    // Animation support (for animated models)
    void AddAnimationPath(const std::string& path) { m_AnimationPaths.push_back(path); }
    void RemoveAnimationPath(size_t index) {
        if (index < m_AnimationPaths.size()) {
            m_AnimationPaths.erase(m_AnimationPaths.begin() + index);
        }
    }
    const std::vector<std::string>& GetAnimationPaths() const { return m_AnimationPaths; }
    void ClearAnimationPaths() { m_AnimationPaths.clear(); }

    void SetCurrentAnimation(const std::string& name) { m_CurrentAnimation = name; }
    const std::string& GetCurrentAnimation() const { return m_CurrentAnimation; }

    void SetAnimationLoop(bool loop) { m_AnimationLoop = loop; }
    bool GetAnimationLoop() const { return m_AnimationLoop; }

    void SetAnimationSpeed(float speed) { m_AnimationSpeed = speed; }
    float GetAnimationSpeed() const { return m_AnimationSpeed; }

    // Rendering flags
    void SetCastsShadow(bool casts) { m_CastsShadow = casts; }
    bool CastsShadow() const { return m_CastsShadow; }

    void SetReceivesLightmap(bool receives) { m_ReceivesLightmap = receives; }
    bool ReceivesLightmap() const { return m_ReceivesLightmap; }

    // Lightmap data (populated by lightmap baker)
    void SetLightmapIndex(uint32_t index) { m_LightmapIndex = index; }
    uint32_t GetLightmapIndex() const { return m_LightmapIndex; }

    void SetLightmapScaleOffset(const glm::vec4& scaleOffset) { m_LightmapScaleOffset = scaleOffset; }
    const glm::vec4& GetLightmapScaleOffset() const { return m_LightmapScaleOffset; }

    // Per-mesh materials
    void SetMeshMaterial(const std::string& meshName, const MeshMaterial& material) {
        m_MeshMaterials[meshName] = material;
    }
    const MeshMaterial* GetMeshMaterial(const std::string& meshName) const {
        auto it = m_MeshMaterials.find(meshName);
        return it != m_MeshMaterials.end() ? &it->second : nullptr;
    }
    MeshMaterial& GetOrCreateMeshMaterial(const std::string& meshName) {
        return m_MeshMaterials[meshName];
    }
    const std::unordered_map<std::string, MeshMaterial>& GetAllMeshMaterials() const {
        return m_MeshMaterials;
    }
    void ClearMeshMaterials() { m_MeshMaterials.clear(); }

    const char* GetTypeName() const override { return "Static Object"; }

    std::unique_ptr<WorldObject> Clone(uint64_t newGuid) const override {
        auto copy = std::make_unique<StaticObject>(newGuid, GetName() + " Copy");

        // Copy transform
        copy->SetPosition(GetPosition());
        copy->SetRotation(GetRotation());
        copy->SetScale(GetScale());

        // Copy model and material
        copy->m_ModelPath = m_ModelPath;
        copy->m_ModelId = m_ModelId;
        copy->m_MaterialId = m_MaterialId;

        // Copy collider
        copy->m_Collider = m_Collider;

        // Copy all mesh materials (includes per-mesh transforms)
        copy->m_MeshMaterials = m_MeshMaterials;

        // Copy rendering flags
        copy->m_CastsShadow = m_CastsShadow;
        copy->m_ReceivesLightmap = m_ReceivesLightmap;
        copy->m_LightmapIndex = m_LightmapIndex;
        copy->m_LightmapScaleOffset = m_LightmapScaleOffset;

        // Copy animation settings
        copy->m_AnimationPaths = m_AnimationPaths;
        copy->m_CurrentAnimation = m_CurrentAnimation;
        copy->m_AnimationLoop = m_AnimationLoop;
        copy->m_AnimationSpeed = m_AnimationSpeed;

        return copy;
    }

private:
    std::string m_ModelPath;
    uint32_t m_ModelId = 0;
    std::string m_MaterialId;  // MaterialLibrary reference
    ColliderData m_Collider;

    // Per-mesh materials (mesh name -> material)
    std::unordered_map<std::string, MeshMaterial> m_MeshMaterials;

    // Rendering
    bool m_CastsShadow = true;
    bool m_ReceivesLightmap = true;
    uint32_t m_LightmapIndex = 0;
    glm::vec4 m_LightmapScaleOffset = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f); // scale.xy, offset.xy

    // Animation (for animated models with bones)
    std::vector<std::string> m_AnimationPaths;  // Additional animation files (e.g., walking.fbx, running.fbx)
    std::string m_CurrentAnimation;              // Currently selected animation name
    bool m_AnimationLoop = true;
    float m_AnimationSpeed = 1.0f;
};

} // namespace MMO
