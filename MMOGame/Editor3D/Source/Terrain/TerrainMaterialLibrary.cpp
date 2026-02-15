#include "TerrainMaterialLibrary.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace Editor3D {

static bool ParseSolidColor(const std::string& path, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (path.rfind("__solid_", 0) != 0) return false;
    std::string rgb = path.substr(8);
    r = 128; g = 128; b = 128;
    size_t p1 = rgb.find('_');
    if (p1 != std::string::npos) {
        r = static_cast<uint8_t>(std::stoi(rgb.substr(0, p1)));
        size_t p2 = rgb.find('_', p1 + 1);
        if (p2 != std::string::npos) {
            g = static_cast<uint8_t>(std::stoi(rgb.substr(p1 + 1, p2 - p1 - 1)));
            b = static_cast<uint8_t>(std::stoi(rgb.substr(p2 + 1)));
        }
    }
    return true;
}

void TerrainMaterialLibrary::Init(const std::string& assetRoot) {
    m_AssetRoot = assetRoot;
    m_MaterialsDir = assetRoot + "/materials/terrain";

    std::filesystem::create_directories(m_MaterialsDir);
    CreateDefaultTextures();
    ScanForMaterials();
    EnsureDefaultMaterials();
}

void TerrainMaterialLibrary::ScanForMaterials() {
    m_Materials.clear();
    m_MaterialIds.clear();

    namespace fs = std::filesystem;

    if (!fs::exists(m_AssetRoot)) return;

    for (const auto& entry : fs::recursive_directory_iterator(m_AssetRoot)) {
        if (entry.path().extension() == ".terrainmat") {
            TerrainMaterial mat;
            if (LoadTerrainMaterial(entry.path().generic_string(), mat)) {
                if (mat.id.empty()) {
                    mat.id = entry.path().stem().string();
                }
                mat.filePath = entry.path().generic_string();
                m_Materials[mat.id] = mat;
                m_MaterialIds.push_back(mat.id);
            }
        }
    }

    std::sort(m_MaterialIds.begin(), m_MaterialIds.end());
    m_ArraysDirty = true;
}

const TerrainMaterial* TerrainMaterialLibrary::GetMaterial(const std::string& id) const {
    auto it = m_Materials.find(id);
    if (it == m_Materials.end()) return nullptr;
    return &it->second;
}

TerrainMaterial* TerrainMaterialLibrary::GetMaterialMutable(const std::string& id) {
    auto it = m_Materials.find(id);
    if (it == m_Materials.end()) return nullptr;
    return &it->second;
}

std::string TerrainMaterialLibrary::CreateMaterial(const std::string& name, const std::string& directory) {
    std::string id = GenerateUniqueId(name);
    std::string dir = directory.empty() ? m_MaterialsDir : directory;

    std::filesystem::create_directories(dir);

    TerrainMaterial mat;
    mat.name = name;
    mat.id = id;
    mat.tilingScale = 8.0f;
    mat.normalStrength = 1.0f;
    mat.filePath = dir + "/" + id + ".terrainmat";

    SaveTerrainMaterial(mat, mat.filePath);

    m_Materials[id] = mat;
    m_MaterialIds.push_back(id);
    std::sort(m_MaterialIds.begin(), m_MaterialIds.end());
    m_ArraysDirty = true;

    return id;
}

void TerrainMaterialLibrary::SaveMaterial(const std::string& id) {
    auto it = m_Materials.find(id);
    if (it == m_Materials.end()) return;

    if (it->second.filePath.empty()) {
        it->second.filePath = m_MaterialsDir + "/" + id + ".terrainmat";
    }

    SaveTerrainMaterial(it->second, it->second.filePath);
}

void TerrainMaterialLibrary::UpdateMaterial(const std::string& id, const TerrainMaterial& mat) {
    bool isNew = (m_Materials.find(id) == m_Materials.end());
    m_Materials[id] = mat;
    if (isNew) {
        m_MaterialIds.push_back(id);
        std::sort(m_MaterialIds.begin(), m_MaterialIds.end());
    }
    m_ArraysDirty = true;
}

Onyx::Texture* TerrainMaterialLibrary::GetMaterialTexture(
    const std::string& materialId,
    std::string TerrainMaterial::*pathMember, Onyx::Texture* fallback)
{
    auto* mat = GetMaterial(materialId);
    if (!mat || (mat->*pathMember).empty()) return fallback;
    return LoadOrGetCachedTexture(mat->*pathMember);
}

Onyx::Texture* TerrainMaterialLibrary::GetDiffuseTexture(const std::string& materialId) {
    return GetMaterialTexture(materialId, &TerrainMaterial::diffusePath, m_DefaultDiffuse.get());
}

Onyx::Texture* TerrainMaterialLibrary::GetNormalTexture(const std::string& materialId) {
    return GetMaterialTexture(materialId, &TerrainMaterial::normalPath, m_DefaultNormal.get());
}

Onyx::Texture* TerrainMaterialLibrary::GetRMATexture(const std::string& materialId) {
    return GetMaterialTexture(materialId, &TerrainMaterial::rmaPath, m_DefaultRMA.get());
}

