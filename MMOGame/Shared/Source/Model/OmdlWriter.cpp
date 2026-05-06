#include "OmdlWriter.h"
#include <fstream>
#include <iostream>

namespace MMO {

	static void WriteStringField(std::ofstream& f, const std::string& s)
	{
		uint16_t len = static_cast<uint16_t>(s.size());
		f.write(reinterpret_cast<const char*>(&len), sizeof(len));
		if (len > 0)
			f.write(s.data(), len);
	}

	bool WriteOmdl(const std::string& path, const OmdlData& data)
	{
		std::ofstream file(path, std::ios::binary);
		if (!file.is_open())
		{
			std::cerr << "[OmdlWriter] Failed to open: " << path << "\n";
			return false;
		}

		// Header — caller is responsible for setting magic/version/flags/counts/bounds.
		file.write(reinterpret_cast<const char*>(&data.header), sizeof(OmdlHeader));

		// Per-mesh info
		for (const auto& mesh : data.meshes)
		{
			file.write(reinterpret_cast<const char*>(&mesh.indexCount), sizeof(mesh.indexCount));
			file.write(reinterpret_cast<const char*>(&mesh.firstIndex), sizeof(mesh.firstIndex));
			file.write(reinterpret_cast<const char*>(&mesh.baseVertex), sizeof(mesh.baseVertex));
			file.write(reinterpret_cast<const char*>(&mesh.boundsMin), sizeof(mesh.boundsMin));
			file.write(reinterpret_cast<const char*>(&mesh.boundsMax), sizeof(mesh.boundsMax));
			WriteStringField(file, mesh.albedoPath);
			WriteStringField(file, mesh.normalPath);
		}

		// Vertex blob (already in v2 28-byte layout)
		if (!data.vertexBlob.empty())
		{
			file.write(reinterpret_cast<const char*>(data.vertexBlob.data()), data.vertexBlob.size());
		}

		// Index blob (u16 or u32 — controlled by header.flags)
		if (!data.indexBlob.empty())
		{
			file.write(reinterpret_cast<const char*>(data.indexBlob.data()), data.indexBlob.size());
		}

		// Attachments table (v3+). Per entry: u8 id; if Custom, u16 nameLen+chars;
		// i32 parentBoneIndex; float[3] translation; float[4] rotation (xyzw).
		const uint32_t attachmentCount = static_cast<uint32_t>(data.attachments.size());
		file.write(reinterpret_cast<const char*>(&attachmentCount), sizeof(attachmentCount));

		for (const auto& a : data.attachments)
		{
			const uint8_t idByte = static_cast<uint8_t>(a.id);
			file.write(reinterpret_cast<const char*>(&idByte), sizeof(idByte));

			if (a.id == SocketId::Custom)
				WriteStringField(file, a.name);

			file.write(reinterpret_cast<const char*>(&a.parentBoneIndex), sizeof(a.parentBoneIndex));
			file.write(reinterpret_cast<const char*>(a.localTranslation), sizeof(a.localTranslation));
			file.write(reinterpret_cast<const char*>(a.localRotation), sizeof(a.localRotation));
		}

		return file.good();
	}

} // namespace MMO
