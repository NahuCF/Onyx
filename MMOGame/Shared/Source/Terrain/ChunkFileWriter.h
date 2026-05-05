#pragma once

#include "ChunkFileReader.h"
#include <string>

namespace MMO {

	// Write a runtime .chunk file (CHNK container format) from ChunkFileData.
	// Returns true on success.
	bool WriteChunkFile(const std::string& path, const ChunkFileData& data,
						int32_t chunkX, int32_t chunkZ);

} // namespace MMO
