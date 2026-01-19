#include "InspectorPanel.h"
#include "ViewportPanel.h"
#include "World/EditorWorld.h"
#include "World/WorldTypes.h"
#include <Graphics/Model.h>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <algorithm>

namespace MMO {

// Texture type names for UI
static const char* TextureTypeNames[] = {
    "Diffuse",
    "Normal",
    "Specular",
    "Metallic",
    "Emissive",
    "AO"
};

void InspectorPanel::Init(EditorWorld* world, ViewportPanel* viewport) {
    m_World = world;
    m_Viewport = viewport;
}

void InspectorPanel::OnImGuiRender() {
    ImGui::Begin("Inspector");

    if (!m_World) {
        ImGui::Text("No world loaded");
        ImGui::End();
        return;
    }

    WorldObject* selection = m_World->GetPrimarySelection();
    if (!selection) {
        ImGui::TextDisabled("No object selected");
        ImGui::End();
        return;
    }

    // Object name (editable)
    char nameBuf[256];
    strncpy(nameBuf, selection->GetName().c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';

    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
        selection->SetName(nameBuf);
    }

    // Object type (read-only)
    const char* typeStr = "Unknown";
    switch (selection->GetObjectType()) {
        case WorldObjectType::STATIC_OBJECT: typeStr = "Static Object"; break;
        case WorldObjectType::SPAWN_POINT: typeStr = "Spawn Point"; break;
        case WorldObjectType::LIGHT: typeStr = "Light"; break;
        case WorldObjectType::PARTICLE_EMITTER: typeStr = "Particle Emitter"; break;
        case WorldObjectType::TRIGGER_VOLUME: typeStr = "Trigger Volume"; break;
        case WorldObjectType::INSTANCE_PORTAL: typeStr = "Instance Portal"; break;
        default: break;
    }
    ImGui::Text("Type: %s", typeStr);
    ImGui::Text("GUID: %llu", static_cast<unsigned long long>(selection->GetGuid()));

    ImGui::Separator();

    // Transform section
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderTransform(selection);
    }

    // Type-specific properties
    switch (selection->GetObjectType()) {
        case WorldObjectType::STATIC_OBJECT:
            if (ImGui::CollapsingHeader("Static Object", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderStaticObjectProperties(static_cast<StaticObject*>(selection));
            }
            break;
        case WorldObjectType::SPAWN_POINT:
            if (ImGui::CollapsingHeader("Spawn Point", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderSpawnPointProperties(static_cast<SpawnPoint*>(selection));
            }
            break;
        case WorldObjectType::LIGHT:
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderLightProperties(static_cast<Light*>(selection));
            }
            break;
        case WorldObjectType::PARTICLE_EMITTER:
            if (ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderParticleEmitterProperties(static_cast<ParticleEmitter*>(selection));
            }
            break;
        case WorldObjectType::TRIGGER_VOLUME:
            if (ImGui::CollapsingHeader("Trigger Volume", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderTriggerVolumeProperties(static_cast<TriggerVolume*>(selection));
            }
            break;
        case WorldObjectType::INSTANCE_PORTAL:
            if (ImGui::CollapsingHeader("Instance Portal", ImGuiTreeNodeFlags_DefaultOpen)) {
                RenderInstancePortalProperties(static_cast<InstancePortal*>(selection));
            }
            break;
        default:
            break;
    }

    ImGui::End();

    // Render file browser popup if open
    RenderFileBrowserPopup();
}

void InspectorPanel::RenderTransform(WorldObject* object) {
    // Position
    glm::vec3 pos = object->GetPosition();
    if (ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f)) {
        object->SetPosition(pos);
    }

    // Rotation (as euler angles for easier editing)
    glm::vec3 euler = object->GetEulerAngles();
    if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 1.0f)) {
        object->SetEulerAngles(euler);
    }

    // Scale
    float scale = object->GetScale();
    if (ImGui::DragFloat("Scale", &scale, 0.01f, 0.01f, 100.0f)) {
        object->SetScale(scale);
    }
}

