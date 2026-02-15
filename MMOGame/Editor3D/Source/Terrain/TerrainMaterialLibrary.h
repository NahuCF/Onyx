#pragma once

#include "TerrainMaterial.h"
#include <Graphics/Texture.h>
#include <Graphics/TextureArray.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Editor3D {

class TerrainMaterialLibrary {
public:
    void Init(const std::string& assetRoot);
    void ScanForMaterials();

    const TerrainMaterial* GetMaterial(const std::string& id) const;
    TerrainMaterial* GetMaterialMutable(const std::string& id);
    std::string CreateMaterial(const std::string& name, const std::string& directory = "");
    void SaveMaterial(const std::string& id);
    void UpdateMaterial(const std::string& id, const TerrainMaterial& mat);
    const std::vector<std::string>& GetMaterialIds() const { return m_MaterialIds; }

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

private:
    void CreateDefaultTextures();
    void EnsureDefaultMaterials();
    std::string GenerateUniqueId(const std::string& baseName) const;
    Onyx::Texture* GetMaterialTexture(const std::string& materialId,
        std::string TerrainMaterial::*pathMember, Onyx::Texture* fallback);
    void LoadArrayLayer(Onyx::TextureArray* array, int layer,
        const std::string& path, uint8_t fallbackR, uint8_t fallbackG, uint8_t fallbackB);

    std::string m_AssetRoot;
    std::string m_MaterialsDir;

    std::unordered_map<std::string, TerrainMaterial> m_Materials;
    std::vector<std::string> m_MaterialIds;

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
