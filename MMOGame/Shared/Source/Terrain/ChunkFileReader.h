#pragma once

#include "ChunkFormat.h"
#include "TerrainData.h"
#include <fstream>
#include <string>
#include <vector>

namespace MMO {

	// Complete data from a single .chunk file
	struct ChunkFileData
	{
		uint32_t mapId = 0;
		TerrainChunkData terrain;
		std::vector<ChunkLightData> lights;
		std::vector<ChunkObjectData> objects;
	};

	// Read a .chunk file (CHNK container format)
	// Returns true on success
	bool LoadChunkFile(const std::string& path, ChunkFileData& out);

	// Read just the terrain section from an already-open .chunk stream
	void ReadTerrainSection(std::ifstream& file, TerrainChunkData& data,
							int32_t chunkX, int32_t chunkZ);

} // namespace MMO
