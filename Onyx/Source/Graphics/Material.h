#pragma once

#include <string>

namespace Onyx {

struct Material {
    std::string id;
    std::string name;
    std::string albedoPath;
    std::string normalPath;
    std::string rmaPath;
    float tilingScale = 8.0f;
    float normalStrength = 1.0f;
    std::string filePath;
};

} // namespace Onyx