void InspectorPanel::RenderStaticObjectProperties(StaticObject* object) {
    // Model section
    ImGui::Text("Model");
    ImGui::Indent();

    std::string oldModelPath = object->GetModelPath();
    RenderPathInput("Model", object->GetModelPath(),
        [object](const std::string& path) {
            object->SetModelPath(path);
            object->ClearMeshMaterials();  // Clear materials when model changes
        },
        ".obj,.fbx,.gltf,.glb");

    // Model ID
    uint32_t modelId = object->GetModelId();
    if (ImGui::InputScalar("Model ID", ImGuiDataType_U32, &modelId)) {
        object->SetModelId(modelId);
    }

    ImGui::Unindent();
    ImGui::Spacing();

    // Per-mesh materials section
    const std::string& modelPath = object->GetModelPath();
    if (!modelPath.empty() && m_Viewport) {
        Onyx::Model* model = m_Viewport->GetModel(modelPath);
        if (model && !model->GetMeshes().empty()) {
            int selectedMeshIdx = m_World->GetSelectedMeshIndex();

            // Show currently selected mesh name
            if (selectedMeshIdx >= 0 && selectedMeshIdx < static_cast<int>(model->GetMeshes().size())) {
                auto& selectedMesh = model->GetMeshes()[selectedMeshIdx];
                std::string selectedName = selectedMesh.m_Name.empty()
                    ? ("Mesh " + std::to_string(selectedMeshIdx))
                    : selectedMesh.m_Name;
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Selected: %s", selectedName.c_str());
                if (ImGui::SmallButton("Deselect Mesh")) {
                    m_World->DeselectMesh();
                }
                ImGui::Separator();
            }

            if (ImGui::TreeNodeEx("Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& meshes = model->GetMeshes();
                for (size_t i = 0; i < meshes.size(); i++) {
                    auto& mesh = meshes[i];
                    std::string meshName = mesh.m_Name.empty() ? ("Mesh " + std::to_string(i)) : mesh.m_Name;

                    ImGui::PushID(static_cast<int>(i));

                    // Highlight selected mesh
                    bool isSelected = (static_cast<int>(i) == selectedMeshIdx);
                    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
                    if (isSelected) {
                        nodeFlags |= ImGuiTreeNodeFlags_Selected;
                    }

                    bool nodeOpen = ImGui::TreeNodeEx(meshName.c_str(), nodeFlags);

                    // Click to select mesh
                    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                        m_World->SelectMesh(static_cast<int>(i));
                    }

                    if (nodeOpen) {
                        // Get or create material for this mesh
                        MeshMaterial& material = object->GetOrCreateMeshMaterial(meshName);

                        // Per-mesh transform
                        if (ImGui::TreeNode("Transform Offset")) {
                            if (ImGui::DragFloat3("Position", glm::value_ptr(material.positionOffset), 0.1f)) {
                                // Transform changed
                            }
                            if (ImGui::DragFloat3("Rotation", glm::value_ptr(material.rotationOffset), 1.0f)) {
                                // Transform changed
                            }
                            if (ImGui::DragFloat("Scale", &material.scaleMultiplier, 0.01f, 0.01f, 10.0f)) {
                                // Transform changed
                            }
                            if (ImGui::Button("Reset Transform")) {
                                material.positionOffset = glm::vec3(0.0f);
                                material.rotationOffset = glm::vec3(0.0f);
                                material.scaleMultiplier = 1.0f;
                            }
                            ImGui::TreePop();
                        }

                        // Textures
                        if (ImGui::TreeNode("Textures")) {
                            // Albedo texture
                            RenderPathInput("Albedo", material.albedoPath,
                                [&material](const std::string& path) { material.albedoPath = path; },
                                ".png,.jpg,.jpeg,.tga,.bmp");

                            // Normal texture
                            RenderPathInput("Normal", material.normalPath,
                                [&material](const std::string& path) { material.normalPath = path; },
                                ".png,.jpg,.jpeg,.tga,.bmp");
                            ImGui::TreePop();
                        }

                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }
    }

    // Fallback textures (used when no per-mesh material is set)
    if (ImGui::TreeNode("Default Textures")) {
        ImGui::TextDisabled("Used for meshes without specific materials");
        for (int i = 0; i < static_cast<int>(TextureType::COUNT); i++) {
            TextureType type = static_cast<TextureType>(i);
            ImGui::PushID(i);

            RenderPathInput(TextureTypeNames[i], object->GetTexturePath(type),
                [object, type](const std::string& path) { object->SetTexturePath(type, path); },
                ".png,.jpg,.jpeg,.tga,.bmp");

            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    ImGui::Spacing();

    // Rendering options
    ImGui::Text("Rendering");
    ImGui::Indent();

    bool castsShadow = object->CastsShadow();
    if (ImGui::Checkbox("Casts Shadow", &castsShadow)) {
        object->SetCastsShadow(castsShadow);
    }

    bool receivesLightmap = object->ReceivesLightmap();
    if (ImGui::Checkbox("Receives Lightmap", &receivesLightmap)) {
        object->SetReceivesLightmap(receivesLightmap);
    }

    ImGui::Unindent();
}

void InspectorPanel::RenderSpawnPointProperties(SpawnPoint* object) {
    // Creature template ID
    uint32_t templateId = object->GetCreatureTemplateId();
    if (ImGui::InputScalar("Creature Template ID", ImGuiDataType_U32, &templateId)) {
        object->SetCreatureTemplateId(templateId);
    }

    ImGui::Spacing();

    // Preview Model section
    if (ImGui::TreeNode("Preview Model")) {
        RenderPathInput("Model", object->GetModelPath(),
            [object](const std::string& path) { object->SetModelPath(path); },
            ".obj,.fbx,.gltf,.glb");

        // Textures for preview
        if (ImGui::TreeNode("Textures")) {
            // Only show diffuse and normal for spawn point preview
            RenderPathInput("Diffuse", object->GetDiffuseTexture(),
                [object](const std::string& path) { object->SetDiffuseTexture(path); },
                ".png,.jpg,.jpeg,.tga,.bmp");

            RenderPathInput("Normal", object->GetNormalTexture(),
                [object](const std::string& path) { object->SetNormalTexture(path); },
                ".png,.jpg,.jpeg,.tga,.bmp");

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    ImGui::Spacing();

    // Spawn behavior
    ImGui::Text("Spawn Behavior");
    ImGui::Indent();

    // Respawn time
    float respawnTime = object->GetRespawnTime();
    if (ImGui::DragFloat("Respawn Time (s)", &respawnTime, 1.0f, 0.0f, 3600.0f)) {
        object->SetRespawnTime(respawnTime);
    }

    // Wander radius
    float wanderRadius = object->GetWanderRadius();
    if (ImGui::DragFloat("Wander Radius", &wanderRadius, 0.5f, 0.0f, 100.0f)) {
        object->SetWanderRadius(wanderRadius);
    }

    // Max count
    uint32_t maxCount = object->GetMaxCount();
    if (ImGui::InputScalar("Max Count", ImGuiDataType_U32, &maxCount)) {
        object->SetMaxCount(maxCount);
    }

    ImGui::Unindent();
}

void InspectorPanel::RenderLightProperties(Light* object) {
    // Light type
    const char* lightTypes[] = { "Point", "Spot", "Directional" };
    int currentType = static_cast<int>(object->GetLightType());
    if (ImGui::Combo("Light Type", &currentType, lightTypes, 3)) {
        object->SetLightType(static_cast<LightType>(currentType));
    }

    // Color
    glm::vec3 color = object->GetColor();
    if (ImGui::ColorEdit3("Color", glm::value_ptr(color))) {
        object->SetColor(color);
    }

    // Intensity
    float intensity = object->GetIntensity();
    if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f)) {
        object->SetIntensity(intensity);
    }

    // Radius (for point and spot lights)
    LightType type = object->GetLightType();
    if (type == LightType::POINT || type == LightType::SPOT) {
        float radius = object->GetRadius();
        if (ImGui::DragFloat("Radius", &radius, 0.5f, 0.0f, 1000.0f)) {
            object->SetRadius(radius);
        }
    }

    // Angles (for spot lights)
    if (type == LightType::SPOT) {
        float innerAngle = object->GetInnerAngle();
        float outerAngle = object->GetOuterAngle();

        if (ImGui::DragFloat("Inner Angle", &innerAngle, 1.0f, 0.0f, outerAngle)) {
            object->SetInnerAngle(innerAngle);
        }
        if (ImGui::DragFloat("Outer Angle", &outerAngle, 1.0f, innerAngle, 180.0f)) {
            object->SetOuterAngle(outerAngle);
        }
    }

    // Shadow options
    bool castsShadows = object->CastsShadows();
    if (ImGui::Checkbox("Casts Shadows", &castsShadows)) {
        object->SetCastsShadows(castsShadows);
    }

    bool bakedOnly = object->IsBakedOnly();
    if (ImGui::Checkbox("Baked Only", &bakedOnly)) {
        object->SetBakedOnly(bakedOnly);
    }
}

void InspectorPanel::RenderParticleEmitterProperties(ParticleEmitter* object) {
    // Effect ID
    uint32_t effectId = object->GetEffectId();
    if (ImGui::InputScalar("Effect ID", ImGuiDataType_U32, &effectId)) {
        object->SetEffectId(effectId);
    }

    // Effect path
    char effectPath[256];
    strncpy(effectPath, object->GetEffectPath().c_str(), sizeof(effectPath) - 1);
    effectPath[sizeof(effectPath) - 1] = '\0';

    if (ImGui::InputText("Effect Path", effectPath, sizeof(effectPath))) {
        object->SetEffectPath(effectPath);
    }

    // Auto play
    bool autoPlay = object->IsAutoPlay();
    if (ImGui::Checkbox("Auto Play", &autoPlay)) {
        object->SetAutoPlay(autoPlay);
    }

    // Loop
    bool loop = object->IsLooping();
    if (ImGui::Checkbox("Loop", &loop)) {
        object->SetLooping(loop);
    }
}

void InspectorPanel::RenderTriggerVolumeProperties(TriggerVolume* object) {
    // Shape type
    const char* shapeTypes[] = { "Box", "Sphere", "Capsule" };
    int currentShape = static_cast<int>(object->GetShape());
    if (ImGui::Combo("Shape", &currentShape, shapeTypes, 3)) {
        object->SetShape(static_cast<TriggerShape>(currentShape));
    }

    // Half extents (for box)
    if (object->GetShape() == TriggerShape::BOX) {
        glm::vec3 halfExtents = object->GetHalfExtents();
        if (ImGui::DragFloat3("Half Extents", glm::value_ptr(halfExtents), 0.1f, 0.1f, 100.0f)) {
            object->SetHalfExtents(halfExtents);
        }
    }

    // Radius (for sphere/capsule)
    if (object->GetShape() == TriggerShape::SPHERE || object->GetShape() == TriggerShape::CAPSULE) {
        float radius = object->GetRadius();
        if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 100.0f)) {
            object->SetRadius(radius);
        }
    }

    // Event type
    const char* eventTypes[] = { "On Enter", "On Exit", "On Stay" };
    int currentEvent = static_cast<int>(object->GetTriggerEvent());
    if (ImGui::Combo("Event Type", &currentEvent, eventTypes, 3)) {
        object->SetTriggerEvent(static_cast<TriggerEvent>(currentEvent));
    }

    // Script name
    char scriptName[256];
    strncpy(scriptName, object->GetScriptName().c_str(), sizeof(scriptName) - 1);
    scriptName[sizeof(scriptName) - 1] = '\0';

    if (ImGui::InputText("Script Name", scriptName, sizeof(scriptName))) {
        object->SetScriptName(scriptName);
    }

    // Trigger once
    bool triggerOnce = object->IsTriggerOnce();
    if (ImGui::Checkbox("Trigger Once", &triggerOnce)) {
        object->SetTriggerOnce(triggerOnce);
    }

    // Filters
    ImGui::Text("Filters:");
    bool filterPlayers = object->TriggerPlayers();
    if (ImGui::Checkbox("Players", &filterPlayers)) {
        object->SetTriggerPlayers(filterPlayers);
    }
    ImGui::SameLine();
    bool filterCreatures = object->TriggerCreatures();
    if (ImGui::Checkbox("Creatures", &filterCreatures)) {
        object->SetTriggerCreatures(filterCreatures);
    }
}

void InspectorPanel::RenderInstancePortalProperties(InstancePortal* object) {
    // Dungeon ID
    uint32_t dungeonId = object->GetDungeonId();
    if (ImGui::InputScalar("Dungeon ID", ImGuiDataType_U32, &dungeonId)) {
        object->SetDungeonId(dungeonId);
    }

    // Dungeon name
    char dungeonName[256];
    strncpy(dungeonName, object->GetDungeonName().c_str(), sizeof(dungeonName) - 1);
    dungeonName[sizeof(dungeonName) - 1] = '\0';

    if (ImGui::InputText("Dungeon Name", dungeonName, sizeof(dungeonName))) {
        object->SetDungeonName(dungeonName);
    }

    // Exit info
    ImGui::Separator();
    ImGui::Text("Exit Location:");

    uint32_t exitMapId = object->GetExitMapId();
    if (ImGui::InputScalar("Exit Map ID", ImGuiDataType_U32, &exitMapId)) {
        object->SetExitMapId(exitMapId);
    }

    glm::vec3 exitPos = object->GetExitPosition();
    if (ImGui::DragFloat3("Exit Position", glm::value_ptr(exitPos), 0.1f)) {
        object->SetExitPosition(exitPos);
    }

    // Interaction
    float interactRadius = object->GetInteractionRadius();
    if (ImGui::DragFloat("Interaction Radius", &interactRadius, 0.1f, 0.1f, 20.0f)) {
        object->SetInteractionRadius(interactRadius);
    }

    // Requirements
    ImGui::Separator();
    ImGui::Text("Requirements:");

    uint32_t minLevel = object->GetMinLevel();
    if (ImGui::InputScalar("Min Level", ImGuiDataType_U32, &minLevel)) {
        object->SetMinLevel(minLevel);
    }

    uint32_t maxLevel = object->GetMaxLevel();
    if (ImGui::InputScalar("Max Level", ImGuiDataType_U32, &maxLevel)) {
        object->SetMaxLevel(maxLevel);
    }

    uint32_t minPlayers = object->GetMinPlayers();
    if (ImGui::InputScalar("Min Players", ImGuiDataType_U32, &minPlayers)) {
        object->SetMinPlayers(minPlayers);
    }

    uint32_t maxPlayers = object->GetMaxPlayers();
    if (ImGui::InputScalar("Max Players", ImGuiDataType_U32, &maxPlayers)) {
        object->SetMaxPlayers(maxPlayers);
    }
}

bool InspectorPanel::RenderPathInput(const char* label, const std::string& currentPath,
                                      const std::function<void(const std::string&)>& onPathChanged,
                                      const char* filter) {
    bool changed = false;

    ImGui::PushID(label);

    // Calculate widths
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float buttonWidth = 60.0f;
    float clearButtonWidth = 20.0f;
    float inputWidth = totalWidth - buttonWidth - clearButtonWidth - 8.0f;

    // Text input for path
    char pathBuf[512];
    strncpy(pathBuf, currentPath.c_str(), sizeof(pathBuf) - 1);
    pathBuf[sizeof(pathBuf) - 1] = '\0';

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGui::InputText(label, pathBuf, sizeof(pathBuf))) {
        onPathChanged(pathBuf);
        changed = true;
    }

    // Show tooltip with full path
    if (ImGui::IsItemHovered() && !currentPath.empty()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", currentPath.c_str());
        ImGui::EndTooltip();
    }

    // Drag-drop target for asset browser
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            const char* droppedPath = static_cast<const char*>(payload->Data);

            // Check if file matches filter (if filter provided)
            bool accepted = true;
            if (filter && strlen(filter) > 0) {
                std::string ext = std::filesystem::path(droppedPath).extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                // Parse filter (comma-separated extensions)
                std::string filterStr = filter;
                accepted = false;
                size_t pos = 0;
                while (pos < filterStr.size()) {
                    size_t next = filterStr.find(',', pos);
                    if (next == std::string::npos) next = filterStr.size();

                    std::string allowedExt = filterStr.substr(pos, next - pos);
                    std::transform(allowedExt.begin(), allowedExt.end(), allowedExt.begin(), ::tolower);

                    if (ext == allowedExt) {
                        accepted = true;
                        break;
                    }
                    pos = next + 1;
                }
            }

            if (accepted) {
                onPathChanged(droppedPath);
                changed = true;
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();

    // Browse button
    if (ImGui::Button("Browse", ImVec2(buttonWidth, 0))) {
        m_ShowFileBrowser = true;
        m_BrowseFilter = filter ? filter : "";
        m_BrowseCallback = onPathChanged;
    }

    ImGui::SameLine();

    // Clear button
    if (ImGui::Button("X", ImVec2(clearButtonWidth, 0))) {
        onPathChanged("");
        changed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear");
    }

    ImGui::PopID();

    return changed;
}

void InspectorPanel::RenderModelSection(const std::string& modelPath,
                                        const std::function<void(const std::string&)>& onModelChanged,
                                        const std::function<std::string(int)>& getTexture,
                                        const std::function<void(int, const std::string&)>& setTexture) {
    // Model path
    RenderPathInput("Model", modelPath, onModelChanged, ".obj,.fbx,.gltf,.glb");

    // Textures
    if (ImGui::TreeNode("Textures")) {
        for (int i = 0; i < static_cast<int>(TextureType::COUNT); i++) {
            ImGui::PushID(i);
            RenderPathInput(TextureTypeNames[i], getTexture(i),
                [setTexture, i](const std::string& path) { setTexture(i, path); },
                ".png,.jpg,.jpeg,.tga,.bmp");
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
}

void InspectorPanel::RenderFileBrowserPopup() {
    if (!m_ShowFileBrowser) return;

    // Initialize current directory if empty
    if (m_BrowseCurrentDir.empty()) {
        m_BrowseCurrentDir = "MMOGame/assets";
    }

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Select File", &m_ShowFileBrowser)) {
        // Navigation bar
        if (ImGui::Button("^")) {
            std::filesystem::path current(m_BrowseCurrentDir);
            if (current.has_parent_path() && current.parent_path() != current) {
                m_BrowseCurrentDir = current.parent_path().string();
            }
        }
        ImGui::SameLine();
        ImGui::Text("%s", m_BrowseCurrentDir.c_str());

        ImGui::Separator();

        // File filter display
        if (!m_BrowseFilter.empty()) {
            ImGui::TextDisabled("Filter: %s", m_BrowseFilter.c_str());
        }

        ImGui::Separator();

        // File list
        ImGui::BeginChild("FileList", ImVec2(0, -30), true);

        namespace fs = std::filesystem;

        if (fs::exists(m_BrowseCurrentDir)) {
            // Collect and sort entries
            std::vector<fs::directory_entry> entries;
            try {
                for (const auto& entry : fs::directory_iterator(m_BrowseCurrentDir)) {
                    entries.push_back(entry);
                }
            } catch (...) {}

            // Sort: directories first, then alphabetically
            std::sort(entries.begin(), entries.end(),
                [](const fs::directory_entry& a, const fs::directory_entry& b) {
                    if (a.is_directory() != b.is_directory()) {
                        return a.is_directory() > b.is_directory();
                    }
                    return a.path().filename() < b.path().filename();
                });

            for (const auto& entry : entries) {
                std::string name = entry.path().filename().string();

                // Skip hidden files
                if (!name.empty() && name[0] == '.') continue;

                if (entry.is_directory()) {
                    // Directory - show with folder icon
                    if (ImGui::Selectable(("[DIR] " + name).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            m_BrowseCurrentDir = entry.path().string();
                        }
                    }
                } else {
                    // File - check filter
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    bool matchesFilter = m_BrowseFilter.empty();
                    if (!matchesFilter) {
                        std::string filterLower = m_BrowseFilter;
                        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

                        size_t pos = 0;
                        while (pos < filterLower.size()) {
                            size_t next = filterLower.find(',', pos);
                            if (next == std::string::npos) next = filterLower.size();

                            std::string allowedExt = filterLower.substr(pos, next - pos);
                            if (ext == allowedExt) {
                                matchesFilter = true;
                                break;
                            }
                            pos = next + 1;
                        }
                    }

                    if (matchesFilter) {
                        if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                            if (ImGui::IsMouseDoubleClicked(0)) {
                                // Select file
                                if (m_BrowseCallback) {
                                    m_BrowseCallback(entry.path().string());
                                }
                                m_ShowFileBrowser = false;
                            }
                        }
                    } else {
                        // Show filtered files as disabled
                        ImGui::TextDisabled("%s", name.c_str());
                    }
                }
            }
        }

        ImGui::EndChild();

        // Cancel button
        if (ImGui::Button("Cancel")) {
            m_ShowFileBrowser = false;
        }
    }
    ImGui::End();

    // If window was closed
    if (!m_ShowFileBrowser) {
        m_BrowseCallback = nullptr;
    }
}

} // namespace MMO
