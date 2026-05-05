#include "OmdlReader.h"

#include <cstring>
#include <iostream>

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
#else
	#include <fcntl.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

namespace MMO {

	// Owns a read-only file mapping. Destructor unmaps and closes.
	class OmdlMapping
	{
	public:
		const uint8_t* base = nullptr;
		size_t size = 0;
#if defined(_WIN32)
		HANDLE hFile = INVALID_HANDLE_VALUE;
		HANDLE hMap = nullptr;
		void* view = nullptr;

		~OmdlMapping()
		{
			if (view) UnmapViewOfFile(view);
			if (hMap) CloseHandle(hMap);
			if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
		}

		bool Open(const std::string& path)
		{
			hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
								OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE) return false;
			LARGE_INTEGER sz;
			if (!GetFileSizeEx(hFile, &sz)) return false;
			size = static_cast<size_t>(sz.QuadPart);
			if (size == 0) return false;
			hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if (!hMap) return false;
			view = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
			if (!view) return false;
			base = static_cast<const uint8_t*>(view);
			return true;
		}
#else
		int fd = -1;
		void* view = nullptr;

		~OmdlMapping()
		{
			if (view && view != MAP_FAILED) munmap(view, size);
			if (fd >= 0) close(fd);
		}

		bool Open(const std::string& path)
		{
			fd = ::open(path.c_str(), O_RDONLY);
			if (fd < 0) return false;
			struct stat st;
			if (fstat(fd, &st) != 0) return false;
			size = static_cast<size_t>(st.st_size);
			if (size == 0) return false;
			view = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
			if (view == MAP_FAILED) return false;
			base = static_cast<const uint8_t*>(view);
			return true;
		}
#endif
	};

	// OmdlMapped move ops + destructor — defined here because OmdlMapping
	// is incomplete in the header.
	OmdlMapped::OmdlMapped() = default;
	OmdlMapped::~OmdlMapped() = default;
	OmdlMapped::OmdlMapped(OmdlMapped&&) noexcept = default;
	OmdlMapped& OmdlMapped::operator=(OmdlMapped&&) noexcept = default;

	namespace {

		// Bounds-checked little-endian read — bails out cleanly on truncation.
		struct Cursor
		{
			const uint8_t* p;
			const uint8_t* end;

			bool Read(void* out, size_t n)
			{
				if (static_cast<size_t>(end - p) < n) return false;
				std::memcpy(out, p, n);
				p += n;
				return true;
			}

			// Skip n bytes and return the pointer that just got skipped past.
			const uint8_t* Skip(size_t n)
			{
				if (static_cast<size_t>(end - p) < n) return nullptr;
				const uint8_t* start = p;
				p += n;
				return start;
			}
		};

		bool ReadStringField(Cursor& c, std::string& out)
		{
			uint16_t len = 0;
			if (!c.Read(&len, sizeof(len))) return false;
			if (len == 0) { out.clear(); return true; }
			if (static_cast<size_t>(c.end - c.p) < len) return false;
			out.assign(reinterpret_cast<const char*>(c.p), len);
			c.p += len;
			return true;
		}

	} // namespace

	bool ReadOmdl(const std::string& path, OmdlMapped& out)
	{
		auto mapping = std::make_unique<OmdlMapping>();
		if (!mapping->Open(path))
		{
			std::cerr << "[OmdlReader] Failed to map: " << path << "\n";
			return false;
		}

		Cursor c{ mapping->base, mapping->base + mapping->size };

		// Header
		if (!c.Read(&out.header, sizeof(OmdlHeader)))
		{
			std::cerr << "[OmdlReader] Truncated header in: " << path << "\n";
			return false;
		}
		if (out.header.magic != OMDL_MAGIC)
		{
			std::cerr << "[OmdlReader] Bad magic in: " << path << "\n";
			return false;
		}
		if (out.header.version != OMDL_VERSION)
		{
			std::cerr << "[OmdlReader] Unsupported OMDL version " << out.header.version
					  << " in: " << path << " (expected v" << OMDL_VERSION
					  << "; please re-export from the editor)\n";
			return false;
		}

		// Per-mesh info
		out.meshes.resize(out.header.meshCount);
		for (uint32_t i = 0; i < out.header.meshCount; i++)
		{
			auto& mesh = out.meshes[i];
			if (!c.Read(&mesh.indexCount, sizeof(mesh.indexCount)) ||
				!c.Read(&mesh.firstIndex, sizeof(mesh.firstIndex)) ||
				!c.Read(&mesh.baseVertex, sizeof(mesh.baseVertex)) ||
				!c.Read(&mesh.boundsMin, sizeof(mesh.boundsMin)) ||
				!c.Read(&mesh.boundsMax, sizeof(mesh.boundsMax)) ||
				!ReadStringField(c, mesh.albedoPath) ||
				!ReadStringField(c, mesh.normalPath))
			{
				std::cerr << "[OmdlReader] Truncated mesh-info in: " << path << "\n";
				return false;
			}
		}

		// Vertex blob — zero-copy: just point into the mapping.
		const size_t vertexBytes = static_cast<size_t>(out.header.totalVertices) * OMDL_VERTEX_BYTES;
		const uint8_t* vp = c.Skip(vertexBytes);
		if (vertexBytes > 0 && !vp)
		{
			std::cerr << "[OmdlReader] Truncated vertex blob in: " << path << "\n";
			return false;
		}
		out.vertexData = vp;
		out.vertexBytes = vertexBytes;

		// Index blob — uint16_t when OMDL_FLAG_U16_INDICES, else uint32_t.
		const bool u16 = (out.header.flags & OMDL_FLAG_U16_INDICES) != 0;
		const size_t indexBytes = static_cast<size_t>(out.header.totalIndices) * (u16 ? 2 : 4);
		const uint8_t* ip = c.Skip(indexBytes);
		if (indexBytes > 0 && !ip)
		{
			std::cerr << "[OmdlReader] Truncated index blob in: " << path << "\n";
			return false;
		}
		out.indexData = ip;
		out.indexBytes = indexBytes;

		// Hand the mapping to OmdlMapped so it stays alive while pointers are used.
		out.mapping = std::move(mapping);
		return true;
	}

} // namespace MMO
