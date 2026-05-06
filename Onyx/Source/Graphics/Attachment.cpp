#include "Attachment.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace Onyx {

	void AttachmentSet::Reserve(size_t n)
	{
		m_Attachments.reserve(n);
		m_ByName.reserve(n);
	}

	void AttachmentSet::Add(Attachment a)
	{
		if (m_ByName.find(a.name) != m_ByName.end())
		{
			std::cerr << "[Onyx] Duplicate socket name '" << a.name
					  << "' — keeping first; later duplicate ignored.\n";
			return;
		}
		const int32_t index = static_cast<int32_t>(m_Attachments.size());
		m_ByName[a.name] = index;
		m_Attachments.push_back(std::move(a));
	}

	void AttachmentSet::Clear()
	{
		m_Attachments.clear();
		m_ByName.clear();
		m_BySocketSlot.fill(-1);
	}

	const Attachment* AttachmentSet::FindByName(std::string_view name) const
	{
		auto it = m_ByName.find(std::string(name));
		if (it == m_ByName.end())
			return nullptr;
		return &m_Attachments[static_cast<size_t>(it->second)];
	}

	void AttachmentSet::AssignWellKnownSlot(uint8_t slot, size_t attachmentIndex)
	{
		if (slot >= WELL_KNOWN_SLOTS)
			return;
		if (attachmentIndex >= m_Attachments.size())
			return;
		m_BySocketSlot[slot] = static_cast<int16_t>(attachmentIndex);
	}

	const Attachment* AttachmentSet::FindByWellKnownSlot(uint8_t slot) const
	{
		if (slot >= WELL_KNOWN_SLOTS)
			return nullptr;
		const int16_t idx = m_BySocketSlot[slot];
		if (idx < 0)
			return nullptr;
		return &m_Attachments[static_cast<size_t>(idx)];
	}

	glm::mat4 AttachmentSet::ResolveAttachment(const Attachment& a,
											   const glm::mat4& modelMatrix,
											   const std::vector<glm::mat4>* boneAnimGlobals) const
	{
		const glm::mat4 local = glm::translate(glm::mat4(1.0f), a.localTranslation) *
								glm::mat4_cast(a.localRotation);

		if (a.parentBoneIndex < 0 || boneAnimGlobals == nullptr)
			return modelMatrix * local;

		const size_t idx = static_cast<size_t>(a.parentBoneIndex);
		if (idx >= boneAnimGlobals->size())
			return modelMatrix * local;

		return modelMatrix * (*boneAnimGlobals)[idx] * local;
	}

	bool AttachmentSet::ResolveWorld(std::string_view name,
									 const glm::mat4& modelMatrix,
									 const std::vector<glm::mat4>* boneAnimGlobals,
									 glm::mat4& outWorld) const
	{
		const Attachment* a = FindByName(name);
		if (!a)
			return false;
		if (a->parentBoneIndex >= 0 && !boneAnimGlobals)
			return false; // animated socket needs a pose
		outWorld = ResolveAttachment(*a, modelMatrix, boneAnimGlobals);
		return true;
	}

	bool AttachmentSet::ResolveWorldByWellKnownSlot(uint8_t slot,
													const glm::mat4& modelMatrix,
													const std::vector<glm::mat4>* boneAnimGlobals,
													glm::mat4& outWorld) const
	{
		const Attachment* a = FindByWellKnownSlot(slot);
		if (!a)
			return false;
		if (a->parentBoneIndex >= 0 && !boneAnimGlobals)
			return false; // animated socket needs a pose
		outWorld = ResolveAttachment(*a, modelMatrix, boneAnimGlobals);
		return true;
	}

} // namespace Onyx
