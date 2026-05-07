#include "WorldUIAnchorSystem.h"

#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

namespace Onyx {

	WorldUIAnchorSystem::WorldUIAnchorSystem() = default;

	void WorldUIAnchorSystem::BeginFrame(const glm::mat4& view, const glm::mat4& projection, glm::ivec2 viewport)
	{
		m_View = view;
		m_Projection = projection;
		m_ViewProj = projection * view;
		m_Viewport = viewport;
		m_CameraPos = glm::vec3(glm::inverse(view) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		m_Frustum.Update(m_ViewProj);
		++m_CameraTickId;
	}

	WorldUIAnchorSystem::Sample WorldUIAnchorSystem::Project(const AnchorRequest& req)
	{
		++m_Stats.requests;

		const bool cacheable = req.entityId != 0;
		CacheKey key = MakeKey(req);

		// Latched fast path: serve previously latched value immediately.
		if (cacheable && req.mode == AnchorMode::Latched)
		{
			auto it = m_Cache.find(key);
			if (it != m_Cache.end() && it->second.latched)
			{
				++m_Stats.cacheHits;
				return it->second.sample;
			}
		}

		// Track mode cache lookup: hit when both camera and pose are unchanged.
		if (cacheable && req.mode == AnchorMode::Track)
		{
			auto it = m_Cache.find(key);
			if (it != m_Cache.end() &&
				!it->second.latched &&
				it->second.cameraTickId == m_CameraTickId &&
				it->second.animTickId == req.animTickId)
			{
				++m_Stats.cacheHits;
				return it->second.sample;
			}
		}

		// Miss: compute world point, project, cache.
		glm::vec3 worldPos(0.0f);
		if (!ResolveSocketWorld(req, worldPos))
		{
			Sample miss{}; // visible=false
			if (cacheable)
			{
				CacheEntry& e = m_Cache[key];
				e.sample = miss;
				e.cameraTickId = m_CameraTickId;
				e.animTickId = req.animTickId;
				e.latched = false;
			}
			return miss;
		}

		Sample sample = ProjectWorldPoint(worldPos);
		++m_Stats.projections;

		if (cacheable)
		{
			CacheEntry& e = m_Cache[key];
			e.sample = sample;
			e.cameraTickId = m_CameraTickId;
			e.animTickId = req.animTickId;
			e.latched = (req.mode == AnchorMode::Latched);
			if (e.latched)
				++m_Stats.latched;
		}

		return sample;
	}

	bool WorldUIAnchorSystem::ResolveSocketWorld(const AnchorRequest& req, glm::vec3& outWorld) const
	{
		if (!req.modelMatrix)
			return false;

		// No socket info: anchor at entity origin (the model-matrix translation).
		if (!req.attachments)
		{
			outWorld = glm::vec3((*req.modelMatrix)[3]);
			return true;
		}

		glm::mat4 socketWorld(1.0f);
		bool ok = false;

		if (req.socketSlot != 0xFF)
		{
			ok = req.attachments->ResolveWorldByWellKnownSlot(
				req.socketSlot, *req.modelMatrix, req.meshSpaceBones, socketWorld);
		}
		else if (req.customSocketName)
		{
			ok = req.attachments->ResolveWorld(
				req.customSocketName, *req.modelMatrix, req.meshSpaceBones, socketWorld);
		}

		if (!ok)
		{
			// Fallback: entity origin. The widget-level fallback ladder
			// (mounted → primary → AABB) lives in the widget; this is only
			// the last resort if the widget gave up too.
			outWorld = glm::vec3((*req.modelMatrix)[3]);
			return true;
		}

		outWorld = glm::vec3(socketWorld[3]);
		return true;
	}

	WorldUIAnchorSystem::Sample WorldUIAnchorSystem::ProjectWorldPoint(const glm::vec3& worldPos)
	{
		Sample s{};

		// Distance cull
		const float distSq = glm::dot(worldPos - m_CameraPos, worldPos - m_CameraPos);
		const float maxDistSq = m_CullDistance * m_CullDistance;
		if (distSq > maxDistSq)
		{
			++m_Stats.culledDistance;
			return s;
		}

		// Frustum cull
		if (!m_Frustum.IsPointVisible(worldPos))
		{
			++m_Stats.culledFrustum;
			return s;
		}

		// Project
		const glm::vec4 clip = m_ViewProj * glm::vec4(worldPos, 1.0f);
		if (clip.w <= 0.0001f)
		{
			++m_Stats.culledBehindCamera;
			return s; // behind camera or singular
		}

		const glm::vec3 ndc = glm::vec3(clip) / clip.w;
		if (ndc.z < -1.0f || ndc.z > 1.0f)
		{
			++m_Stats.culledFrustum;
			return s;
		}

		s.screenPos.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(m_Viewport.x);
		s.screenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(m_Viewport.y);
		s.depth = ndc.z * 0.5f + 0.5f;
		s.visible = true;
		return s;
	}

	void WorldUIAnchorSystem::ReleaseEntity(uint64_t entityId)
	{
		for (auto it = m_Cache.begin(); it != m_Cache.end();)
		{
			if (it->first.entityId == entityId)
				it = m_Cache.erase(it);
			else
				++it;
		}
	}

	void WorldUIAnchorSystem::ClearCache()
	{
		m_Cache.clear();
	}

	uint64_t WorldUIAnchorSystem::HashName(const char* name)
	{
		// FNV-1a 64-bit
		uint64_t h = 0xCBF29CE484222325ULL;
		if (!name)
			return h;
		while (*name)
		{
			h ^= static_cast<unsigned char>(*name++);
			h *= 0x100000001B3ULL;
		}
		// Avoid collision with well-known slot space (slots use raw 0..63).
		// Set top bit so name-hashes never alias slot integers.
		return h | 0x8000000000000000ULL;
	}

	WorldUIAnchorSystem::CacheKey WorldUIAnchorSystem::MakeKey(const AnchorRequest& req)
	{
		CacheKey k{};
		k.entityId = req.entityId;
		if (req.socketSlot != 0xFF)
			k.socketHash = static_cast<uint64_t>(req.socketSlot);
		else
			k.socketHash = HashName(req.customSocketName);
		return k;
	}

} // namespace Onyx
