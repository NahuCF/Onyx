#pragma once

#include <string>

namespace Editor3D {

struct TerrainMaterial {
    std::string name;
    std::string id;
    std::string diffusePath;
    std::string normalPath;
    std::string rmaPath;
    float tilingScale = 8.0f;
    float normalStrength = 1.0f;
    std::string filePath;
};

bool SaveTerrainMaterial(const TerrainMaterial& mat, const std::string& path);
bool LoadTerrainMaterial(const std::string& path, TerrainMaterial& out);

} // namespace Editor3D
