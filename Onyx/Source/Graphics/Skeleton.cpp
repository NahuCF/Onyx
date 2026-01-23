#include "pch.h"
#include "Skeleton.h"

namespace Onyx {

void Skeleton::AddBone(const std::string& name, int parentIndex, const glm::mat4& offsetMatrix) {
    if (m_BoneMap.find(name) != m_BoneMap.end()) {
        return;  // Bone already exists
    }

    Bone bone;
    bone.name = name;
    bone.index = static_cast<int>(m_Bones.size());
    bone.parentIndex = parentIndex;
    bone.offsetMatrix = offsetMatrix;

    m_BoneMap[name] = bone.index;
    m_Bones.push_back(bone);
}

void Skeleton::SetBoneLocalTransform(int index, const glm::mat4& transform) {
    if (index >= 0 && index < static_cast<int>(m_Bones.size())) {
        m_Bones[index].localBindTransform = transform;
    }
}

int Skeleton::GetBoneIndex(const std::string& name) const {
    auto it = m_BoneMap.find(name);
    if (it != m_BoneMap.end()) {
        return it->second;
    }
    return -1;
}

const Bone* Skeleton::GetBone(int index) const {
    if (index >= 0 && index < static_cast<int>(m_Bones.size())) {
        return &m_Bones[index];
    }
    return nullptr;
}

const Bone* Skeleton::GetBone(const std::string& name) const {
    int index = GetBoneIndex(name);
    return GetBone(index);
}

} // namespace Onyx
