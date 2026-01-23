#pragma once

#include "Skeleton.h"
#include "Animation.h"
#include "Buffers.h"
#include "Texture.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Onyx {

struct SkinnedVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::ivec4 boneIds = glm::ivec4(-1);
    glm::vec4 boneWeights = glm::vec4(0.0f);

    void AddBoneData(int boneId, float weight);
};

struct SkinnedMesh {
    std::unique_ptr<VertexArray> vao;
    std::unique_ptr<VertexBuffer> vbo;
    std::unique_ptr<IndexBuffer> ebo;
    uint32_t indexCount = 0;
    uint32_t materialIndex = 0;
    std::string name;
};

struct AnimatedMaterial {
    std::string name;
    glm::vec3 diffuseColor = glm::vec3(1.0f);
    glm::vec3 specularColor = glm::vec3(1.0f);
    float shininess = 32.0f;
    std::unique_ptr<Texture> diffuseTexture;
    std::unique_ptr<Texture> normalTexture;
    std::unique_ptr<Texture> specularTexture;
};

class AnimatedModel {
public:
    AnimatedModel() = default;
    ~AnimatedModel() = default;

    bool Load(const std::string& path);

    // Skeleton access
    Skeleton& GetSkeleton() { return m_Skeleton; }
    const Skeleton& GetSkeleton() const { return m_Skeleton; }

    // Animation access
    const std::vector<std::unique_ptr<Animation>>& GetAnimations() const { return m_Animations; }
    Animation* GetAnimation(const std::string& name);
    Animation* GetAnimation(int index);
    int GetAnimationCount() const { return static_cast<int>(m_Animations.size()); }
    std::vector<std::string> GetAnimationNames() const;

    // Bone matrices (updated by Animator)
    void SetBoneMatrices(const std::vector<glm::mat4>& matrices) { m_BoneMatrices = matrices; }
    const std::vector<glm::mat4>& GetBoneMatrices() const { return m_BoneMatrices; }

    // Mesh access for rendering
    const std::vector<SkinnedMesh>& GetMeshes() const { return m_Meshes; }
    const std::vector<AnimatedMaterial>& GetMaterials() const { return m_Materials; }

    // Bounds
    const glm::vec3& GetBoundsMin() const { return m_BoundsMin; }
    const glm::vec3& GetBoundsMax() const { return m_BoundsMax; }

    const std::string& GetPath() const { return m_Path; }

private:
    // Implementation functions (defined in .cpp, use Assimp types)
    void ProcessNodeImpl(void* node, const void* scene);
    void ProcessMeshImpl(void* mesh, const void* scene);
    void ProcessBonesImpl(void* mesh, std::vector<SkinnedVertex>& vertices);
    void LoadAnimationsImpl(const void* scene);
    void LoadMaterialsImpl(const void* scene);
    void BuildNodeHierarchyImpl(void* node, int parentIndex);

    std::string m_Path;
    std::string m_Directory;

    Skeleton m_Skeleton;
    std::vector<std::unique_ptr<Animation>> m_Animations;
    std::unordered_map<std::string, int> m_AnimationMap;

    std::vector<SkinnedMesh> m_Meshes;
    std::vector<AnimatedMaterial> m_Materials;

    // Final bone matrices for GPU
    std::vector<glm::mat4> m_BoneMatrices;

    // Bounding box
    glm::vec3 m_BoundsMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 m_BoundsMax = glm::vec3(std::numeric_limits<float>::lowest());

    // Node hierarchy for animation (maps node name to its local transform)
    struct NodeData {
        std::string name;
        glm::mat4 transform;
        int parentIndex;
        std::vector<int> children;
    };
    std::vector<NodeData> m_NodeHierarchy;
    std::unordered_map<std::string, int> m_NodeMap;
};

} // namespace Onyx
