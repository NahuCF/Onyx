#pragma once

#include "MaterialSerializer.h"
#include <Graphics/Material.h>
#include <Graphics/Texture.h>
#include <Graphics/TextureArray.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Onyx { class AssetManager; }

namespace Editor3D {

class TerrainMaterialLibrary {
public:
    void Init(const std::string& assetRoot, Onyx::AssetManager* assetManager);

    const Onyx::Material* GetMaterial(const std::string& id) const;
    Onyx::Material* GetMaterialMutable(const std::string& id);
    std::string CreateMaterial(const std::string& name, const std::string& directory = "");
    void SaveMaterial(const std::string& id);
    void UpdateMaterial(const std::string& id, const Onyx::Material& mat);
    std::vector<std::string> GetMaterialIds() const;

    Onyx::Texture* GetDiffuseTexture(const std::string& materialId);
    Onyx::Texture* GetNormalTexture(const std::string& materialId);
    Onyx::Texture* GetRMATexture(const std::string& materialId);

    Onyx::Texture* GetDefaultDiffuse() { return m_DefaultDiffuse.get(); }
    Onyx::Texture* GetDefaultNormal() { return m_DefaultNormal.get(); }
    Onyx::Texture* GetDefaultRMA() { return m_DefaultRMA.get(); }

    const std::string& GetAssetRoot() const { return m_AssetRoot; }

    Onyx::Texture* LoadOrGetCachedTexture(const std::string& path);

    void RebuildTextureArrays();
    int GetMaterialArrayIndex(const std::string& materialId) const;
    Onyx::TextureArray* GetDiffuseArray() { return m_DiffuseArray.get(); }
    Onyx::TextureArray* GetNormalArray() { return m_NormalArray.get(); }
    Onyx::TextureArray* GetRMAArray() { return m_RMAArray.get(); }
    bool IsArraysDirty() const { return m_ArraysDirty; }
    void MarkArraysDirty() { m_ArraysDirty = true; }

private:
    void CreateDefaultTextures();
    Onyx::Texture* GetMaterialTexture(const std::string& materialId,
        std::string Onyx::Material::*pathMember, Onyx::Texture* fallback);
    void LoadArrayLayer(Onyx::TextureArray* array, int layer,
        const std::string& path, uint8_t fallbackR, uint8_t fallbackG, uint8_t fallbackB);

    std::string m_AssetRoot;
    std::string m_MaterialsDir;

    Onyx::AssetManager* m_AssetManager = nullptr;

    std::unordered_map<std::string, std::unique_ptr<Onyx::Texture>> m_TextureCache;

    std::unique_ptr<Onyx::Texture> m_DefaultDiffuse;
    std::unique_ptr<Onyx::Texture> m_DefaultNormal;
    std::unique_ptr<Onyx::Texture> m_DefaultRMA;

    std::unique_ptr<Onyx::TextureArray> m_DiffuseArray;
    std::unique_ptr<Onyx::TextureArray> m_NormalArray;
    std::unique_ptr<Onyx::TextureArray> m_RMAArray;
    std::unordered_map<std::string, int> m_MaterialArrayIndex;
    bool m_ArraysDirty = true;
};

} // namespace Editor3D
