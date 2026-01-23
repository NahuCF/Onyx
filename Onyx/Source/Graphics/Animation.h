#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Onyx {

struct PositionKey {
    float time;
    glm::vec3 position;
};

struct RotationKey {
    float time;
    glm::quat rotation;
};

struct ScaleKey {
    float time;
    glm::vec3 scale;
};

struct BoneAnimation {
    std::string boneName;
    std::vector<PositionKey> positionKeys;
    std::vector<RotationKey> rotationKeys;
    std::vector<ScaleKey> scaleKeys;

    glm::vec3 InterpolatePosition(float time) const;
    glm::quat InterpolateRotation(float time) const;
    glm::vec3 InterpolateScale(float time) const;

private:
    template<typename KeyType>
    int FindKeyIndex(const std::vector<KeyType>& keys, float time) const;
};

class Animation {
public:
    Animation() = default;
    Animation(const std::string& name, float duration, float ticksPerSecond);

    void AddBoneAnimation(const BoneAnimation& boneAnim);

    const std::string& GetName() const { return m_Name; }
    float GetDuration() const { return m_Duration; }
    float GetTicksPerSecond() const { return m_TicksPerSecond; }
    float GetDurationInSeconds() const { return m_Duration / m_TicksPerSecond; }

    const BoneAnimation* GetBoneAnimation(const std::string& boneName) const;
    const std::vector<BoneAnimation>& GetBoneAnimations() const { return m_BoneAnimations; }

private:
    std::string m_Name;
    float m_Duration = 0.0f;
    float m_TicksPerSecond = 25.0f;
    std::vector<BoneAnimation> m_BoneAnimations;
    std::unordered_map<std::string, int> m_BoneAnimationMap;
};

} // namespace Onyx
