#pragma once

#include "EditorPanel.h"

namespace Editor3D { class TerrainMaterialLibrary; }

namespace MMO {

class ViewportPanel;

class TerrainPanel : public EditorPanel {
public:
    TerrainPanel() { m_Name = "Terrain"; }
    ~TerrainPanel() override = default;

    void SetViewport(ViewportPanel* viewport) { m_Viewport = viewport; }
    void SetMaterialLibrary(Editor3D::TerrainMaterialLibrary* lib) { m_MaterialLibrary = lib; }
    void OnImGuiRender() override;

private:
    ViewportPanel* m_Viewport = nullptr;
    Editor3D::TerrainMaterialLibrary* m_MaterialLibrary = nullptr;
};

} // namespace MMO
