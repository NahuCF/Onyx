#pragma once

#include "Attachment.h"
#include "Frustum.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace Onyx {

	// Engine subsystem that projects entity-bound world points to screen space.
	// Owned by Application; used by the UI library, FX, 3D audio, equipment
	// scene-graph — anywhere a "where is socket X on entity Y in screen space?"
	// query is needed.
	//
	// Usage per frame (caller order):
	//   sys.BeginFrame(view, projection, viewport);
	//   for each entity & widget:
	//     auto sample = sys.Project(request);
	//
	// Cache key: (entityId, socketSlot/name, cameraTickId, animTickId). When
	// camera and animator pose haven't changed, the projection is reused.
	// Static entities (no animator) report animTickId=0 forever, so their
	// projections cache across the entity's lifetime.
	class WorldUIAnchorSystem
	{
	public:
		enum class AnchorMode : uint8_t
		{
			Track,	 // re-resolve every frame; cache hits when state is unchanged
			Latched, // resolve once at first request, then cached until ReleaseEntity
		};

		struct Sample
		{
			glm::vec2 screenPos{0.0f}; // pixels, top-left origin (Y down)
			float depth = 0.0f;		   // 0..1, 0 = near
			bool visible = false;	   // in frustum + on-screen + not behind camera + within cull distance
		};

		struct AnchorRequest
		{
			uint64_t entityId = 0;					// stable id; 0 reserved for "anonymous, never cache"
			uint8_t socketSlot = 0xFF;				// MMO::SocketId well-known slot, 0xFF = Custom/no slot
			const char* customSocketName = nullptr; // used when socketSlot == 0xFF and attachments != nullptr
			const glm::mat4* modelMatrix = nullptr; // required: entity world transform
			const std::vector<glm::mat4>* meshSpaceBones = nullptr; // null for static entities
			const AttachmentSet* attachments = nullptr;				// null = use modelMatrix only (anchor at entity origin)
			uint32_t animTickId = 0;								// from Animator::GetTickId(); 0 for static
			AnchorMode mode = AnchorMode::Track;
		};

		struct Stats
		{
			uint32_t requests = 0;
			uint32_t cacheHits = 0;
			uint32_t projections = 0;
			uint32_t culledFrustum = 0;
			uint32_t culledDistance = 0;
			uint32_t culledBehindCamera = 0;
			uint32_t latched = 0;
		};

		WorldUIAnchorSystem();

		void BeginFrame(const glm::mat4& view, const glm::mat4& projection, glm::ivec2 viewport);

		Sample Project(const AnchorRequest& req);

		// Drop cached projections for an entity (e.g. on despawn). Latched
		// anchors for this entity also clear; next Project re-latches.
		void ReleaseEntity(uint64_t entityId);

		// Drop everything (e.g. zone change).
		void ClearCache();

		void SetCullDistance(float worldUnits) { m_CullDistance = worldUnits; }
		float GetCullDistance() const { return m_CullDistance; }

		uint32_t GetCameraTickId() const { return m_CameraTickId; }

		const Stats& GetStats() const { return m_Stats; }
		void ResetStats() { m_Stats = {}; }

	private:
		struct CacheKey
		{
			uint64_t entityId;
			uint64_t socketHash; // socketSlot if !=0xFF, else hash(customName)

			bool operator==(const CacheKey& o) const noexcept
			{
				return entityId == o.entityId && socketHash == o.socketHash;
			}
		};
		struct CacheKeyHash
		{
			size_t operator()(const CacheKey& k) const noexcept
			{
				// 64-bit splitmix mix of the two halves
				uint64_t x = k.entityId * 0x9E3779B97F4A7C15ULL ^ k.socketHash;
				x ^= x >> 30;
				x *= 0xBF58476D1CE4E5B9ULL;
				x ^= x >> 27;
				x *= 0x94D049BB133111EBULL;
				x ^= x >> 31;
				return static_cast<size_t>(x);
			}
		};

		struct CacheEntry
		{
			Sample sample;
			uint32_t cameraTickId = 0;
			uint32_t animTickId = 0;
			bool latched = false;
		};

		Sample ProjectWorldPoint(const glm::vec3& worldPos);
		bool ResolveSocketWorld(const AnchorRequest& req, glm::vec3& outWorld) const;

		static uint64_t HashName(const char* name);
		static CacheKey MakeKey(const AnchorRequest& req);

		glm::mat4 m_View{1.0f};
		glm::mat4 m_Projection{1.0f};
		glm::mat4 m_ViewProj{1.0f};
		glm::vec3 m_CameraPos{0.0f};
		glm::ivec2 m_Viewport{0, 0};
		Frustum m_Frustum;

		uint32_t m_CameraTickId = 0;
		float m_CullDistance = 200.0f;

		std::unordered_map<CacheKey, CacheEntry, CacheKeyHash> m_Cache;
		Stats m_Stats;
	};

} // namespace Onyx
