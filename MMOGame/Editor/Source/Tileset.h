#pragma once

#include "EditorDefines.h"
#include <Graphics/Texture.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace MMO {

// ============================================================
// TILE PROPERTIES
// ============================================================

struct TileProperties {
    uint16_t id = 0;
    std::string name;
    bool collision = false;     // Blocks movement
    bool water = false;         // Water tile
};

// ============================================================
// TILESET
// ============================================================

class Tileset {
public:
    Tileset() = default;
    ~Tileset() = default;

    // Load tileset from image file
    bool Load(const std::string& imagePath, int32_t tileWidth, int32_t tileHeight);

    // Get texture for rendering
    Onyx::Texture* GetTexture() const { return m_Texture.get(); }

    // Get tile UV coordinates (in pixels)
    void GetTileUV(uint16_t tileIndex, float& outU, float& outV) const;

    // Get tile properties
    const TileProperties* GetTileProperties(uint16_t tileIndex) const;
    void SetTileCollision(uint16_t tileIndex, bool collision);

    // Getters
    const std::string& GetName() const { return m_Name; }
    const std::string& GetImagePath() const { return m_ImagePath; }
    int32_t GetTileWidth() const { return m_TileWidth; }
    int32_t GetTileHeight() const { return m_TileHeight; }
    int32_t GetTilesPerRow() const { return m_TilesPerRow; }
    int32_t GetTilesPerCol() const { return m_TilesPerCol; }
    int32_t GetTileCount() const { return m_TilesPerRow * m_TilesPerCol; }
    int32_t GetTextureWidth() const { return m_TextureWidth; }
    int32_t GetTextureHeight() const { return m_TextureHeight; }

    void SetName(const std::string& name) { m_Name = name; }

private:
    std::string m_Name;
    std::string m_ImagePath;
    std::unique_ptr<Onyx::Texture> m_Texture;

    int32_t m_TileWidth = 160;
    int32_t m_TileHeight = 80;
    int32_t m_TilesPerRow = 0;
    int32_t m_TilesPerCol = 0;
    int32_t m_TextureWidth = 0;
    int32_t m_TextureHeight = 0;

    std::vector<TileProperties> m_TileProperties;
};

// ============================================================
// TILESET MANAGER
// ============================================================

class TilesetManager {
public:
    static TilesetManager& Instance() {
        static TilesetManager instance;
        return instance;
    }

    // Load a tileset (returns index)
    uint16_t LoadTileset(const std::string& imagePath, int32_t tileWidth, int32_t tileHeight);

    // Get tileset by index
    Tileset* GetTileset(uint16_t index);
    const Tileset* GetTileset(uint16_t index) const;

    // Get tileset by name
    Tileset* GetTilesetByName(const std::string& name);

    // Get all tilesets
    const std::vector<std::unique_ptr<Tileset>>& GetTilesets() const { return m_Tilesets; }
    size_t GetTilesetCount() const { return m_Tilesets.size(); }

    // Remove tileset
    void RemoveTileset(uint16_t index);

    // Clear all
    void Clear();

private:
    TilesetManager() = default;

    std::vector<std::unique_ptr<Tileset>> m_Tilesets;
    std::unordered_map<std::string, uint16_t> m_PathToIndex;
};

} // namespace MMO
