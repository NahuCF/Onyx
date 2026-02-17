#include "MaterialLibrary.h"
#include <iostream>

namespace Onyx {

MaterialLibrary::MaterialLibrary() {
    m_DefaultAlbedo = Texture::CreateSolidColor(255, 255, 255);
    m_DefaultNormal = Texture::CreateSolidColor(128, 128, 255);
}

Material& MaterialLibrary::CreateMaterial(const std::string& id, const std::string& name) {
    auto& mat = m_Materials[id];
    mat.id = id;
    mat.name = name.empty() ? id : name;
    return mat;
}

Material* MaterialLibrary::GetMaterial(const std::string& id) {
    auto it = m_Materials.find(id);
    return it != m_Materials.end() ? &it->second : nullptr;
}

const Material* MaterialLibrary::GetMaterial(const std::string& id) const {
    auto it = m_Materials.find(id);
    return it != m_Materials.end() ? &it->second : nullptr;
}

bool MaterialLibrary::RemoveMaterial(const std::string& id) {
    return m_Materials.erase(id) > 0;
}

std::vector<std::string> MaterialLibrary::GetAllMaterialIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_Materials.size());
    for (const auto& [id, mat] : m_Materials) {
        ids.push_back(id);
    }
    return ids;
}

bool MaterialLibrary::HasMaterial(const std::string& id) const {
    return m_Materials.find(id) != m_Materials.end();
}

Texture* MaterialLibrary::ResolveTexture(const std::string& path) {
    if (path.empty()) return nullptr;

    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end()) {
        return it->second.get();
    }

    try {
        auto texture = std::make_unique<Texture>(path.c_str());
        Texture* ptr = texture.get();
        m_TextureCache[path] = std::move(texture);
        return ptr;
    } catch (...) {
        m_TextureCache[path] = nullptr;
        return nullptr;
    }
}

bool MaterialLibrary::BindMaterial(const Material* material, int albedoSlot, int normalSlot) {
    Texture* albedo = nullptr;
    Texture* normal = nullptr;

    if (material) {
        albedo = ResolveTexture(material->albedoPath);
        normal = ResolveTexture(material->normalPath);
    }

    if (albedo) albedo->Bind(albedoSlot);
    else m_DefaultAlbedo->Bind(albedoSlot);

    if (normal) {
        normal->Bind(normalSlot);
        return true;
    }
    return false;
}

} // namespace Onyx
