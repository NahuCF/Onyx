#pragma once

#include "AnimatedModel.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Onyx {

class Animator {
public:
    Animator() = default;
    explicit Animator(AnimatedModel* model);

    void SetModel(AnimatedModel* model);

    void Play(const std::string& animationName, bool loop = true);
    void Play(int animationIndex, bool loop = true);
    void Stop();
    void Pause();
    void Resume();

    void Update(float deltaTime);

    // Blending (smooth transition between animations)
    void BlendTo(const std::string& animationName, float blendDuration, bool loop = true);
    void BlendTo(int animationIndex, float blendDuration, bool loop = true);

    // State
    bool IsPlaying() const { return m_Playing; }
    bool IsPaused() const { return m_Paused; }
    float GetCurrentTime() const { return m_CurrentTime; }
    float GetNormalizedTime() const;
    const Animation* GetCurrentAnimation() const { return m_CurrentAnimation; }

    // Playback control
    void SetSpeed(float speed) { m_PlaybackSpeed = speed; }
    float GetSpeed() const { return m_PlaybackSpeed; }
    void SetTime(float time);

    // Get final bone matrices for rendering
    const std::vector<glm::mat4>& GetBoneMatrices() const { return m_FinalBoneMatrices; }

private:
    void CalculateBoneTransform(int nodeIndex, const glm::mat4& parentTransform);
    glm::mat4 GetNodeTransform(const std::string& nodeName, float animTime);

    AnimatedModel* m_Model = nullptr;
    Animation* m_CurrentAnimation = nullptr;

    float m_CurrentTime = 0.0f;
    float m_PlaybackSpeed = 1.0f;
    bool m_Playing = false;
    bool m_Paused = false;
    bool m_Loop = true;

    // Blending
    Animation* m_BlendFromAnimation = nullptr;
    float m_BlendFromTime = 0.0f;
    float m_BlendFactor = 0.0f;
    float m_BlendDuration = 0.0f;
    bool m_IsBlending = false;

    // Final matrices
    std::vector<glm::mat4> m_FinalBoneMatrices;

    // Node transforms cache (for hierarchy traversal)
    struct NodeTransformCache {
        std::string name;
        glm::mat4 localTransform;
        int parentIndex;
        std::vector<int> children;
    };
    std::vector<NodeTransformCache> m_NodeCache;
    bool m_NodeCacheValid = false;
    void BuildNodeCache();
};

} // namespace Onyx
