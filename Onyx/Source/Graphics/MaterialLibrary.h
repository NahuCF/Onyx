#pragma once

#include "Material.h"
#include "Texture.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Onyx {

class MaterialLibrary {
public:
    MaterialLibrary();
    ~MaterialLibrary() = default;

    Material& CreateMaterial(const std::string& id, const std::string& name = "");
    Material* GetMaterial(const std::string& id);
    const Material* GetMaterial(const std::string& id) const;
    bool RemoveMaterial(const std::string& id);
    std::vector<std::string> GetAllMaterialIds() const;
    bool HasMaterial(const std::string& id) const;

    Texture* ResolveTexture(const std::string& path);

    Texture* GetDefaultAlbedo() const { return m_DefaultAlbedo.get(); }
    Texture* GetDefaultNormal() const { return m_DefaultNormal.get(); }

    bool BindMaterial(const Material* material, int albedoSlot = 0, int normalSlot = 1);

private:
    std::unordered_map<std::string, Material> m_Materials;
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_TextureCache;

    std::unique_ptr<Texture> m_DefaultAlbedo;
    std::unique_ptr<Texture> m_DefaultNormal;
};

} // namespace Onyx
