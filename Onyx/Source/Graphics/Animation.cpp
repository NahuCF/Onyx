#include "pch.h"
#include "Animation.h"
#include <algorithm>

namespace Onyx {

template<typename KeyType>
int BoneAnimation::FindKeyIndex(const std::vector<KeyType>& keys, float time) const {
    for (size_t i = 0; i < keys.size() - 1; i++) {
        if (time < keys[i + 1].time) {
            return static_cast<int>(i);
        }
    }
    return static_cast<int>(keys.size() - 2);
}

glm::vec3 BoneAnimation::InterpolatePosition(float time) const {
    if (positionKeys.empty()) {
        return glm::vec3(0.0f);
    }
    if (positionKeys.size() == 1) {
        return positionKeys[0].position;
    }

    int index = FindKeyIndex(positionKeys, time);
    int nextIndex = index + 1;

    float deltaTime = positionKeys[nextIndex].time - positionKeys[index].time;
    float factor = (time - positionKeys[index].time) / deltaTime;
    factor = glm::clamp(factor, 0.0f, 1.0f);

    return glm::mix(positionKeys[index].position, positionKeys[nextIndex].position, factor);
}

glm::quat BoneAnimation::InterpolateRotation(float time) const {
    if (rotationKeys.empty()) {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }
    if (rotationKeys.size() == 1) {
        return rotationKeys[0].rotation;
    }

    int index = FindKeyIndex(rotationKeys, time);
    int nextIndex = index + 1;

    float deltaTime = rotationKeys[nextIndex].time - rotationKeys[index].time;
    float factor = (time - rotationKeys[index].time) / deltaTime;
    factor = glm::clamp(factor, 0.0f, 1.0f);

    return glm::slerp(rotationKeys[index].rotation, rotationKeys[nextIndex].rotation, factor);
}

glm::vec3 BoneAnimation::InterpolateScale(float time) const {
    if (scaleKeys.empty()) {
        return glm::vec3(1.0f);
    }
    if (scaleKeys.size() == 1) {
        return scaleKeys[0].scale;
    }

    int index = FindKeyIndex(scaleKeys, time);
    int nextIndex = index + 1;

    float deltaTime = scaleKeys[nextIndex].time - scaleKeys[index].time;
    float factor = (time - scaleKeys[index].time) / deltaTime;
    factor = glm::clamp(factor, 0.0f, 1.0f);

    return glm::mix(scaleKeys[index].scale, scaleKeys[nextIndex].scale, factor);
}

Animation::Animation(const std::string& name, float duration, float ticksPerSecond)
    : m_Name(name), m_Duration(duration), m_TicksPerSecond(ticksPerSecond) {
    if (m_TicksPerSecond <= 0.0f) {
        m_TicksPerSecond = 25.0f;
    }
}

void Animation::AddBoneAnimation(const BoneAnimation& boneAnim) {
    m_BoneAnimationMap[boneAnim.boneName] = static_cast<int>(m_BoneAnimations.size());
    m_BoneAnimations.push_back(boneAnim);
}

const BoneAnimation* Animation::GetBoneAnimation(const std::string& boneName) const {
    auto it = m_BoneAnimationMap.find(boneName);
    if (it != m_BoneAnimationMap.end()) {
        return &m_BoneAnimations[it->second];
    }
    return nullptr;
}

} // namespace Onyx
