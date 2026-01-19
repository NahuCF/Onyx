#pragma once

#include "World/WorldTypes.h"
#include "Gizmo/TransformGizmo.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace MMO {

class EditorWorld {
public:
    EditorWorld();
    ~EditorWorld();

    // ─────────────────────────────────────────────────────────────
    // MAP MANAGEMENT
    // ─────────────────────────────────────────────────────────────
    void NewMap(const std::string& name);
    void Clear();

    const std::string& GetMapName() const { return m_MapName; }
    void SetMapName(const std::string& name) { m_MapName = name; }

    bool IsDirty() const { return m_Dirty; }
    void SetDirty(bool dirty) { m_Dirty = dirty; }

    // ─────────────────────────────────────────────────────────────
    // OBJECT CREATION
    // ─────────────────────────────────────────────────────────────
    StaticObject* CreateStaticObject(const std::string& name = "Static Object");
    SpawnPoint* CreateSpawnPoint(const std::string& name = "Spawn Point");
    Light* CreateLight(LightType type = LightType::POINT, const std::string& name = "");
    ParticleEmitter* CreateParticleEmitter(const std::string& name = "Particle Emitter");
    TriggerVolume* CreateTriggerVolume(const std::string& name = "Trigger Volume");
    InstancePortal* CreateInstancePortal(const std::string& name = "Instance Portal");
    GroupObject* CreateGroup(const std::string& name = "Group");

    // ─────────────────────────────────────────────────────────────
    // OBJECT ACCESS
    // ─────────────────────────────────────────────────────────────
    WorldObject* GetObject(uint64_t guid);
    const WorldObject* GetObject(uint64_t guid) const;

    template<typename T>
    T* GetObjectAs(uint64_t guid) {
        WorldObject* obj = GetObject(guid);
        return obj ? dynamic_cast<T*>(obj) : nullptr;
    }

    // ─────────────────────────────────────────────────────────────
    // OBJECT ITERATION
    // ─────────────────────────────────────────────────────────────
    const std::vector<std::unique_ptr<StaticObject>>& GetStaticObjects() const { return m_StaticObjects; }
    const std::vector<std::unique_ptr<SpawnPoint>>& GetSpawnPoints() const { return m_SpawnPoints; }
    const std::vector<std::unique_ptr<Light>>& GetLights() const { return m_Lights; }
    const std::vector<std::unique_ptr<ParticleEmitter>>& GetParticleEmitters() const { return m_ParticleEmitters; }
    const std::vector<std::unique_ptr<TriggerVolume>>& GetTriggerVolumes() const { return m_TriggerVolumes; }
    const std::vector<std::unique_ptr<InstancePortal>>& GetInstancePortals() const { return m_InstancePortals; }
    const std::vector<std::unique_ptr<GroupObject>>& GetGroups() const { return m_Groups; }

    // Iterate all objects
    void ForEachObject(const std::function<void(WorldObject*)>& callback);
    void ForEachObject(const std::function<void(const WorldObject*)>& callback) const;

    size_t GetObjectCount() const;

    // ─────────────────────────────────────────────────────────────
    // OBJECT DELETION
    // ─────────────────────────────────────────────────────────────
    void DeleteObject(uint64_t guid);
    void DeleteSelected();

    // ─────────────────────────────────────────────────────────────
    // SELECTION
    // ─────────────────────────────────────────────────────────────
    void Select(WorldObject* object, bool addToSelection = false);
    void SelectByGuid(uint64_t guid, bool addToSelection = false);
    void Deselect(WorldObject* object);
    void DeselectAll();
    void SelectAll();

    bool IsSelected(const WorldObject* object) const;
    bool HasSelection() const { return !m_SelectedObjects.empty(); }
    size_t GetSelectionCount() const { return m_SelectedObjects.size(); }

    const std::vector<WorldObject*>& GetSelectedObjects() const { return m_SelectedObjects; }
    WorldObject* GetPrimarySelection() const;

    // Mesh selection within a model
    void SelectMesh(int meshIndex) { m_SelectedMeshIndex = meshIndex; }
    void DeselectMesh() { m_SelectedMeshIndex = -1; m_SelectedMeshName.clear(); }
    int GetSelectedMeshIndex() const { return m_SelectedMeshIndex; }
    bool HasMeshSelected() const { return m_SelectedMeshIndex >= 0; }

    // ─────────────────────────────────────────────────────────────
    // HIERARCHY (Parent-Child)
    // ─────────────────────────────────────────────────────────────
    void SetParent(WorldObject* child, GroupObject* parent);
    void Unparent(WorldObject* child);
    GroupObject* GetParentGroup(WorldObject* object);

    // Get world transform considering parent hierarchy
    glm::mat4 GetWorldMatrix(const WorldObject* object) const;

    // Get root-level objects (no parent) in display order
    std::vector<WorldObject*> GetRootObjects();
    const std::vector<uint64_t>& GetRootDisplayOrder() const { return m_RootDisplayOrder; }