Onyx::Texture* TerrainMaterialLibrary::LoadOrGetCachedTexture(const std::string& path) {
    if (path.empty()) return nullptr;

    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end()) {
        return it->second.get();
    }

    uint8_t r, g, b;
    if (ParseSolidColor(path, r, g, b)) {
        m_TextureCache[path] = Onyx::Texture::CreateSolidColor(r, g, b);
        return m_TextureCache[path].get();
    }

    std::string fullPath = path;
    if (!std::filesystem::exists(fullPath)) {
        fullPath = m_AssetRoot + "/" + path;
    }

    if (!std::filesystem::exists(fullPath)) {
        std::cerr << "[TerrainMaterialLibrary] Texture not found: " << path << std::endl;
        m_TextureCache[path] = nullptr;
        return nullptr;
    }

    try {
        auto texture = std::make_unique<Onyx::Texture>(fullPath.c_str());
        Onyx::Texture* ptr = texture.get();
        m_TextureCache[path] = std::move(texture);
        return ptr;
    } catch (...) {
        std::cerr << "[TerrainMaterialLibrary] Failed to load texture: " << fullPath << std::endl;
        m_TextureCache[path] = nullptr;
        return nullptr;
    }
}

void TerrainMaterialLibrary::CreateDefaultTextures() {
    m_DefaultDiffuse = Onyx::Texture::CreateSolidColor(255, 255, 255);
    m_DefaultNormal = Onyx::Texture::CreateSolidColor(128, 128, 255);
    m_DefaultRMA = Onyx::Texture::CreateSolidColor(128, 0, 255);
}

void TerrainMaterialLibrary::EnsureDefaultMaterials() {
    if (!m_Materials.empty()) return;

    struct DefaultMat {
        const char* name;
        const char* id;
        uint8_t r, g, b;
    };

    DefaultMat defaults[] = {
        {"Grass", "grass", 80, 140, 50},
        {"Dirt",  "dirt",  140, 100, 60},
        {"Rock",  "rock",  130, 130, 130},
        {"Sand",  "sand",  200, 180, 120},
    };

    for (auto& def : defaults) {
        TerrainMaterial mat;
        mat.name = def.name;
        mat.id = def.id;
        mat.tilingScale = 8.0f;
        mat.normalStrength = 1.0f;
        mat.filePath = m_MaterialsDir + "/" + std::string(def.id) + ".terrainmat";

        mat.diffusePath = "__solid_" + std::to_string(def.r) + "_"
                        + std::to_string(def.g) + "_" + std::to_string(def.b);

        SaveTerrainMaterial(mat, mat.filePath);
        m_Materials[mat.id] = mat;
        m_MaterialIds.push_back(mat.id);
    }

    std::sort(m_MaterialIds.begin(), m_MaterialIds.end());
}

std::string TerrainMaterialLibrary::GenerateUniqueId(const std::string& baseName) const {
    std::string id = baseName;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    std::replace(id.begin(), id.end(), ' ', '_');

    id.erase(std::remove_if(id.begin(), id.end(), [](char c) {
        return !std::isalnum(c) && c != '_';
    }), id.end());

    if (id.empty()) id = "material";

    if (m_Materials.find(id) == m_Materials.end()) return id;

    for (int i = 1; ; i++) {
        std::string candidate = id + "_" + std::to_string(i);
        if (m_Materials.find(candidate) == m_Materials.end()) return candidate;
    }
}

void TerrainMaterialLibrary::LoadArrayLayer(Onyx::TextureArray* array, int layer,
    const std::string& path, uint8_t fallbackR, uint8_t fallbackG, uint8_t fallbackB)
{
    if (path.empty()) {
        array->SetLayerSolidColor(layer, fallbackR, fallbackG, fallbackB);
        return;
    }

    uint8_t r, g, b;
    if (ParseSolidColor(path, r, g, b)) {
        array->SetLayerSolidColor(layer, r, g, b);
        return;
    }

    std::string fullPath = path;
    if (!std::filesystem::exists(fullPath))
        fullPath = m_AssetRoot + "/" + path;

    if (std::filesystem::exists(fullPath))
        array->LoadLayerFromFile(fullPath, layer);
    else
        array->SetLayerSolidColor(layer, fallbackR, fallbackG, fallbackB);
}

void TerrainMaterialLibrary::RebuildTextureArrays() {
    if (!m_ArraysDirty) return;
    m_ArraysDirty = false;

    int numMaterials = std::max(1, (int)m_MaterialIds.size());
    const int ARRAY_SIZE = 1024;

    m_DiffuseArray = std::make_unique<Onyx::TextureArray>();
    m_NormalArray = std::make_unique<Onyx::TextureArray>();
    m_RMAArray = std::make_unique<Onyx::TextureArray>();

    m_DiffuseArray->Create(ARRAY_SIZE, ARRAY_SIZE, numMaterials, 4);
    m_NormalArray->Create(ARRAY_SIZE, ARRAY_SIZE, numMaterials, 4);
    m_RMAArray->Create(ARRAY_SIZE, ARRAY_SIZE, numMaterials, 4);

    m_MaterialArrayIndex.clear();

    for (int i = 0; i < (int)m_MaterialIds.size(); i++) {
        const auto& matId = m_MaterialIds[i];
        m_MaterialArrayIndex[matId] = i;

        const auto* mat = GetMaterial(matId);
        if (!mat) continue;

        LoadArrayLayer(m_DiffuseArray.get(), i, mat->diffusePath, 255, 255, 255);
        LoadArrayLayer(m_NormalArray.get(), i, mat->normalPath, 128, 128, 255);
        LoadArrayLayer(m_RMAArray.get(), i, mat->rmaPath, 128, 0, 255);
    }
}

int TerrainMaterialLibrary::GetMaterialArrayIndex(const std::string& materialId) const {
    if (materialId.empty()) return 0;
    auto it = m_MaterialArrayIndex.find(materialId);
    if (it != m_MaterialArrayIndex.end()) return it->second;
    return 0;
}

} // namespace Editor3D
