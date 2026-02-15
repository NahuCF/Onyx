#include "TerrainMaterial.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace Editor3D {

bool SaveTerrainMaterial(const TerrainMaterial& mat, const std::string& path) {
    nlohmann::json j;
    j["name"] = mat.name;
    j["id"] = mat.id;
    j["diffuse"] = mat.diffusePath;
    j["normal"] = mat.normalPath;
    j["rma"] = mat.rmaPath;
    j["tilingScale"] = mat.tilingScale;
    j["normalStrength"] = mat.normalStrength;

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "[TerrainMaterial] Failed to save: " << path << std::endl;
        return false;
    }

    file << j.dump(4);
    return true;
}

bool LoadTerrainMaterial(const std::string& path, TerrainMaterial& out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[TerrainMaterial] Failed to load: " << path << std::endl;
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[TerrainMaterial] Parse error in " << path << ": " << e.what() << std::endl;
        return false;
    }

    out.name = j.value("name", "Unnamed");
    out.id = j.value("id", "");
    out.diffusePath = j.value("diffuse", "");
    out.normalPath = j.value("normal", "");
    out.rmaPath = j.value("rma", "");
    out.tilingScale = j.value("tilingScale", 8.0f);
    out.normalStrength = j.value("normalStrength", 1.0f);
    out.filePath = path;

    return true;
}

} // namespace Editor3D
