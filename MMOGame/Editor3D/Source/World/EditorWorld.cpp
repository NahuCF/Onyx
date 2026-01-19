#include "EditorWorld.h"
#include <algorithm>
#include <iostream>

namespace MMO {

EditorWorld::EditorWorld() {
    // Initialize visibility - all types visible by default
    m_TypeVisibility[WorldObjectType::STATIC_OBJECT] = true;
    m_TypeVisibility[WorldObjectType::SPAWN_POINT] = true;
    m_TypeVisibility[WorldObjectType::LIGHT] = true;
    m_TypeVisibility[WorldObjectType::PARTICLE_EMITTER] = true;
    m_TypeVisibility[WorldObjectType::TRIGGER_VOLUME] = true;
    m_TypeVisibility[WorldObjectType::INSTANCE_PORTAL] = true;
    m_TypeVisibility[WorldObjectType::GROUP] = true;
}

EditorWorld::~EditorWorld() {
    Clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// MAP MANAGEMENT
// ─────────────────────────────────────────────────────────────────────────────

void EditorWorld::NewMap(const std::string& name) {
    Clear();
    m_MapName = name;
    m_Dirty = false;
}

void EditorWorld::Clear() {
    m_SelectedObjects.clear();
    m_SelectedMeshIndex = -1;
    m_ObjectsByGuid.clear();
    m_RootDisplayOrder.clear();

    m_StaticObjects.clear();
    m_SpawnPoints.clear();
    m_Lights.clear();
    m_ParticleEmitters.clear();
    m_TriggerVolumes.clear();
    m_InstancePortals.clear();
    m_Groups.clear();

    m_NextGuid = 1;
    m_Dirty = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// OBJECT CREATION
// ─────────────────────────────────────────────────────────────────────────────

StaticObject* EditorWorld::CreateStaticObject(const std::string& name) {
    uint64_t guid = GenerateGuid();
    auto obj = std::make_unique<StaticObject>(guid, name);
    StaticObject* ptr = obj.get();

    m_ObjectsByGuid[guid] = ptr;
    m_StaticObjects.push_back(std::move(obj));
    m_RootDisplayOrder.push_back(guid);  // Add to display order

    m_Dirty = true;
    return ptr;
}

SpawnPoint* EditorWorld::CreateSpawnPoint(const std::string& name) {
    uint64_t guid = GenerateGuid();
    auto obj = std::make_unique<SpawnPoint>(guid, name);
    SpawnPoint* ptr = obj.get();

    m_ObjectsByGuid[guid] = ptr;
    m_SpawnPoints.push_back(std::move(obj));
    m_RootDisplayOrder.push_back(guid);

    m_Dirty = true;
    return ptr;
}

Light* EditorWorld::CreateLight(LightType type, const std::string& name) {
    uint64_t guid = GenerateGuid();
    std::string lightName = name;
    if (lightName.empty()) {
        switch (type) {
            case LightType::POINT: lightName = "Point Light"; break;
            case LightType::SPOT: lightName = "Spot Light"; break;
            case LightType::DIRECTIONAL: lightName = "Directional Light"; break;
        }
    }

    auto obj = std::make_unique<Light>(guid, type, lightName);
    Light* ptr = obj.get();

    m_ObjectsByGuid[guid] = ptr;
    m_Lights.push_back(std::move(obj));
    m_RootDisplayOrder.push_back(guid);

    m_Dirty = true;
    return ptr;
}

ParticleEmitter* EditorWorld::CreateParticleEmitter(const std::string& name) {
    uint64_t guid = GenerateGuid();
    auto obj = std::make_unique<ParticleEmitter>(guid, name);
    ParticleEmitter* ptr = obj.get();

    m_ObjectsByGuid[guid] = ptr;
    m_ParticleEmitters.push_back(std::move(obj));
    m_RootDisplayOrder.push_back(guid);

    m_Dirty = true;
    return ptr;
}

TriggerVolume* EditorWorld::CreateTriggerVolume(const std::string& name) {
    uint64_t guid = GenerateGuid();
    auto obj = std::make_unique<TriggerVolume>(guid, name);
    TriggerVolume* ptr = obj.get();

    m_ObjectsByGuid[guid] = ptr;
    m_TriggerVolumes.push_back(std::move(obj));
    m_RootDisplayOrder.push_back(guid);

    m_Dirty = true;
    return ptr;
}

InstancePortal* EditorWorld::CreateInstancePortal(const std::string& name) {
    uint64_t guid = GenerateGuid();
    auto obj = std::make_unique<InstancePortal>(guid, name);
    InstancePortal* ptr = obj.get();

    m_ObjectsByGuid[guid] = ptr;
    m_InstancePortals.push_back(std::move(obj));
    m_RootDisplayOrder.push_back(guid);

    m_Dirty = true;
    return ptr;
}

GroupObject* EditorWorld::CreateGroup(const std::string& name) {
    uint64_t guid = GenerateGuid();
    auto obj = std::make_unique<GroupObject>(guid, name);
    GroupObject* ptr = obj.get();

    m_ObjectsByGuid[guid] = ptr;
    m_Groups.push_back(std::move(obj));
    m_RootDisplayOrder.push_back(guid);

    m_Dirty = true;
    return ptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// OBJECT ACCESS
// ─────────────────────────────────────────────────────────────────────────────

WorldObject* EditorWorld::GetObject(uint64_t guid) {
    auto it = m_ObjectsByGuid.find(guid);
    return it != m_ObjectsByGuid.end() ? it->second : nullptr;
}

const WorldObject* EditorWorld::GetObject(uint64_t guid) const {
    auto it = m_ObjectsByGuid.find(guid);
    return it != m_ObjectsByGuid.end() ? it->second : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// OBJECT ITERATION
// ─────────────────────────────────────────────────────────────────────────────

void EditorWorld::ForEachObject(const std::function<void(WorldObject*)>& callback) {
    for (auto& obj : m_StaticObjects) callback(obj.get());
    for (auto& obj : m_SpawnPoints) callback(obj.get());
    for (auto& obj : m_Lights) callback(obj.get());
    for (auto& obj : m_ParticleEmitters) callback(obj.get());
    for (auto& obj : m_TriggerVolumes) callback(obj.get());
    for (auto& obj : m_InstancePortals) callback(obj.get());
    for (auto& obj : m_Groups) callback(obj.get());
}

void EditorWorld::ForEachObject(const std::function<void(const WorldObject*)>& callback) const {
    for (const auto& obj : m_StaticObjects) callback(obj.get());
    for (const auto& obj : m_SpawnPoints) callback(obj.get());
    for (const auto& obj : m_Lights) callback(obj.get());
    for (const auto& obj : m_ParticleEmitters) callback(obj.get());
    for (const auto& obj : m_TriggerVolumes) callback(obj.get());
    for (const auto& obj : m_InstancePortals) callback(obj.get());
    for (const auto& obj : m_Groups) callback(obj.get());
}

size_t EditorWorld::GetObjectCount() const {
    return m_StaticObjects.size() +
           m_SpawnPoints.size() +
           m_Lights.size() +
           m_ParticleEmitters.size() +
           m_TriggerVolumes.size() +
           m_InstancePortals.size() +
           m_Groups.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// OBJECT DELETION
// ─────────────────────────────────────────────────────────────────────────────

template<typename T>
void EditorWorld::RemoveFromVector(std::vector<std::unique_ptr<T>>& vec, uint64_t guid) {
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [guid](const std::unique_ptr<T>& obj) { return obj->GetGuid() == guid; }),
        vec.end()
    );
}

void EditorWorld::DeleteObject(uint64_t guid) {
    WorldObject* obj = GetObject(guid);
    if (!obj) return;

    // Remove from selection
    Deselect(obj);

    // Remove from root display order (if it's a root object)
    if (!obj->HasParent()) {
        m_RootDisplayOrder.erase(
            std::remove(m_RootDisplayOrder.begin(), m_RootDisplayOrder.end(), guid),
            m_RootDisplayOrder.end()
        );
    }

    // If this object has a parent, remove it from parent's children list
    if (obj->HasParent()) {
        if (GroupObject* parent = GetObjectAs<GroupObject>(obj->GetParentGuid())) {
            parent->RemoveChild(guid);
        }
    }

    // If this is a group, unparent all children before deleting
    if (obj->GetObjectType() == WorldObjectType::GROUP) {
        GroupObject* group = static_cast<GroupObject*>(obj);
        for (uint64_t childGuid : group->GetChildren()) {
            if (WorldObject* child = GetObject(childGuid)) {
                child->SetParent(0);
                // Add children back to root display order
                m_RootDisplayOrder.push_back(childGuid);
            }
        }
    }

    // Remove from type-specific vector
    switch (obj->GetObjectType()) {
        case WorldObjectType::STATIC_OBJECT:
            RemoveFromVector(m_StaticObjects, guid);
            break;
        case WorldObjectType::SPAWN_POINT:
            RemoveFromVector(m_SpawnPoints, guid);
            break;
        case WorldObjectType::LIGHT:
            RemoveFromVector(m_Lights, guid);
            break;
        case WorldObjectType::PARTICLE_EMITTER:
            RemoveFromVector(m_ParticleEmitters, guid);
            break;
        case WorldObjectType::TRIGGER_VOLUME:
            RemoveFromVector(m_TriggerVolumes, guid);
            break;
        case WorldObjectType::INSTANCE_PORTAL:
            RemoveFromVector(m_InstancePortals, guid);
            break;
        case WorldObjectType::GROUP:
            RemoveFromVector(m_Groups, guid);
            break;
        default:
            break;
    }

    // Remove from GUID lookup
    m_ObjectsByGuid.erase(guid);

    m_Dirty = true;
}

void EditorWorld::DeleteSelected() {
    // Collect GUIDs first (since deletion modifies selection)
    std::vector<uint64_t> guidsToDelete;
    for (WorldObject* obj : m_SelectedObjects) {
        guidsToDelete.push_back(obj->GetGuid());
    }

    for (uint64_t guid : guidsToDelete) {
        DeleteObject(guid);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SELECTION
// ─────────────────────────────────────────────────────────────────────────────

void EditorWorld::Select(WorldObject* object, bool addToSelection) {
    if (!object) return;

    if (!addToSelection) {
        DeselectAll();
    }

    if (!IsSelected(object)) {
        object->SetSelected(true);
        m_SelectedObjects.push_back(object);
    }
}

void EditorWorld::SelectByGuid(uint64_t guid, bool addToSelection) {
    Select(GetObject(guid), addToSelection);
}

void EditorWorld::Deselect(WorldObject* object) {
    if (!object) return;

    object->SetSelected(false);
    m_SelectedObjects.erase(
        std::remove(m_SelectedObjects.begin(), m_SelectedObjects.end(), object),
        m_SelectedObjects.end()
    );
}

void EditorWorld::DeselectAll() {
    for (WorldObject* obj : m_SelectedObjects) {
        obj->SetSelected(false);
    }
    m_SelectedObjects.clear();
    m_SelectedMeshIndex = -1;  // Clear mesh selection
}

void EditorWorld::SelectAll() {
    DeselectAll();
    ForEachObject([this](WorldObject* obj) {
        if (obj->IsVisible() && !obj->IsLocked()) {
            obj->SetSelected(true);
            m_SelectedObjects.push_back(obj);
        }
    });
}

bool EditorWorld::IsSelected(const WorldObject* object) const {
    return std::find(m_SelectedObjects.begin(), m_SelectedObjects.end(), object)
           != m_SelectedObjects.end();
}

WorldObject* EditorWorld::GetPrimarySelection() const {
    return m_SelectedObjects.empty() ? nullptr : m_SelectedObjects.front();
}

// ─────────────────────────────────────────────────────────────────────────────
// VISIBILITY FILTERS
// ─────────────────────────────────────────────────────────────────────────────

void EditorWorld::SetTypeVisible(WorldObjectType type, bool visible) {
    m_TypeVisibility[type] = visible;

    // Update visibility of all objects of this type
    ForEachObject([type, visible](WorldObject* obj) {
        if (obj->GetObjectType() == type) {
            obj->SetVisible(visible);
        }
    });
}

bool EditorWorld::IsTypeVisible(WorldObjectType type) const {
    auto it = m_TypeVisibility.find(type);
    return it != m_TypeVisibility.end() ? it->second : true;
}

// ─────────────────────────────────────────────────────────────────────────────
// CLIPBOARD
// ─────────────────────────────────────────────────────────────────────────────

void EditorWorld::Copy() {
    m_Clipboard.clear();
    for (WorldObject* obj : m_SelectedObjects) {
        // Clone with temporary GUID (will get real GUID on paste)
        auto clone = obj->Clone(0);
        m_Clipboard.push_back(std::move(clone));
    }
}

void EditorWorld::Paste() {
    if (m_Clipboard.empty()) return;

    DeselectAll();

    for (const auto& clipObj : m_Clipboard) {
        // Clone again with new GUID
        uint64_t newGuid = GenerateGuid();
        auto newObj = clipObj->Clone(newGuid);

        // Offset position so it's not exactly overlapping
        glm::vec3 pos = newObj->GetPosition();
        newObj->SetPosition(pos + glm::vec3(1.0f, 0.0f, 1.0f));

        // Add to world and select
        WorldObject* ptr = AddObject(std::move(newObj));
        if (ptr) {
            ptr->SetSelected(true);
            m_SelectedObjects.push_back(ptr);
        }
    }

    m_Dirty = true;
}

void EditorWorld::CopyMesh() {
    std::cout << "[CopyMesh] HasMeshSelected=" << HasMeshSelected()
              << " MeshName='" << m_SelectedMeshName << "'" << std::endl;

    if (!HasMeshSelected() || m_SelectedMeshName.empty()) {
        std::cout << "[CopyMesh] FAILED - no mesh selected or name empty" << std::endl;
        return;
    }

    WorldObject* selection = GetPrimarySelection();
    if (selection && selection->GetObjectType() == WorldObjectType::STATIC_OBJECT) {
        StaticObject* staticObj = static_cast<StaticObject*>(selection);
        const MeshMaterial* mat = staticObj->GetMeshMaterial(m_SelectedMeshName);
        if (mat) {
            m_MeshClipboard = *mat;
            std::cout << "[CopyMesh] Copied existing material for '" << m_SelectedMeshName << "'" << std::endl;
        } else {
            // No custom material yet - copy default values
            m_MeshClipboard = MeshMaterial();
            std::cout << "[CopyMesh] Copied default material for '" << m_SelectedMeshName << "'" << std::endl;
        }
        m_HasMeshClipboard = true;
    } else {
        std::cout << "[CopyMesh] FAILED - selection is not a StaticObject" << std::endl;
    }
}

void EditorWorld::CopySingleMesh() {
    std::cout << "[CopySingleMesh] MeshName='" << m_SelectedMeshName << "'" << std::endl;

    if (!HasMeshSelected() || m_SelectedMeshName.empty()) {
        std::cout << "[CopySingleMesh] FAILED - no mesh selected" << std::endl;
        return;
    }

    WorldObject* selection = GetPrimarySelection();
    if (!selection || selection->GetObjectType() != WorldObjectType::STATIC_OBJECT) {
        std::cout << "[CopySingleMesh] FAILED - selection is not a StaticObject" << std::endl;
        return;
    }

    // Clone the object
    auto clone = selection->Clone(0);
    StaticObject* clonedStatic = static_cast<StaticObject*>(clone.get());

    // Hide all meshes except the selected one
    // First, get existing materials and set them all to hidden
    auto existingMaterials = clonedStatic->GetAllMeshMaterials();
    for (auto& pair : existingMaterials) {
        MeshMaterial& mat = clonedStatic->GetOrCreateMeshMaterial(pair.first);
        mat.visible = (pair.first == m_SelectedMeshName);
    }

    // Make sure the selected mesh is visible (create material if needed)
    MeshMaterial& selectedMat = clonedStatic->GetOrCreateMeshMaterial(m_SelectedMeshName);
    selectedMat.visible = true;

    // Store in clipboard
    m_Clipboard.clear();
    m_Clipboard.push_back(std::move(clone));

    std::cout << "[CopySingleMesh] Copied object with only mesh '" << m_SelectedMeshName << "' visible" << std::endl;
}

void EditorWorld::PasteMesh() {
    if (!m_HasMeshClipboard || !HasMeshSelected() || m_SelectedMeshName.empty()) return;

    WorldObject* selection = GetPrimarySelection();
    if (selection && selection->GetObjectType() == WorldObjectType::STATIC_OBJECT) {
        StaticObject* staticObj = static_cast<StaticObject*>(selection);
        MeshMaterial& mat = staticObj->GetOrCreateMeshMaterial(m_SelectedMeshName);
        // Copy transform data (not texture paths - those are mesh-specific)
        mat.positionOffset = m_MeshClipboard.positionOffset;
        mat.rotationOffset = m_MeshClipboard.rotationOffset;
        mat.scaleMultiplier = m_MeshClipboard.scaleMultiplier;
        m_Dirty = true;
    }
}

void EditorWorld::Duplicate() {
    if (m_SelectedObjects.empty()) return;

    // Store current clipboard
    auto oldClipboard = std::move(m_Clipboard);
    m_Clipboard.clear();

    // Copy current selection
    Copy();

    // Paste immediately
    Paste();

    // Restore old clipboard
    m_Clipboard = std::move(oldClipboard);
}

WorldObject* EditorWorld::AddObject(std::unique_ptr<WorldObject> object) {
    if (!object) return nullptr;

    WorldObject* ptr = object.get();
    uint64_t guid = object->GetGuid();

    // Add to type-specific vector
    switch (object->GetObjectType()) {
        case WorldObjectType::STATIC_OBJECT: {
            auto* staticObj = static_cast<StaticObject*>(object.release());
            m_StaticObjects.push_back(std::unique_ptr<StaticObject>(staticObj));
            break;
        }
        case WorldObjectType::SPAWN_POINT: {
            auto* spawnPoint = static_cast<SpawnPoint*>(object.release());
            m_SpawnPoints.push_back(std::unique_ptr<SpawnPoint>(spawnPoint));
            break;
        }
        case WorldObjectType::LIGHT: {
            auto* light = static_cast<Light*>(object.release());
            m_Lights.push_back(std::unique_ptr<Light>(light));
            break;
        }
        case WorldObjectType::PARTICLE_EMITTER: {
            auto* emitter = static_cast<ParticleEmitter*>(object.release());
            m_ParticleEmitters.push_back(std::unique_ptr<ParticleEmitter>(emitter));
            break;
        }
        case WorldObjectType::TRIGGER_VOLUME: {
            auto* trigger = static_cast<TriggerVolume*>(object.release());
            m_TriggerVolumes.push_back(std::unique_ptr<TriggerVolume>(trigger));
            break;
        }
        case WorldObjectType::INSTANCE_PORTAL: {
            auto* portal = static_cast<InstancePortal*>(object.release());
            m_InstancePortals.push_back(std::unique_ptr<InstancePortal>(portal));
            break;
        }
        case WorldObjectType::GROUP: {
            auto* group = static_cast<GroupObject*>(object.release());
            m_Groups.push_back(std::unique_ptr<GroupObject>(group));
            break;
        }
        default:
            return nullptr;
    }

    // Add to GUID lookup
    m_ObjectsByGuid[guid] = ptr;

    m_Dirty = true;
    return ptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// HIERARCHY (Parent-Child)
// ─────────────────────────────────────────────────────────────────────────────

void EditorWorld::SetParent(WorldObject* child, GroupObject* parent) {
    if (!child) return;

    uint64_t childGuid = child->GetGuid();
    bool wasRoot = !child->HasParent();

    // Remove from old parent
    if (child->HasParent()) {
        if (GroupObject* oldParent = GetObjectAs<GroupObject>(child->GetParentGuid())) {
            oldParent->RemoveChild(childGuid);
        }
    }

    if (parent) {
        // Add to new parent
        child->SetParent(parent->GetGuid());
        parent->AddChild(childGuid);

        // Remove from root display order (moving from root to child)
        if (wasRoot) {
            m_RootDisplayOrder.erase(
                std::remove(m_RootDisplayOrder.begin(), m_RootDisplayOrder.end(), childGuid),
                m_RootDisplayOrder.end()
            );
        }
    } else {
        // No parent (root level)
        child->SetParent(0);

        // Add to root display order if wasn't already root
        // (Unparent will handle inserting at specific position if needed)
        if (!wasRoot) {
            // Check if already in display order
            if (std::find(m_RootDisplayOrder.begin(), m_RootDisplayOrder.end(), childGuid) == m_RootDisplayOrder.end()) {
                m_RootDisplayOrder.push_back(childGuid);
            }
        }
    }

    m_Dirty = true;
}

void EditorWorld::Unparent(WorldObject* child) {
    if (!child || !child->HasParent()) return;

    // Calculate world position before unparenting
    glm::mat4 worldMatrix = GetWorldMatrix(child);
    glm::vec3 worldPos = glm::vec3(worldMatrix[3]);

    // Unparent
    SetParent(child, nullptr);

    // Restore world position (now it becomes the local position since no parent)
    child->SetPosition(worldPos);
}

GroupObject* EditorWorld::GetParentGroup(WorldObject* object) {
    if (!object || !object->HasParent()) return nullptr;
    return GetObjectAs<GroupObject>(object->GetParentGuid());
}

glm::mat4 EditorWorld::GetWorldMatrix(const WorldObject* object) const {
    if (!object) return glm::mat4(1.0f);

    glm::mat4 localMatrix = object->GetLocalMatrix();

    if (object->HasParent()) {
        const WorldObject* parent = GetObject(object->GetParentGuid());
        if (parent) {
            // Recursively get parent's world matrix
            return GetWorldMatrix(parent) * localMatrix;
        }
    }

    return localMatrix;
}

std::vector<WorldObject*> EditorWorld::GetRootObjects() {
    std::vector<WorldObject*> roots;
    roots.reserve(m_RootDisplayOrder.size());
    for (uint64_t guid : m_RootDisplayOrder) {
        if (WorldObject* obj = GetObject(guid)) {
            roots.push_back(obj);
        }
    }
    return roots;
}

void EditorWorld::MoveInDisplayOrder(uint64_t guid, size_t newIndex) {
    // Find current position
    auto it = std::find(m_RootDisplayOrder.begin(), m_RootDisplayOrder.end(), guid);
    if (it == m_RootDisplayOrder.end()) return;

    // Remove from current position
    m_RootDisplayOrder.erase(it);

    // Insert at new position
    if (newIndex >= m_RootDisplayOrder.size()) {
        m_RootDisplayOrder.push_back(guid);
    } else {
        m_RootDisplayOrder.insert(m_RootDisplayOrder.begin() + newIndex, guid);
    }

    m_Dirty = true;
}

void EditorWorld::MoveChildInGroup(GroupObject* group, uint64_t childGuid, size_t newIndex) {
    if (!group) return;
    group->MoveChild(childGuid, newIndex);
    m_Dirty = true;
}

void EditorWorld::UnparentAtIndex(WorldObject* child, size_t rootIndex) {
    if (!child || !child->HasParent()) return;

    uint64_t childGuid = child->GetGuid();

    // Calculate world position before unparenting
    glm::mat4 worldMatrix = GetWorldMatrix(child);
    glm::vec3 worldPos = glm::vec3(worldMatrix[3]);

    // Remove from old parent
    if (GroupObject* oldParent = GetObjectAs<GroupObject>(child->GetParentGuid())) {
        oldParent->RemoveChild(childGuid);
    }
    child->SetParent(0);

    // Insert at specific position in root display order
    if (rootIndex >= m_RootDisplayOrder.size()) {
        m_RootDisplayOrder.push_back(childGuid);
    } else {
        m_RootDisplayOrder.insert(m_RootDisplayOrder.begin() + rootIndex, childGuid);
    }

    // Restore world position
    child->SetPosition(worldPos);
    m_Dirty = true;
}

void EditorWorld::SetParentAtIndex(WorldObject* child, GroupObject* parent, size_t childIndex) {
    if (!child || !parent) return;

    uint64_t childGuid = child->GetGuid();
    bool wasRoot = !child->HasParent();

    // Remove from old parent
    if (child->HasParent()) {
        if (GroupObject* oldParent = GetObjectAs<GroupObject>(child->GetParentGuid())) {
            oldParent->RemoveChild(childGuid);
        }
    }

    // Remove from root display order if was root
    if (wasRoot) {
        m_RootDisplayOrder.erase(
            std::remove(m_RootDisplayOrder.begin(), m_RootDisplayOrder.end(), childGuid),
            m_RootDisplayOrder.end()
        );
    }

    // Set new parent and insert at specific position
    child->SetParent(parent->GetGuid());
    parent->InsertChildAt(childGuid, childIndex);

    m_Dirty = true;
}

GroupObject* EditorWorld::GroupSelected(const std::string& name) {
    if (m_SelectedObjects.empty()) return nullptr;

    // Calculate center of selection for group position
    glm::vec3 center(0.0f);
    for (WorldObject* obj : m_SelectedObjects) {
        center += obj->GetPosition();
    }
    center /= static_cast<float>(m_SelectedObjects.size());

    // Create the group
    GroupObject* group = CreateGroup(name);
    group->SetPosition(center);

    // Add all selected objects as children
    // Adjust their positions to be relative to group
    for (WorldObject* obj : m_SelectedObjects) {
        // Make position relative to group
        glm::vec3 relativePos = obj->GetPosition() - center;
        obj->SetPosition(relativePos);

        // Set parent
        SetParent(obj, group);
    }

    // Select the new group
    DeselectAll();
    Select(group);

    return group;
}

void EditorWorld::UngroupSelected() {
    std::vector<GroupObject*> groupsToUngroup;

    // Find all selected groups
    for (WorldObject* obj : m_SelectedObjects) {
        if (obj->GetObjectType() == WorldObjectType::GROUP) {
            groupsToUngroup.push_back(static_cast<GroupObject*>(obj));
        }
    }

    if (groupsToUngroup.empty()) return;

    std::vector<WorldObject*> newSelection;

    for (GroupObject* group : groupsToUngroup) {
        glm::mat4 groupWorldMatrix = GetWorldMatrix(group);

        // Unparent children and restore their world positions
        std::vector<uint64_t> childGuids = group->GetChildren();  // Copy list
        for (uint64_t childGuid : childGuids) {
            WorldObject* child = GetObject(childGuid);
            if (child) {
                // Calculate child's world position
                glm::mat4 childWorldMatrix = groupWorldMatrix * child->GetLocalMatrix();
                glm::vec3 worldPos = glm::vec3(childWorldMatrix[3]);

                // Unparent
                Unparent(child);

                // Set world position
                child->SetPosition(worldPos);

                newSelection.push_back(child);
            }
        }

        // Delete the empty group
        DeleteObject(group->GetGuid());
    }

    // Select the ungrouped objects
    DeselectAll();
    for (WorldObject* obj : newSelection) {
        Select(obj, true);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PRIVATE
// ─────────────────────────────────────────────────────────────────────────────

uint64_t EditorWorld::GenerateGuid() {
    return m_NextGuid++;
}

} // namespace MMO
