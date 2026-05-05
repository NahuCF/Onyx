#include "ChunkFileReader.h"
#include "ChunkIO.h"
#include <fstream>
#include <iostream>

namespace MMO {

	void ReadTerrainSection(std::ifstream& file, TerrainChunkData& data, int32_t chunkX, int32_t chunkZ)
	{
		data.chunkX = chunkX;
		data.chunkZ = chunkZ;

		data.heightmap.resize(TERRAIN_CHUNK_HEIGHTMAP_SIZE);
		file.read(reinterpret_cast<char*>(data.heightmap.data()),
				  TERRAIN_CHUNK_HEIGHTMAP_SIZE * sizeof(float));

		data.splatmap.resize(TERRAIN_SPLATMAP_TEXELS * TERRAIN_MAX_LAYERS);
		file.read(reinterpret_cast<char*>(data.splatmap.data()),
				  TERRAIN_SPLATMAP_TEXELS * TERRAIN_MAX_LAYERS);

		file.read(reinterpret_cast<char*>(&data.holeMask), sizeof(data.holeMask));
		file.read(reinterpret_cast<char*>(&data.minHeight), sizeof(data.minHeight));
		file.read(reinterpret_cast<char*>(&data.maxHeight), sizeof(data.maxHeight));

		for (int i = 0; i < TERRAIN_MAX_LAYERS; i++)
		{
			data.materialIds[i] = ReadString(file);
		}
	}

	static void ReadLightsSection(std::ifstream& file, std::vector<ChunkLightData>& lights)
	{
		uint32_t count = 0;
		file.read(reinterpret_cast<char*>(&count), sizeof(count));
		if (count > MAX_LIGHTS_PER_CHUNK)
			return;

		lights.resize(count);
		for (uint32_t i = 0; i < count; i++)
		{
			auto& light = lights[i];
			file.read(reinterpret_cast<char*>(&light.type), sizeof(light.type));
			file.read(reinterpret_cast<char*>(&light.position), sizeof(light.position));
			file.read(reinterpret_cast<char*>(&light.direction), sizeof(light.direction));
			file.read(reinterpret_cast<char*>(&light.color), sizeof(light.color));
			file.read(reinterpret_cast<char*>(&light.intensity), sizeof(light.intensity));
			file.read(reinterpret_cast<char*>(&light.range), sizeof(light.range));
			file.read(reinterpret_cast<char*>(&light.innerAngle), sizeof(light.innerAngle));
			file.read(reinterpret_cast<char*>(&light.outerAngle), sizeof(light.outerAngle));
			file.read(reinterpret_cast<char*>(&light.castShadows), sizeof(light.castShadows));
		}
	}

	static void ReadObjectsSection(std::ifstream& file, std::vector<ChunkObjectData>& objects,
								   uint32_t containerVersion)
	{
		uint32_t count = 0;
		file.read(reinterpret_cast<char*>(&count), sizeof(count));
		if (count > MAX_OBJECTS_PER_CHUNK)
			return;

		objects.resize(count);
		for (uint32_t i = 0; i < count; i++)
		{
			auto& obj = objects[i];
			obj.modelPath = ReadString(file);

			file.read(reinterpret_cast<char*>(&obj.position), sizeof(obj.position));
			file.read(reinterpret_cast<char*>(&obj.rotation), sizeof(obj.rotation));
			file.read(reinterpret_cast<char*>(&obj.scale), sizeof(obj.scale));
			file.read(reinterpret_cast<char*>(&obj.flags), sizeof(obj.flags));
			if (containerVersion >= 3)
			{
				obj.materialId = ReadString(file);
			}
		}
	}

	bool LoadChunkFile(const std::string& path, ChunkFileData& out)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
			return false;

		uint32_t magic;
		file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
		if (magic != CHNK_MAGIC)
		{
			return false;
		}

		uint32_t version;
		file.read(reinterpret_cast<char*>(&version), sizeof(version));
		if (version < 2 || version > CHUNK_FORMAT_VERSION)
		{
			std::cerr << "ChunkFileReader: unsupported CHNK version " << version
					  << " (supported 2-" << CHUNK_FORMAT_VERSION << ") in " << path << "\n";
			return false;
		}

		file.read(reinterpret_cast<char*>(&out.mapId), sizeof(out.mapId));

		int32_t chunkX, chunkZ;
		file.read(reinterpret_cast<char*>(&chunkX), sizeof(chunkX));
		file.read(reinterpret_cast<char*>(&chunkZ), sizeof(chunkZ));

		uint32_t sectionCount;
		file.read(reinterpret_cast<char*>(&sectionCount), sizeof(sectionCount));

		for (uint32_t s = 0; s < sectionCount && file.good(); s++)
		{
			uint32_t sectionType;
			file.read(reinterpret_cast<char*>(&sectionType), sizeof(sectionType));

			uint32_t sectionSize;
			file.read(reinterpret_cast<char*>(&sectionSize), sizeof(sectionSize));

			auto sectionStart = file.tellg();

			switch (sectionType)
			{
			case TERR_TAG:
				ReadTerrainSection(file, out.terrain, chunkX, chunkZ);
				break;
			case LGHT_TAG:
				ReadLightsSection(file, out.lights);
				break;
			case OBJS_TAG:
				ReadObjectsSection(file, out.objects, version);
				break;
			case SNDS_TAG:
				break; // Skip sounds for now
			default:
				break;
			}

			file.seekg(sectionStart + static_cast<std::streamoff>(sectionSize));
		}

		return true;
	}

} // namespace MMO
