#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace MMO {

	// .omdl — Onyx Model format
	// GPU-ready binary: vertices in Onyx::MeshVertex layout (28 B = pos float3 +
	// snorm16x2 oct-normal + half2 UV + snorm16x2 oct-tangent + snorm16x2 bitangent-sign),
	// indices are uint32_t (or uint16_t when OMDL_FLAG_U16_INDICES is set in the header).
	// Reader memory-maps the file and points the VBO at the vertex blob — no Assimp at runtime.

	constexpr uint32_t OMDL_MAGIC = 0x4F4D444C; // "OMDL"
	constexpr uint32_t OMDL_VERSION = 2;        // v2 = quantized 28 B vertex + flags
	constexpr uint32_t OMDL_VERTEX_BYTES = 28;  // sizeof(Onyx::MeshVertex)

	// flags bits
	constexpr uint32_t OMDL_FLAG_U16_INDICES = 1u << 0; // index blob is uint16_t when totalVertices < 65536

	struct OmdlHeader
	{
		uint32_t magic = OMDL_MAGIC;
		uint32_t version = OMDL_VERSION;
		uint32_t flags = 0;
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

	// Writer-side: owns blob bytes via std::vector. Used by WriteOmdl + the editor exporter.
	struct OmdlData
	{
		OmdlHeader header;
		std::vector<OmdlMeshInfo> meshes;
		std::vector<uint8_t> vertexBlob; // totalVertices * 28 bytes (MeshVertex)
		std::vector<uint8_t> indexBlob;  // totalIndices * (header.flags & OMDL_FLAG_U16_INDICES ? 2 : 4) bytes
	};

	// Reader-side: zero-copy view into a memory-mapped file. ReadOmdl returns this.
	// Pointers are valid as long as the OmdlMapped object lives — the file mapping
	// is unmapped when this destructs. Move-only.
	class OmdlMapping;

	struct OmdlMapped
	{
		OmdlHeader header;
		std::vector<OmdlMeshInfo> meshes;
		const void* vertexData = nullptr;
		size_t      vertexBytes = 0;
		const void* indexData = nullptr;
		size_t      indexBytes = 0;

		std::unique_ptr<OmdlMapping> mapping; // RAII: owns the file mapping

		OmdlMapped();
		~OmdlMapped();
		OmdlMapped(OmdlMapped&&) noexcept;
		OmdlMapped& operator=(OmdlMapped&&) noexcept;
		OmdlMapped(const OmdlMapped&) = delete;
		OmdlMapped& operator=(const OmdlMapped&) = delete;
	};

} // namespace MMO
