#include "pch.h"
#include "Animator.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Onyx {

Animator::Animator(AnimatedModel* model) : m_Model(model) {
    if (m_Model) {
        m_FinalBoneMatrices.resize(Skeleton::MAX_BONES, glm::mat4(1.0f));
        BuildNodeCache();
    }
}

void Animator::SetModel(AnimatedModel* model) {
    m_Model = model;
    m_CurrentAnimation = nullptr;
    m_BlendFromAnimation = nullptr;
    m_CurrentTime = 0.0f;
    m_Playing = false;
    m_IsBlending = false;
    m_NodeCacheValid = false;

    if (m_Model) {
        m_FinalBoneMatrices.resize(Skeleton::MAX_BONES, glm::mat4(1.0f));
        BuildNodeCache();
    }
}

void Animator::BuildNodeCache() {
    if (!m_Model) return;

    // We need access to the node hierarchy from the model
    // For now, build from skeleton
    m_NodeCache.clear();
    const auto& bones = m_Model->GetSkeleton().GetBones();

    for (const auto& bone : bones) {
        NodeTransformCache node;
        node.name = bone.name;
        node.localTransform = bone.localBindTransform;
        node.parentIndex = bone.parentIndex;
        m_NodeCache.push_back(node);
    }

    // Build children lists
    for (int i = 0; i < static_cast<int>(m_NodeCache.size()); i++) {
        int parent = m_NodeCache[i].parentIndex;
        if (parent >= 0 && parent < static_cast<int>(m_NodeCache.size())) {
            m_NodeCache[parent].children.push_back(i);
        }
    }

    m_NodeCacheValid = true;
}

void Animator::Play(const std::string& animationName, bool loop) {
    if (!m_Model) return;

    Animation* anim = m_Model->GetAnimation(animationName);
    if (anim) {
        m_CurrentAnimation = anim;
        m_CurrentTime = 0.0f;
        m_Playing = true;
        m_Paused = false;
        m_Loop = loop;
        m_IsBlending = false;
    }
}

void Animator::Play(int animationIndex, bool loop) {
    if (!m_Model) return;

    Animation* anim = m_Model->GetAnimation(animationIndex);
    if (anim) {
        m_CurrentAnimation = anim;
        m_CurrentTime = 0.0f;
        m_Playing = true;
        m_Paused = false;
        m_Loop = loop;
        m_IsBlending = false;
    }
}

void Animator::Stop() {
    m_Playing = false;
    m_Paused = false;
    m_CurrentTime = 0.0f;
    m_IsBlending = false;
}

void Animator::Pause() {
    if (m_Playing) {
        m_Paused = true;
    }
}

void Animator::Resume() {
    if (m_Paused) {
        m_Paused = false;
    }
}

void Animator::BlendTo(const std::string& animationName, float blendDuration, bool loop) {
    if (!m_Model) return;

    Animation* anim = m_Model->GetAnimation(animationName);
    if (anim && anim != m_CurrentAnimation) {
        m_BlendFromAnimation = m_CurrentAnimation;
        m_BlendFromTime = m_CurrentTime;
        m_CurrentAnimation = anim;
        m_CurrentTime = 0.0f;
        m_BlendDuration = blendDuration;
        m_BlendFactor = 0.0f;
        m_IsBlending = true;
        m_Playing = true;
        m_Paused = false;
        m_Loop = loop;
    }
}

void Animator::BlendTo(int animationIndex, float blendDuration, bool loop) {
    if (!m_Model) return;

    Animation* anim = m_Model->GetAnimation(animationIndex);
    if (anim) {
        BlendTo(anim->GetName(), blendDuration, loop);
    }
}

void Animator::SetTime(float time) {
    m_CurrentTime = time;
    if (m_CurrentAnimation) {
        float duration = m_CurrentAnimation->GetDuration();
        if (m_CurrentTime > duration) {
            m_CurrentTime = m_Loop ? fmod(m_CurrentTime, duration) : duration;
        }
    }
}

float Animator::GetNormalizedTime() const {
    if (!m_CurrentAnimation) return 0.0f;
    float duration = m_CurrentAnimation->GetDuration();
    return duration > 0.0f ? m_CurrentTime / duration : 0.0f;
}

