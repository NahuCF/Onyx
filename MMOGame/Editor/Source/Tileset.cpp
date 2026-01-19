#include "Tileset.h"
#include <iostream>

namespace MMO {

// ============================================================
// TILESET IMPLEMENTATION
// ============================================================

bool Tileset::Load(const std::string& imagePath, int32_t tileWidth, int32_t tileHeight) {
    m_ImagePath = imagePath;
    m_TileWidth = tileWidth;
    m_TileHeight = tileHeight;

    // Extract name from path
    size_t lastSlash = imagePath.find_last_of("/\\");
    size_t lastDot = imagePath.find_last_of('.');
    if (lastSlash != std::string::npos && lastDot != std::string::npos) {
        m_Name = imagePath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    } else {
        m_Name = imagePath;
    }

    // Load texture with magenta (255, 0, 255) as transparent color key
    try {
        m_Texture = std::make_unique<Onyx::Texture>(imagePath.c_str(), 255, 0, 255);
    } catch (...) {
        std::cerr << "Failed to load tileset texture: " << imagePath << std::endl;
        return false;
    }

    // Get texture dimensions
    Onyx::Vector2D size = m_Texture->GetTextureSize();
    m_TextureWidth = static_cast<int32_t>(size.x);
    m_TextureHeight = static_cast<int32_t>(size.y);

    // Calculate tiles per row/col
    m_TilesPerRow = m_TextureWidth / m_TileWidth;
    m_TilesPerCol = m_TextureHeight / m_TileHeight;

    // Initialize tile properties
    int32_t tileCount = m_TilesPerRow * m_TilesPerCol;
    m_TileProperties.resize(tileCount);
    for (int32_t i = 0; i < tileCount; ++i) {
        m_TileProperties[i].id = static_cast<uint16_t>(i);
        m_TileProperties[i].name = "Tile " + std::to_string(i);
        m_TileProperties[i].collision = false;
    }

    std::cout << "Loaded tileset '" << m_Name << "': " << m_TilesPerRow << "x" << m_TilesPerCol
              << " tiles (" << tileCount << " total)" << std::endl;

    return true;
}

void Tileset::GetTileUV(uint16_t tileIndex, float& outU, float& outV) const {
    if (m_TilesPerRow == 0) {
        outU = 0;
        outV = 0;
        return;
    }

    int32_t tileX = tileIndex % m_TilesPerRow;
    int32_t tileY = tileIndex / m_TilesPerRow;

    // Return pixel coordinates (Renderer2D expects pixel coords for spriteCoord)
    outU = static_cast<float>(tileX * m_TileWidth);
    outV = static_cast<float>(tileY * m_TileHeight);
}

const TileProperties* Tileset::GetTileProperties(uint16_t tileIndex) const {
    if (tileIndex >= m_TileProperties.size()) {
        return nullptr;
    }
    return &m_TileProperties[tileIndex];
}

void Tileset::SetTileCollision(uint16_t tileIndex, bool collision) {
    if (tileIndex < m_TileProperties.size()) {
        m_TileProperties[tileIndex].collision = collision;
    }
}

// ============================================================
// TILESET MANAGER IMPLEMENTATION
// ============================================================

uint16_t TilesetManager::LoadTileset(const std::string& imagePath, int32_t tileWidth, int32_t tileHeight) {
    // Check if already loaded
    auto it = m_PathToIndex.find(imagePath);
    if (it != m_PathToIndex.end()) {
        return it->second;
    }

    // Load new tileset
    auto tileset = std::make_unique<Tileset>();
    if (!tileset->Load(imagePath, tileWidth, tileHeight)) {
        return 0;  // Return 0 for failure (reserved as "no tileset")
    }

    // Index 0 is reserved for "empty/no tileset"
    uint16_t index = static_cast<uint16_t>(m_Tilesets.size() + 1);
    m_Tilesets.push_back(std::move(tileset));
    m_PathToIndex[imagePath] = index;

    return index;
}

Tileset* TilesetManager::GetTileset(uint16_t index) {
    if (index == 0 || index > m_Tilesets.size()) {
        return nullptr;
    }
    return m_Tilesets[index - 1].get();
}

const Tileset* TilesetManager::GetTileset(uint16_t index) const {
    if (index == 0 || index > m_Tilesets.size()) {
        return nullptr;
    }
    return m_Tilesets[index - 1].get();
}

Tileset* TilesetManager::GetTilesetByName(const std::string& name) {
    for (auto& tileset : m_Tilesets) {
        if (tileset->GetName() == name) {
            return tileset.get();
        }
    }
    return nullptr;
}

void TilesetManager::RemoveTileset(uint16_t index) {
    if (index == 0 || index > m_Tilesets.size()) {
        return;
    }

    // Remove from path map
    const std::string& path = m_Tilesets[index - 1]->GetImagePath();
    m_PathToIndex.erase(path);

    // Erase from vector
    m_Tilesets.erase(m_Tilesets.begin() + (index - 1));

    // Update indices in path map
    m_PathToIndex.clear();
    for (size_t i = 0; i < m_Tilesets.size(); ++i) {
        m_PathToIndex[m_Tilesets[i]->GetImagePath()] = static_cast<uint16_t>(i + 1);
    }
}

void TilesetManager::Clear() {
    m_Tilesets.clear();
    m_PathToIndex.clear();
}

} // namespace MMO
