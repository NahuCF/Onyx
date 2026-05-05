#pragma once

#include "OmdlFormat.h"
#include <string>

namespace MMO {

// Read an .omdl file into CPU memory. No GL — the caller creates GPU objects.
// Returns true on success.
bool ReadOmdl(const std::string& path, OmdlData& out);

} // namespace MMO
