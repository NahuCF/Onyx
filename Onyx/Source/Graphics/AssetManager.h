#pragma once

#include "AssetHandle.h"
#include "Material.h"
#include "Texture.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Onyx {

class Model;
class AnimatedModel;

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    ModelHandle LoadModel(const std::string& path, bool loadTextures = false);
    Model* Get(ModelHandle handle);
    Model* FindModel(const std::string& path);

    AnimatedModelHandle LoadAnimatedModel(const std::string& path);
    AnimatedModel* Get(AnimatedModelHandle handle);
    AnimatedModel* FindAnimatedModel(const std::string& path);

    TextureHandle LoadTexture(const std::string& path);
    Texture* Get(TextureHandle handle);

    Texture* ResolveTexture(const std::string& path);

    Material& CreateMaterial(const std::string& id, const std::string& name = "");
    void RegisterMaterial(const Material& mat);
    std::string GenerateUniqueMaterialId(const std::string& baseName) const;
    Material* GetMaterial(const std::string& id);
    const Material* GetMaterial(const std::string& id) const;
    bool HasMaterial(const std::string& id) const;
    bool RemoveMaterial(const std::string& id);
    std::vector<std::string> GetAllMaterialIds() const;

    Texture* GetDefaultAlbedo() const { return m_DefaultAlbedo.get(); }
    Texture* GetDefaultNormal() const { return m_DefaultNormal.get(); }

    void Reload(ModelHandle handle);
    void Reload(AnimatedModelHandle handle);

private:
    uint32_t m_NextId = 1;

    std::unordered_map<std::string, ModelHandle> m_ModelPaths;
    std::unordered_map<std::string, AnimatedModelHandle> m_AnimModelPaths;
    std::unordered_map<std::string, TextureHandle> m_TexturePaths;

    std::unordered_map<uint32_t, std::unique_ptr<Model>> m_Models;
    std::unordered_map<uint32_t, std::unique_ptr<AnimatedModel>> m_AnimModels;
    std::unordered_map<uint32_t, std::unique_ptr<Texture>> m_Textures;

    std::unordered_map<std::string, Material> m_Materials;

    std::unique_ptr<Texture> m_DefaultAlbedo;
    std::unique_ptr<Texture> m_DefaultNormal;
};

} // namespace Onyx
