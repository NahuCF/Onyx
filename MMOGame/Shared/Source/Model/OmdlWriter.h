#pragma once

#include "OmdlFormat.h"
#include <string>

namespace MMO {

	// Write an .omdl file from pre-merged vertex/index data + mesh metadata.
	// Returns true on success.
	bool WriteOmdl(const std::string& path, const OmdlData& data);

} // namespace MMO
