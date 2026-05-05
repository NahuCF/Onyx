#pragma once

#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>

namespace Shared {

	// ============================================
	// Minimal data structs for serialization
	//
	// Storage strategy (AzerothCore-inspired):
	// - TERRAIN data (heightmap, splatmap, holes) -> Files (chunk_X_Z.terrain)
	// - STATIC OBJECTS (props, buildings) -> Files (chunk_X_Z.objects)
	// - DYNAMIC ENTITIES (spawns, portals, triggers) -> Database
	//
	// The chunk coordinates are computed from position:
	//   chunkX = floor(position.x / CHUNK_SIZE)
	//   chunkZ = floor(position.z / CHUNK_SIZE)
	//
	// This allows the server to load only entities in active chunks
	// without storing redundant chunk references in each entity.
	// ============================================

	constexpr float WORLD_CHUNK_SIZE = 64.0f; // Must match TerrainChunk::CHUNK_SIZE

	inline int32_t WorldToChunkX(float worldX)
	{
		return static_cast<int32_t>(std::floor(worldX / WORLD_CHUNK_SIZE));
	}
	inline int32_t WorldToChunkZ(float worldZ)
	{
		return static_cast<int32_t>(std::floor(worldZ / WORLD_CHUNK_SIZE));
	}

	struct StaticObjectData
	{
		uint64_t guid = 0;
		uint32_t templateId = 0;
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
	};

	struct SpawnPointData
	{
		uint64_t guid = 0;
		uint32_t creatureTemplateId = 0;
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);
		float respawnTime = 60.0f;
		float wanderRadius = 0.0f;
	};

	struct TriggerVolumeData
	{
		uint64_t guid = 0;
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 halfExtents = glm::vec3(1.0f);
		uint32_t actionId = 0;
		uint32_t flags = 0;
	};

	struct LightData
	{
		uint64_t guid = 0;
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 color = glm::vec3(1.0f);
		float intensity = 1.0f;
		float range = 10.0f;
		uint8_t type = 0; // 0=Point, 1=Spot, 2=Directional
		float spotAngle = 45.0f;
		glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
	};

	struct PortalData
	{
		uint64_t guid = 0;
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);
		glm::vec3 halfExtents = glm::vec3(1.0f, 2.0f, 0.2f);
		uint32_t destinationMapId = 0;
		glm::vec3 destinationPosition = glm::vec3(0.0f);
	};

	struct ParticleEmitterData
	{
		uint64_t guid = 0;
		glm::vec3 position = glm::vec3(0.0f);
		uint32_t templateId = 0;
	};

	struct WaypointData
	{
		uint64_t guid = 0;
		glm::vec3 position = glm::vec3(0.0f);
		uint64_t nextWaypointGuid = 0; // For patrol paths
		float waitTime = 0.0f;
	};

} // namespace Shared
