#include "pch.h"
#include "AssetManager.h"
#include "Model.h"
#include "AnimatedModel.h"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace Onyx {

AssetManager::AssetManager() {
    m_DefaultAlbedo = Texture::CreateSolidColor(200, 200, 200);
    m_DefaultNormal = Texture::CreateSolidColor(128, 128, 255);
}

AssetManager::~AssetManager() = default;

ModelHandle AssetManager::LoadModel(const std::string& path, bool loadTextures) {
    auto it = m_ModelPaths.find(path);
    if (it != m_ModelPaths.end()) return it->second;

    ModelHandle handle;
    handle.id = m_NextId++;

    try {
        auto model = std::make_unique<Model>(path.c_str(), loadTextures);
        m_Models[handle.id] = std::move(model);
    } catch (...) {
        m_Models[handle.id] = nullptr;
    }

    m_ModelPaths[path] = handle;
    return handle;
}

Model* AssetManager::Get(ModelHandle handle) {
    auto it = m_Models.find(handle.id);
    return it != m_Models.end() ? it->second.get() : nullptr;
}

Model* AssetManager::FindModel(const std::string& path) {
    auto it = m_ModelPaths.find(path);
    if (it == m_ModelPaths.end()) return nullptr;
    auto modelIt = m_Models.find(it->second.id);
    return modelIt != m_Models.end() ? modelIt->second.get() : nullptr;
}

AnimatedModelHandle AssetManager::LoadAnimatedModel(const std::string& path) {
    auto it = m_AnimModelPaths.find(path);
    if (it != m_AnimModelPaths.end()) return it->second;

    AnimatedModelHandle handle;
    handle.id = m_NextId++;

    auto model = std::make_unique<AnimatedModel>();
    if (!model->Load(path)) {
        m_AnimModels[handle.id] = nullptr;
    } else {
        m_AnimModels[handle.id] = std::move(model);
    }

    m_AnimModelPaths[path] = handle;
    return handle;
}

AnimatedModel* AssetManager::Get(AnimatedModelHandle handle) {
    auto it = m_AnimModels.find(handle.id);
    return it != m_AnimModels.end() ? it->second.get() : nullptr;
}

AnimatedModel* AssetManager::FindAnimatedModel(const std::string& path) {
    auto it = m_AnimModelPaths.find(path);
    if (it == m_AnimModelPaths.end()) return nullptr;
    auto modelIt = m_AnimModels.find(it->second.id);
    return modelIt != m_AnimModels.end() ? modelIt->second.get() : nullptr;
}

TextureHandle AssetManager::LoadTexture(const std::string& path) {
    if (path.empty()) return {};

    auto it = m_TexturePaths.find(path);
    if (it != m_TexturePaths.end()) return it->second;

    TextureHandle handle;
    handle.id = m_NextId++;

    try {
        auto texture = std::make_unique<Texture>(path.c_str());
        m_Textures[handle.id] = std::move(texture);
    } catch (...) {
        m_Textures[handle.id] = nullptr;
    }

    m_TexturePaths[path] = handle;
    return handle;
}

Texture* AssetManager::Get(TextureHandle handle) {
    auto it = m_Textures.find(handle.id);
    return it != m_Textures.end() ? it->second.get() : nullptr;
}

Texture* AssetManager::ResolveTexture(const std::string& path) {
    if (path.empty()) return nullptr;
    return Get(LoadTexture(path));
}

Material& AssetManager::CreateMaterial(const std::string& id, const std::string& name) {
    auto& mat = m_Materials[id];
    mat.id = id;
    mat.name = name.empty() ? id : name;
    return mat;
}

void AssetManager::RegisterMaterial(const Material& mat) {
    m_Materials[mat.id] = mat;
}

std::string AssetManager::GenerateUniqueMaterialId(const std::string& baseName) const {
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

Material* AssetManager::GetMaterial(const std::string& id) {
    auto it = m_Materials.find(id);
    return it != m_Materials.end() ? &it->second : nullptr;
}

const Material* AssetManager::GetMaterial(const std::string& id) const {
    auto it = m_Materials.find(id);
    return it != m_Materials.end() ? &it->second : nullptr;
}

bool AssetManager::HasMaterial(const std::string& id) const {
    return m_Materials.find(id) != m_Materials.end();
}

bool AssetManager::RemoveMaterial(const std::string& id) {
    return m_Materials.erase(id) > 0;
}

std::vector<std::string> AssetManager::GetAllMaterialIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_Materials.size());
    for (const auto& [id, mat] : m_Materials) {
        ids.push_back(id);
    }
    return ids;
}

void AssetManager::Reload(ModelHandle handle) {
    std::string path;
    for (const auto& [p, h] : m_ModelPaths) {
        if (h == handle) { path = p; break; }
    }
    if (path.empty()) return;

    try {
        m_Models[handle.id] = std::make_unique<Model>(path.c_str(), false);
    } catch (...) {
        m_Models[handle.id] = nullptr;
    }
}

void AssetManager::Reload(AnimatedModelHandle handle) {
    std::string path;
    for (const auto& [p, h] : m_AnimModelPaths) {
        if (h == handle) { path = p; break; }
    }
    if (path.empty()) return;

    auto model = std::make_unique<AnimatedModel>();
    if (model->Load(path)) {
        m_AnimModels[handle.id] = std::move(model);
    } else {
        m_AnimModels[handle.id] = nullptr;
    }
}

} // namespace Onyx
