#pragma once

#include "WorldObject.h"
#include <unordered_map>

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

// Texture types for materials
enum class TextureType : uint8_t {
    DIFFUSE = 0,    // Albedo/base color
    NORMAL,         // Normal map
    SPECULAR,       // Specular/Roughness
    METALLIC,       // Metallic map
    EMISSIVE,       // Emissive/glow
    AO,             // Ambient occlusion
    COUNT
};

// Per-mesh material assignment
struct MeshMaterial {
    std::string albedoPath;
    std::string normalPath;
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

    // Textures
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

    void SetSpecularTexture(const std::string& path) { SetTexturePath(TextureType::SPECULAR, path); }
    const std::string& GetSpecularTexture() const { return GetTexturePath(TextureType::SPECULAR); }

    void SetMetallicTexture(const std::string& path) { SetTexturePath(TextureType::METALLIC, path); }
    const std::string& GetMetallicTexture() const { return GetTexturePath(TextureType::METALLIC); }

    void SetEmissiveTexture(const std::string& path) { SetTexturePath(TextureType::EMISSIVE, path); }
    const std::string& GetEmissiveTexture() const { return GetTexturePath(TextureType::EMISSIVE); }

    void SetAOTexture(const std::string& path) { SetTexturePath(TextureType::AO, path); }
    const std::string& GetAOTexture() const { return GetTexturePath(TextureType::AO); }

    // Collision
    void SetCollider(const ColliderData& collider) { m_Collider = collider; }
    ColliderData& GetCollider() { return m_Collider; }
    const ColliderData& GetCollider() const { return m_Collider; }

    bool HasCollision() const { return m_Collider.type != ColliderType::NONE; }

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

        // Copy model and textures
        copy->m_ModelPath = m_ModelPath;
        copy->m_ModelId = m_ModelId;
        for (size_t i = 0; i < static_cast<size_t>(TextureType::COUNT); i++) {
            copy->m_TexturePaths[i] = m_TexturePaths[i];
        }

        // Copy collider
        copy->m_Collider = m_Collider;

        // Copy all mesh materials (includes per-mesh transforms)
        copy->m_MeshMaterials = m_MeshMaterials;

        // Copy rendering flags
        copy->m_CastsShadow = m_CastsShadow;
        copy->m_ReceivesLightmap = m_ReceivesLightmap;
        copy->m_LightmapIndex = m_LightmapIndex;
        copy->m_LightmapScaleOffset = m_LightmapScaleOffset;

        return copy;
    }

private:
    std::string m_ModelPath;
    uint32_t m_ModelId = 0;
    std::string m_TexturePaths[static_cast<size_t>(TextureType::COUNT)];
    ColliderData m_Collider;

    // Per-mesh materials (mesh name -> material)
    std::unordered_map<std::string, MeshMaterial> m_MeshMaterials;

    // Rendering
    bool m_CastsShadow = true;
    bool m_ReceivesLightmap = true;
    uint32_t m_LightmapIndex = 0;
    glm::vec4 m_LightmapScaleOffset = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f); // scale.xy, offset.xy
};

} // namespace MMO