void Animator::Update(float deltaTime) {
    if (!m_Model || !m_CurrentAnimation || !m_Playing || m_Paused) {
        return;
    }

    // Update time
    m_CurrentTime += deltaTime * m_PlaybackSpeed * m_CurrentAnimation->GetTicksPerSecond();

    float duration = m_CurrentAnimation->GetDuration();
    if (m_CurrentTime > duration) {
        if (m_Loop) {
            m_CurrentTime = fmod(m_CurrentTime, duration);
        } else {
            m_CurrentTime = duration;
            m_Playing = false;
        }
    }

    // Update blending
    if (m_IsBlending) {
        m_BlendFactor += deltaTime / m_BlendDuration;
        if (m_BlendFactor >= 1.0f) {
            m_BlendFactor = 1.0f;
            m_IsBlending = false;
            m_BlendFromAnimation = nullptr;
        }
    }

    // Calculate bone transforms
    glm::mat4 identity(1.0f);

    // Find root nodes (nodes with no parent in skeleton)
    for (int i = 0; i < static_cast<int>(m_NodeCache.size()); i++) {
        if (m_NodeCache[i].parentIndex < 0) {
            CalculateBoneTransform(i, identity);
        }
    }

    // Update model's bone matrices
    m_Model->SetBoneMatrices(m_FinalBoneMatrices);
}

glm::mat4 Animator::GetNodeTransform(const std::string& nodeName, float animTime) {
    const BoneAnimation* boneAnim = m_CurrentAnimation->GetBoneAnimation(nodeName);

    if (boneAnim) {
        glm::vec3 position = boneAnim->InterpolatePosition(animTime);
        glm::quat rotation = boneAnim->InterpolateRotation(animTime);
        glm::vec3 scale = boneAnim->InterpolateScale(animTime);

        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    // Return identity if no animation for this node
    return glm::mat4(1.0f);
}

void Animator::CalculateBoneTransform(int nodeIndex, const glm::mat4& parentTransform) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(m_NodeCache.size())) {
        return;
    }

    const auto& node = m_NodeCache[nodeIndex];
    glm::mat4 nodeTransform = node.localTransform;

    // Get animated transform
    if (m_CurrentAnimation) {
        glm::mat4 animTransform = GetNodeTransform(node.name, m_CurrentTime);

        // Handle blending
        if (m_IsBlending && m_BlendFromAnimation) {
            const BoneAnimation* fromBoneAnim = m_BlendFromAnimation->GetBoneAnimation(node.name);
            if (fromBoneAnim) {
                glm::vec3 fromPos = fromBoneAnim->InterpolatePosition(m_BlendFromTime);
                glm::quat fromRot = fromBoneAnim->InterpolateRotation(m_BlendFromTime);
                glm::vec3 fromScale = fromBoneAnim->InterpolateScale(m_BlendFromTime);

                const BoneAnimation* toBoneAnim = m_CurrentAnimation->GetBoneAnimation(node.name);
                if (toBoneAnim) {
                    glm::vec3 toPos = toBoneAnim->InterpolatePosition(m_CurrentTime);
                    glm::quat toRot = toBoneAnim->InterpolateRotation(m_CurrentTime);
                    glm::vec3 toScale = toBoneAnim->InterpolateScale(m_CurrentTime);

                    glm::vec3 blendedPos = glm::mix(fromPos, toPos, m_BlendFactor);
                    glm::quat blendedRot = glm::slerp(fromRot, toRot, m_BlendFactor);
                    glm::vec3 blendedScale = glm::mix(fromScale, toScale, m_BlendFactor);

                    animTransform = glm::translate(glm::mat4(1.0f), blendedPos) *
                                   glm::mat4_cast(blendedRot) *
                                   glm::scale(glm::mat4(1.0f), blendedScale);
                }
            }
        }

        nodeTransform = animTransform;
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // If this node is a bone, update the final matrix
    const Skeleton& skeleton = m_Model->GetSkeleton();
    int boneIndex = skeleton.GetBoneIndex(node.name);
    if (boneIndex >= 0 && boneIndex < static_cast<int>(m_FinalBoneMatrices.size())) {
        const Bone* bone = skeleton.GetBone(boneIndex);
        if (bone) {
            m_FinalBoneMatrices[boneIndex] =
                skeleton.GetGlobalInverseTransform() *
                globalTransform *
                bone->offsetMatrix;
        }
    }

    // Recursively process children
    for (int childIndex : node.children) {
        CalculateBoneTransform(childIndex, globalTransform);
    }
}

} // namespace Onyx
