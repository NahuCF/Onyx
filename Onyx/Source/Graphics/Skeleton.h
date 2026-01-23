#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Onyx {

struct Bone {
    std::string name;
    int index = -1;
    int parentIndex = -1;
    glm::mat4 offsetMatrix = glm::mat4(1.0f);  // Inverse bind pose (mesh space -> bone space)
    glm::mat4 localBindTransform = glm::mat4(1.0f);  // Local transform in bind pose
};

class Skeleton {
public:
    static constexpr int MAX_BONES = 100;

    Skeleton() = default;

    void AddBone(const std::string& name, int parentIndex, const glm::mat4& offsetMatrix);
    void SetBoneLocalTransform(int index, const glm::mat4& transform);

    int GetBoneIndex(const std::string& name) const;
    const Bone* GetBone(int index) const;
    const Bone* GetBone(const std::string& name) const;
    int GetBoneCount() const { return static_cast<int>(m_Bones.size()); }

    const std::vector<Bone>& GetBones() const { return m_Bones; }
    const glm::mat4& GetGlobalInverseTransform() const { return m_GlobalInverseTransform; }
    void SetGlobalInverseTransform(const glm::mat4& transform) { m_GlobalInverseTransform = transform; }

private:
    std::vector<Bone> m_Bones;
    std::unordered_map<std::string, int> m_BoneMap;
    glm::mat4 m_GlobalInverseTransform = glm::mat4(1.0f);
};

} // namespace Onyx
