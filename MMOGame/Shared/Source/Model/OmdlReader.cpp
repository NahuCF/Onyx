#include "OmdlReader.h"
#include <fstream>
#include <iostream>

namespace MMO {

	static std::string ReadStringField(std::ifstream& f)
	{
		uint16_t len = 0;
		f.read(reinterpret_cast<char*>(&len), sizeof(len));
		if (len == 0 || !f.good())
			return {};
		std::string s(len, '\0');
		f.read(s.data(), len);
		return s;
	}

	bool ReadOmdl(const std::string& path, OmdlData& out)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
		{
			std::cerr << "[OmdlReader] Failed to open: " << path << "\n";
			return false;
		}

		// Header
		file.read(reinterpret_cast<char*>(&out.header), sizeof(OmdlHeader));
		if (out.header.magic != OMDL_MAGIC)
		{
			std::cerr << "[OmdlReader] Bad magic in: " << path << "\n";
			return false;
		}
		if (out.header.version != OMDL_VERSION)
		{
			std::cerr << "[OmdlReader] Unsupported version " << out.header.version
					  << " in: " << path << "\n";
			return false;
		}

		// Per-mesh info
		out.meshes.resize(out.header.meshCount);
		for (uint32_t i = 0; i < out.header.meshCount; i++)
		{
			auto& mesh = out.meshes[i];
			file.read(reinterpret_cast<char*>(&mesh.indexCount), sizeof(mesh.indexCount));
			file.read(reinterpret_cast<char*>(&mesh.firstIndex), sizeof(mesh.firstIndex));
			file.read(reinterpret_cast<char*>(&mesh.baseVertex), sizeof(mesh.baseVertex));
			file.read(reinterpret_cast<char*>(&mesh.boundsMin), sizeof(mesh.boundsMin));
			file.read(reinterpret_cast<char*>(&mesh.boundsMax), sizeof(mesh.boundsMax));
			mesh.albedoPath = ReadStringField(file);
			mesh.normalPath = ReadStringField(file);
		}

		// Vertex blob (totalVertices * 56 bytes)
		size_t vertexBytes = static_cast<size_t>(out.header.totalVertices) * 56;
		out.vertexBlob.resize(vertexBytes);
		if (vertexBytes > 0)
		{
			file.read(reinterpret_cast<char*>(out.vertexBlob.data()), vertexBytes);
		}

		// Index blob (totalIndices * 4 bytes)
		size_t indexBytes = static_cast<size_t>(out.header.totalIndices) * 4;
		out.indexBlob.resize(indexBytes);
		if (indexBytes > 0)
		{
			file.read(reinterpret_cast<char*>(out.indexBlob.data()), indexBytes);
		}

		return file.good();
	}

} // namespace MMO
