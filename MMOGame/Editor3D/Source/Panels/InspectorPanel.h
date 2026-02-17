#pragma once

#include "EditorPanel.h"
#include <string>
#include <functional>

namespace MMO {

class ViewportPanel;
class WorldObject;
class StaticObject;
class SpawnPoint;
class Light;
class ParticleEmitter;
class TriggerVolume;
class InstancePortal;

class InspectorPanel : public EditorPanel {
public:
    InspectorPanel() { m_Name = "Inspector"; }
    ~InspectorPanel() override = default;

    void SetViewport(ViewportPanel* viewport) { m_Viewport = viewport; }
    void OnImGuiRender() override;

private:
    void RenderTransform(WorldObject* object);
    void RenderStaticObjectProperties(StaticObject* object);
    void RenderSpawnPointProperties(SpawnPoint* object);
    void RenderLightProperties(Light* object);
    void RenderParticleEmitterProperties(ParticleEmitter* object);
    void RenderTriggerVolumeProperties(TriggerVolume* object);
    void RenderInstancePortalProperties(InstancePortal* object);

    // Helper for material selector dropdown
    // Returns true if the materialId changed
    bool RenderMaterialSelector(const char* label, std::string& materialId);

    // Helper for path input with browse button
    // Returns true if the path changed
    bool RenderPathInput(const char* label, const std::string& currentPath,
                         const std::function<void(const std::string&)>& onPathChanged,
                         const char* filter = nullptr);

    ViewportPanel* m_Viewport = nullptr;

    // File browser state
    bool m_ShowFileBrowser = false;
    std::string m_BrowseFilter;
    std::string m_BrowseCurrentDir;
    std::function<void(const std::string&)> m_BrowseCallback;

    // Render file browser popup
    void RenderFileBrowserPopup();
};

} // namespace MMO
