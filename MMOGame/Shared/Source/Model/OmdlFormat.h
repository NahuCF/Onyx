#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace MMO {

	// .omdl — Onyx Model format
	// GPU-ready binary: vertices in MeshVertex layout (56 bytes), indices as uint32_t.
	// Client does fread() -> glBufferData() with no Assimp dependency.

	constexpr uint32_t OMDL_MAGIC = 0x4F4D444C; // "OMDL"
	constexpr uint32_t OMDL_VERSION = 1;

	struct OmdlHeader
	{
		uint32_t magic = OMDL_MAGIC;
		uint32_t version = OMDL_VERSION;
		uint32_t meshCount = 0;
		uint32_t totalVertices = 0;
		uint32_t totalIndices = 0;
		float boundsMin[3] = {0, 0, 0};
		float boundsMax[3] = {0, 0, 0};
	};

	struct OmdlMeshInfo
	{
		uint32_t indexCount = 0;
		uint32_t firstIndex = 0;
		int32_t baseVertex = 0;
		float boundsMin[3] = {0, 0, 0};
		float boundsMax[3] = {0, 0, 0};
		std::string albedoPath;
		std::string normalPath;
	};

	struct OmdlData
	{
		OmdlHeader header;
		std::vector<OmdlMeshInfo> meshes;
		std::vector<uint8_t> vertexBlob; // totalVertices * 56 bytes (MeshVertex)
		std::vector<uint8_t> indexBlob;	 // totalIndices * 4 bytes (uint32_t)
	};

} // namespace MMO
