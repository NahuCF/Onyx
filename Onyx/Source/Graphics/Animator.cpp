#include "Animator.h"
#include "pch.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Onyx {

	Animator::Animator(AnimatedModel* model) : m_Model(model)
	{
		if (m_Model)
		{
			m_FinalBoneMatrices.resize(Skeleton::MAX_BONES, glm::mat4(1.0f));
			BuildNodeCache();
		}
	}

	void Animator::SetModel(AnimatedModel* model)
	{
		m_Model = model;
		m_CurrentAnimation = nullptr;
		m_BlendFromAnimation = nullptr;
		m_CurrentTime = 0.0f;
		m_Playing = false;
		m_IsBlending = false;
		m_NodeCacheValid = false;

		if (m_Model)
		{
			m_FinalBoneMatrices.resize(Skeleton::MAX_BONES, glm::mat4(1.0f));
			BuildNodeCache();
		}
	}

	void Animator::BuildNodeCache()
	{
		if (!m_Model)
			return;

		m_NodeCache.clear();
		const auto& nodeHierarchy = m_Model->GetNodeHierarchy();

		for (size_t i = 0; i < nodeHierarchy.size(); i++)
		{
			const auto& node = nodeHierarchy[i];
			NodeTransformCache cacheNode;
			cacheNode.name = node.name;
			cacheNode.localTransform = node.transform;
			cacheNode.parentIndex = node.parentIndex;
			cacheNode.children = node.children;
			m_NodeCache.push_back(cacheNode);
		}

		m_NodeCacheValid = true;
	}

	void Animator::Play(const std::string& animationName, bool loop)
	{
		if (!m_Model)
			return;

		Animation* anim = m_Model->GetAnimation(animationName);
		if (anim)
		{
			m_CurrentAnimation = anim;
			m_CurrentTime = 0.0f;
			m_Playing = true;
			m_Paused = false;
			m_Loop = loop;
			m_IsBlending = false;
		}
	}

	void Animator::Play(int animationIndex, bool loop)
	{
		if (!m_Model)
			return;

		Animation* anim = m_Model->GetAnimation(animationIndex);
		if (anim)
		{
			m_CurrentAnimation = anim;
			m_CurrentTime = 0.0f;
			m_Playing = true;
			m_Paused = false;
			m_Loop = loop;
			m_IsBlending = false;
		}
	}

	void Animator::Stop()
	{
		m_Playing = false;
		m_Paused = false;
		m_CurrentTime = 0.0f;
		m_IsBlending = false;
	}

	void Animator::Pause()
	{
		if (m_Playing)
		{
			m_Paused = true;
		}
	}

	void Animator::Resume()
	{
		if (m_Paused)
		{
			m_Paused = false;
		}
	}

	void Animator::BlendTo(const std::string& animationName, float blendDuration, bool loop)
	{
		if (!m_Model)
			return;

		Animation* anim = m_Model->GetAnimation(animationName);
		if (anim && anim != m_CurrentAnimation)
		{
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

	void Animator::BlendTo(int animationIndex, float blendDuration, bool loop)
	{
		if (!m_Model)
			return;

		Animation* anim = m_Model->GetAnimation(animationIndex);
		if (anim)
		{
			BlendTo(anim->GetName(), blendDuration, loop);
		}
	}

	void Animator::SetTime(float time)
	{
		m_CurrentTime = time;
		if (m_CurrentAnimation)
		{
			float duration = m_CurrentAnimation->GetDuration();
			if (m_CurrentTime > duration)
			{
				m_CurrentTime = m_Loop ? fmod(m_CurrentTime, duration) : duration;
			}
		}
	}

	float Animator::GetNormalizedTime() const
	{
		if (!m_CurrentAnimation)
			return 0.0f;
		float duration = m_CurrentAnimation->GetDuration();
		return duration > 0.0f ? m_CurrentTime / duration : 0.0f;
	}

	void Animator::Update(float deltaTime)
	{
		if (!m_Model || !m_CurrentAnimation)
		{
			return;
		}
		if (!m_Playing || m_Paused)
		{
			return;
		}
		if (m_NodeCache.empty() || m_FinalBoneMatrices.empty())
		{
			return;
		}

		m_CurrentTime += deltaTime * m_PlaybackSpeed * m_CurrentAnimation->GetTicksPerSecond();

		float duration = m_CurrentAnimation->GetDuration();
		if (m_CurrentTime > duration)
		{
			if (m_Loop)
			{
				m_CurrentTime = fmod(m_CurrentTime, duration);
			}
			else
			{
				m_CurrentTime = duration;
				m_Playing = false;
			}
		}

		if (m_IsBlending)
		{
			m_BlendFactor += deltaTime / m_BlendDuration;
			if (m_BlendFactor >= 1.0f)
			{
				m_BlendFactor = 1.0f;
				m_IsBlending = false;
				m_BlendFromAnimation = nullptr;
			}
		}

		glm::mat4 identity(1.0f);
		for (int i = 0; i < static_cast<int>(m_NodeCache.size()); i++)
		{
			if (m_NodeCache[i].parentIndex < 0)
			{
				CalculateBoneTransform(i, identity);
			}
		}

		m_Model->SetBoneMatrices(m_FinalBoneMatrices);
	}

	void Animator::CalculateBoneTransform(int nodeIndex, const glm::mat4& parentTransform)
	{
		if (nodeIndex < 0 || nodeIndex >= static_cast<int>(m_NodeCache.size()))
		{
			return;
		}

		const auto& node = m_NodeCache[nodeIndex];
		glm::mat4 nodeTransform = node.localTransform;

		if (m_CurrentAnimation)
		{
			const BoneAnimation* boneAnim = m_CurrentAnimation->GetBoneAnimation(node.name);

			if (boneAnim)
			{
				glm::vec3 position = boneAnim->InterpolatePosition(m_CurrentTime);
				glm::quat rotation = boneAnim->InterpolateRotation(m_CurrentTime);
				glm::vec3 scale = boneAnim->InterpolateScale(m_CurrentTime);

				if (m_IsBlending && m_BlendFromAnimation)
				{
					const BoneAnimation* fromBoneAnim = m_BlendFromAnimation->GetBoneAnimation(node.name);
					if (fromBoneAnim)
					{
						glm::vec3 fromPos = fromBoneAnim->InterpolatePosition(m_BlendFromTime);
						glm::quat fromRot = fromBoneAnim->InterpolateRotation(m_BlendFromTime);
						glm::vec3 fromScale = fromBoneAnim->InterpolateScale(m_BlendFromTime);

						position = glm::mix(fromPos, position, m_BlendFactor);
						rotation = glm::slerp(fromRot, rotation, m_BlendFactor);
						scale = glm::mix(fromScale, scale, m_BlendFactor);
					}
				}

				nodeTransform = glm::translate(glm::mat4(1.0f), position) *
								glm::mat4_cast(rotation) *
								glm::scale(glm::mat4(1.0f), scale);
			}
		}

		glm::mat4 globalTransform = parentTransform * nodeTransform;

		const Skeleton& skeleton = m_Model->GetSkeleton();
		int boneIndex = skeleton.GetBoneIndex(node.name);
		if (boneIndex >= 0 && boneIndex < static_cast<int>(m_FinalBoneMatrices.size()))
		{
			const Bone* bone = skeleton.GetBone(boneIndex);
			if (bone)
			{
				m_FinalBoneMatrices[boneIndex] = skeleton.GetGlobalInverseTransform() * globalTransform * bone->offsetMatrix;
			}
		}

		for (int childIndex : node.children)
		{
			CalculateBoneTransform(childIndex, globalTransform);
		}
	}

} // namespace Onyx
