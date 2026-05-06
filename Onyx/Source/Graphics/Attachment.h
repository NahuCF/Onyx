#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Onyx {

	// Named attachment point on a model: an Empty/Locator authored as
	// "socket_<Name>", parented to a bone (or model root for static props).
	// Engine stores name + parent + offset; semantics live in the game layer.
	struct Attachment
	{
		std::string name;					 // identifier (e.g. "OverheadName"); always populated
		int parentBoneIndex = -1;			 // -1 = model-space (no skeleton parent)
		glm::vec3 localTranslation{0.0f};	 // relative to parent bone in bind pose
		glm::quat localRotation{1, 0, 0, 0}; // identity = no rotation
	};

	// Container of attachments. Two lookup paths: by string name, and by an
	// opaque "well-known slot" populated by a higher layer (e.g. MMO::SocketId).
	class AttachmentSet
	{
	public:
		static constexpr size_t WELL_KNOWN_SLOTS = 64;

		AttachmentSet() { m_BySocketSlot.fill(-1); }

		void Reserve(size_t n);
		void Add(Attachment a);
		void Clear();

		size_t Count() const { return m_Attachments.size(); }
		bool Empty() const { return m_Attachments.empty(); }

		const std::vector<Attachment>& All() const { return m_Attachments; }
		const Attachment* FindByName(std::string_view name) const;

		// Map a well-known slot (e.g. SocketId::MuzzleR) to its attachment index.
		void AssignWellKnownSlot(uint8_t slot, size_t attachmentIndex);
		const Attachment* FindByWellKnownSlot(uint8_t slot) const;

		// boneAnimGlobals = mesh-space animated bone transforms (globalInverse *
		// globalAnim), NOT skinning matrices. Pass nullptr only for static
		// sockets. Returns false on unknown name or missing pose for an
		// animated socket.
		bool ResolveWorld(std::string_view name,
						  const glm::mat4& modelMatrix,
						  const std::vector<glm::mat4>* boneAnimGlobals,
						  glm::mat4& outWorld) const;

		bool ResolveWorldByWellKnownSlot(uint8_t slot,
										 const glm::mat4& modelMatrix,
										 const std::vector<glm::mat4>* boneAnimGlobals,
										 glm::mat4& outWorld) const;

	private:
		glm::mat4 ResolveAttachment(const Attachment& a,
									const glm::mat4& modelMatrix,
									const std::vector<glm::mat4>* boneAnimGlobals) const;

		std::vector<Attachment> m_Attachments;
		std::unordered_map<std::string, int32_t> m_ByName; // name -> index
		std::array<int16_t, WELL_KNOWN_SLOTS> m_BySocketSlot{}; // slot -> index, -1 if unset
	};

} // namespace Onyx
