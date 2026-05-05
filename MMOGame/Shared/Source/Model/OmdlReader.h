#pragma once

#include "OmdlFormat.h"
#include <string>

namespace MMO {

	// Memory-map an .omdl file and return zero-copy views into it. Pointers in
	// OmdlMapped are valid until the returned object is destructed (which unmaps).
	// No GL — the caller calls glBufferData(...) directly from out.vertexData.
	bool ReadOmdl(const std::string& path, OmdlMapped& out);

} // namespace MMO