    // Display order management (for hierarchy panel)
    void MoveInDisplayOrder(uint64_t guid, size_t newIndex);
    void MoveChildInGroup(GroupObject* group, uint64_t childGuid, size_t newIndex);
    void UnparentAtIndex(WorldObject* child, size_t rootIndex);  // Unparent and insert at specific position
    void SetParentAtIndex(WorldObject* child, GroupObject* parent, size_t childIndex);  // Parent at specific child position

    // Grouping operations
    GroupObject* GroupSelected(const std::string& name = "Group");  // Create group from selection
    void UngroupSelected();                                          // Remove group, keep children

    // ─────────────────────────────────────────────────────────────
    // VISIBILITY FILTERS
    // ─────────────────────────────────────────────────────────────
    void SetTypeVisible(WorldObjectType type, bool visible);
    bool IsTypeVisible(WorldObjectType type) const;

    // ─────────────────────────────────────────────────────────────
    // GIZMO STATE (shared editor state)
    // ─────────────────────────────────────────────────────────────
    void SetGizmoMode(GizmoMode mode) { m_GizmoMode = mode; }
    GizmoMode GetGizmoMode() const { return m_GizmoMode; }

    void SetSnapEnabled(bool enabled) { m_SnapEnabled = enabled; }
    bool IsSnapEnabled() const { return m_SnapEnabled; }

    void SetSnapValue(float value) { m_SnapValue = value; }
    float GetSnapValue() const { return m_SnapValue; }

    void SetRotationSnapValue(float degrees) { m_RotationSnapValue = degrees; }
    float GetRotationSnapValue() const { return m_RotationSnapValue; }

    void SetScaleSnapValue(float value) { m_ScaleSnapValue = value; }
    float GetScaleSnapValue() const { return m_ScaleSnapValue; }

    // ─────────────────────────────────────────────────────────────
    // CLIPBOARD (Copy/Paste/Duplicate)
    // ─────────────────────────────────────────────────────────────
    // Object operations (Ctrl+C/V/D)
    void Copy();                              // Copy selected objects to clipboard
    void Paste();                             // Paste clipboard objects
    void Duplicate();                         // Copy + Paste in one action

    // Mesh operations
    void CopyMesh();                          // Copy mesh transform (Ctrl+Shift+C)
    void PasteMesh();                         // Paste mesh transform (Ctrl+Shift+V)
    void CopySingleMesh();                    // Copy object with only selected mesh visible

    bool HasClipboardData() const { return !m_Clipboard.empty() || m_HasMeshClipboard; }
    bool HasMeshClipboard() const { return m_HasMeshClipboard; }

    // Store selected mesh name for clipboard operations
    void SetSelectedMeshName(const std::string& name) { m_SelectedMeshName = name; }
    const std::string& GetSelectedMeshName() const { return m_SelectedMeshName; }

    // Add an already-created object to the world (used for paste)
    WorldObject* AddObject(std::unique_ptr<WorldObject> object);

private:
    uint64_t GenerateGuid();

    template<typename T>
    void RemoveFromVector(std::vector<std::unique_ptr<T>>& vec, uint64_t guid);

    std::string m_MapName = "Untitled";
    bool m_Dirty = false;
    uint64_t m_NextGuid = 1;

    // Object storage by type (for fast iteration)
    std::vector<std::unique_ptr<StaticObject>> m_StaticObjects;
    std::vector<std::unique_ptr<SpawnPoint>> m_SpawnPoints;
    std::vector<std::unique_ptr<Light>> m_Lights;
    std::vector<std::unique_ptr<ParticleEmitter>> m_ParticleEmitters;
    std::vector<std::unique_ptr<TriggerVolume>> m_TriggerVolumes;
    std::vector<std::unique_ptr<InstancePortal>> m_InstancePortals;
    std::vector<std::unique_ptr<GroupObject>> m_Groups;

    // Fast lookup by GUID
    std::unordered_map<uint64_t, WorldObject*> m_ObjectsByGuid;

    // Selection
    std::vector<WorldObject*> m_SelectedObjects;
    int m_SelectedMeshIndex = -1;  // -1 = no mesh selected, >= 0 = mesh index within model

    // Display order for root-level objects in hierarchy panel
    std::vector<uint64_t> m_RootDisplayOrder;

    // Visibility filters
    std::unordered_map<WorldObjectType, bool> m_TypeVisibility;

    // Gizmo state
    GizmoMode m_GizmoMode = GizmoMode::TRANSLATE;
    bool m_SnapEnabled = false;
    float m_SnapValue = 1.0f;           // Translation snap (units)
    float m_RotationSnapValue = 15.0f;  // Rotation snap (degrees)
    float m_ScaleSnapValue = 0.25f;     // Scale snap

    // Clipboard for copy/paste
    std::vector<std::unique_ptr<WorldObject>> m_Clipboard;

    // Mesh-level clipboard
    MeshMaterial m_MeshClipboard;
    bool m_HasMeshClipboard = false;
    std::string m_SelectedMeshName;  // Currently selected mesh name within model
};

} // namespace MMO
