#pragma once

#include <string>

namespace Onyx {

struct Material {
    std::string id;           // Unique key (e.g., "stone_wall")
    std::string name;         // Display name
    std::string albedoPath;   // Diffuse/base color texture path
    std::string normalPath;   // Normal map path
};

} // namespace Onyx
