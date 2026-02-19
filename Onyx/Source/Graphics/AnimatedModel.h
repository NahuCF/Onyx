#pragma once

#include "Skeleton.h"
#include "Animation.h"
#include "Buffers.h"
#include "Texture.h"
#include "Model.h"
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

    std::vector<SkinnedVertex> vertices;
    std::vector<uint32_t> indices;
};

struct MergedMeshInfo;

struct SkinnedMergedBuffers {
    std::unique_ptr<VertexArray> vao;
    std::unique_ptr<VertexBuffer> vbo;
    std::unique_ptr<IndexBuffer> ebo;
    uint32_t totalVertices = 0;
    uint32_t totalIndices = 0;
    std::vector<MergedMeshInfo> meshInfos;
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
    ~AnimatedModel();

    bool Load(const std::string& path);

    bool LoadAnimation(const std::string& path);

    Skeleton& GetSkeleton() { return m_Skeleton; }
    const Skeleton& GetSkeleton() const { return m_Skeleton; }

    const std::vector<std::unique_ptr<Animation>>& GetAnimations() const { return m_Animations; }
    Animation* GetAnimation(const std::string& name);
    Animation* GetAnimation(int index);
    int GetAnimationCount() const { return static_cast<int>(m_Animations.size()); }
    std::vector<std::string> GetAnimationNames() const;

    void SetBoneMatrices(const std::vector<glm::mat4>& matrices) { m_BoneMatrices = matrices; }
    const std::vector<glm::mat4>& GetBoneMatrices() const { return m_BoneMatrices; }

    const std::vector<SkinnedMesh>& GetMeshes() const { return m_Meshes; }
    const std::vector<AnimatedMaterial>& GetMaterials() const { return m_Materials; }

    const glm::vec3& GetBoundsMin() const { return m_BoundsMin; }
    const glm::vec3& GetBoundsMax() const { return m_BoundsMax; }

    const std::string& GetPath() const { return m_Path; }

    void BuildMergedBuffers();
    const SkinnedMergedBuffers& GetMergedBuffers() const { return m_Merged; }
    bool HasMergedBuffers() const { return m_Merged.vao != nullptr; }

    struct NodeData {
        std::string name;
        glm::mat4 transform;
        int parentIndex;
        std::vector<int> children;
    };
    const std::vector<NodeData>& GetNodeHierarchy() const { return m_NodeHierarchy; }
    int GetNodeIndex(const std::string& name) const {
        auto it = m_NodeMap.find(name);
        return it != m_NodeMap.end() ? it->second : -1;
    }

private:
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

    std::vector<glm::mat4> m_BoneMatrices;

    glm::vec3 m_BoundsMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 m_BoundsMax = glm::vec3(std::numeric_limits<float>::lowest());

    std::vector<NodeData> m_NodeHierarchy;
    std::unordered_map<std::string, int> m_NodeMap;

    SkinnedMergedBuffers m_Merged;
};

} // namespace Onyx
