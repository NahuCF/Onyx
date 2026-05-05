#pragma once

#include "Terrain/TerrainChunk.h"
#include <Terrain/ChunkFormat.h>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Editor3D {

	// ---- Light data ----
	enum class LightType : uint8_t
	{
		Point = 0,
		Spot = 1
	};

	struct EditorLight
	{
		LightType type = LightType::Point;
		glm::vec3 position = {0, 0, 0};
		glm::vec3 direction = {0, -1, 0};
		glm::vec3 color = {1, 1, 1};
		float intensity = 1.0f;
		float range = 10.0f;
		float innerAngle = 30.0f;
		float outerAngle = 45.0f;
		bool castShadows = false;
	};

	constexpr int MAX_POINT_LIGHTS = 32;
	constexpr int MAX_SPOT_LIGHTS = 8;

	// ---- Static object data (serialized in OBJS section v2) ----
	struct ChunkObject
	{
		// WorldObject base
		uint64_t guid = 0;
		std::string name;
		glm::vec3 position = {0, 0, 0};
		glm::quat rotation = glm::quat(1, 0, 0, 0); // identity
		float scale = 1.0f;
		uint64_t parentGuid = 0;

		// StaticObject
		std::string modelPath;
		std::string materialId;

		// Collider
		uint8_t colliderType = 0; // NONE=0, BOX=1, SPHERE=2, CAPSULE=3, MESH=4
		glm::vec3 colliderCenter = {0, 0, 0};
		glm::vec3 colliderHalfExtents = {1, 1, 1};
		float colliderRadius = 1.0f;
		float colliderHeight = 2.0f;

		// Rendering flags
		bool castsShadow = true;
		bool receivesLightmap = true;
		uint32_t lightmapIndex = 0;
		glm::vec4 lightmapScaleOffset = {1, 1, 0, 0};

		// Per-mesh materials
		struct MeshMaterialEntry
		{
			std::string meshName;
			std::string materialId;
			glm::vec3 positionOffset = {0, 0, 0};
			glm::vec3 rotationOffset = {0, 0, 0}; // euler degrees
			float scaleMultiplier = 1.0f;
			bool visible = true;
		};
		std::vector<MeshMaterialEntry> meshMaterials;

		// Animations
		std::vector<std::string> animationPaths;
		std::string currentAnimation;
		bool animLoop = true;
		float animSpeed = 1.0f;
	};

	// ---- Sound emitter data ----
	struct SoundEmitter
	{
		std::string soundPath;
		glm::vec3 position = {0, 0, 0};
		float volume = 1.0f;
		float minRange = 5.0f;
		float maxRange = 20.0f;
		bool loop = true;
	};

	// ---- WorldChunk: the spatial container (64x64m) ----
	class WorldChunk
	{
	public:
		WorldChunk(int32_t chunkX, int32_t chunkZ);
		~WorldChunk();

		int32_t GetChunkX() const { return m_ChunkX; }
		int32_t GetChunkZ() const { return m_ChunkZ; }

		// Terrain access
		TerrainChunk* GetTerrain() { return m_Terrain.get(); }
		const TerrainChunk* GetTerrain() const { return m_Terrain.get(); }

		// Non-terrain data
		std::vector<EditorLight>& GetLights()
		{
			m_Modified = true;
			return m_Lights;
		}
		const std::vector<EditorLight>& GetLights() const { return m_Lights; }
		std::vector<ChunkObject>& GetObjects()
		{
			m_Modified = true;
			return m_Objects;
		}
		const std::vector<ChunkObject>& GetObjects() const { return m_Objects; }
		std::vector<SoundEmitter>& GetSounds()
		{
			m_Modified = true;
			return m_Sounds;
		}
		const std::vector<SoundEmitter>& GetSounds() const { return m_Sounds; }

		// File I/O (.chunk section format)
		void Load(const std::string& filePath);
		void Save(const std::string& filePath);
		void Unload();

		// State (delegates terrain state + tracks non-terrain modifications)
		ChunkState GetState() const { return m_Terrain ? m_Terrain->GetState() : ChunkState::Unloaded; }
		bool IsReady() const { return m_Terrain && m_Terrain->IsReady(); }
		bool IsModified() const { return m_Modified || (m_Terrain && m_Terrain->IsModified()); }
		void ClearModified()
		{
			m_Modified = false;
			if (m_Terrain)
				m_Terrain->ClearModified();
		}

	private:
		int32_t m_ChunkX, m_ChunkZ;
		bool m_Modified = false;
		uint32_t m_LoadedVersion = 0; // File version during Load()

		std::unique_ptr<TerrainChunk> m_Terrain;
		std::vector<EditorLight> m_Lights;
		std::vector<ChunkObject> m_Objects;
		std::vector<SoundEmitter> m_Sounds;

		// Section readers/writers
		void LoadTerrainSection(std::ifstream& file);
		void LoadLightsSection(std::ifstream& file);
		void LoadObjectsSection(std::ifstream& file);
		void LoadSoundsSection(std::ifstream& file);

		void SaveTerrainSection(std::ofstream& file);
		void SaveLightsSection(std::ofstream& file);
		void SaveObjectsSection(std::ofstream& file);
		void SaveSoundsSection(std::ofstream& file);
	};

} // namespace Editor3D
