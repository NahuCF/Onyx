#include "InspectorPanel.h"
#include "Data/RaceClassRegistry.h"
#include "ViewportPanel.h"
#include "World/EditorWorld.h"
#include "World/WorldTypes.h"
#include <Core/Application.h>
#include <Graphics/AnimatedModel.h>
#include <Graphics/Animator.h>
#include <Graphics/AssetManager.h>
#include <Graphics/Model.h>
#include <algorithm>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <iostream>

namespace MMO {

	void InspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Inspector");

		if (!m_World)
		{
			ImGui::Text("No world loaded");
			ImGui::End();
			return;
		}

		WorldObject* selection = m_World->GetPrimarySelection();
		if (!selection)
		{
			ImGui::TextDisabled("No object selected");
			ImGui::End();
			return;
		}

		char nameBuf[256];
		strncpy(nameBuf, selection->GetName().c_str(), sizeof(nameBuf) - 1);
		nameBuf[sizeof(nameBuf) - 1] = '\0';

		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
		{
			selection->SetName(nameBuf);
		}

		const char* typeStr = "Unknown";
		switch (selection->GetObjectType())
		{
		case WorldObjectType::STATIC_OBJECT:
			typeStr = "Static Object";
			break;
		case WorldObjectType::SPAWN_POINT:
			typeStr = "Spawn Point";
			break;
		case WorldObjectType::LIGHT:
			typeStr = "Light";
			break;
		case WorldObjectType::PARTICLE_EMITTER:
			typeStr = "Particle Emitter";
			break;
		case WorldObjectType::TRIGGER_VOLUME:
			typeStr = "Trigger Volume";
			break;
		case WorldObjectType::INSTANCE_PORTAL:
			typeStr = "Instance Portal";
			break;
		case WorldObjectType::PLAYER_SPAWN:
			typeStr = "Player Spawn";
			break;
		default:
			break;
		}
		ImGui::Text("Type: %s", typeStr);
		ImGui::Text("GUID: %llu", static_cast<unsigned long long>(selection->GetGuid()));

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			RenderTransform(selection);
		}

		switch (selection->GetObjectType())
		{
		case WorldObjectType::STATIC_OBJECT:
			if (ImGui::CollapsingHeader("Static Object", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RenderStaticObjectProperties(static_cast<StaticObject*>(selection));
			}
			break;
		case WorldObjectType::SPAWN_POINT:
			if (ImGui::CollapsingHeader("Spawn Point", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RenderSpawnPointProperties(static_cast<SpawnPoint*>(selection));
			}
			break;
		case WorldObjectType::LIGHT:
			if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RenderLightProperties(static_cast<Light*>(selection));
			}
			break;
		case WorldObjectType::PARTICLE_EMITTER:
			if (ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RenderParticleEmitterProperties(static_cast<ParticleEmitter*>(selection));
			}
			break;
		case WorldObjectType::TRIGGER_VOLUME:
			if (ImGui::CollapsingHeader("Trigger Volume", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RenderTriggerVolumeProperties(static_cast<TriggerVolume*>(selection));
			}
			break;
		case WorldObjectType::INSTANCE_PORTAL:
			if (ImGui::CollapsingHeader("Instance Portal", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RenderInstancePortalProperties(static_cast<InstancePortal*>(selection));
			}
			break;
		case WorldObjectType::PLAYER_SPAWN:
			if (ImGui::CollapsingHeader("Player Spawn", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RenderPlayerSpawnProperties(static_cast<PlayerSpawn*>(selection));
			}
			break;
		default:
			break;
		}

		ImGui::End();

		RenderFileBrowserPopup();
	}

	void InspectorPanel::RenderTransform(WorldObject* object)
	{
		glm::vec3 pos = object->GetPosition();
		if (ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f))
		{
			object->SetPosition(pos);
		}

		glm::vec3 euler = object->GetEulerAngles();
		if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 1.0f))
		{
			object->SetEulerAngles(euler);
		}

		float scale = object->GetScale();
		if (ImGui::DragFloat("Scale", &scale, 0.01f, 0.01f, 100.0f))
		{
			object->SetScale(scale);
		}
	}

	void InspectorPanel::RenderStaticObjectProperties(StaticObject* object)
	{
		ImGui::Text("Model");
		ImGui::Indent();

		std::string oldModelPath = object->GetModelPath();
		RenderPathInput("Model", object->GetModelPath(), [object](const std::string& path) {
			object->SetModelPath(path);
			object->ClearMeshMaterials(); // Clear materials when model changes
		},
						".obj,.fbx,.gltf,.glb");

		uint32_t modelId = object->GetModelId();
		if (ImGui::InputScalar("Model ID", ImGuiDataType_U32, &modelId))
		{
			object->SetModelId(modelId);
		}

		ImGui::Unindent();
		ImGui::Spacing();

		const std::string& modelPath = object->GetModelPath();
		if (!modelPath.empty() && m_Viewport)
		{
			Onyx::Model* model = m_Viewport->GetModel(modelPath);
			if (model && !model->GetMeshes().empty())
			{
				int selectedMeshIdx = m_World->GetSelectedMeshIndex();

				if (selectedMeshIdx >= 0 && selectedMeshIdx < static_cast<int>(model->GetMeshes().size()))
				{
					auto& selectedMesh = model->GetMeshes()[selectedMeshIdx];
					std::string selectedName = selectedMesh.m_Name.empty()
												   ? ("Mesh " + std::to_string(selectedMeshIdx))
												   : selectedMesh.m_Name;
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Selected: %s", selectedName.c_str());
					if (ImGui::SmallButton("Deselect Mesh"))
					{
						m_World->DeselectMesh();
					}
					ImGui::Separator();
				}

				if (ImGui::TreeNode("Meshes"))
				{
					auto& meshes = model->GetMeshes();
					ImGui::Text("%zu meshes", meshes.size());

					// Filter/search for large mesh lists
					static char meshFilter[128] = "";
					if (meshes.size() > 20)
					{
						ImGui::InputTextWithHint("##meshfilter", "Search meshes...", meshFilter, sizeof(meshFilter));
					}

					ImGui::BeginChild("MeshList", ImVec2(0, std::min(400.0f, meshes.size() * 22.0f)), true);
					for (size_t i = 0; i < meshes.size(); i++)
					{
						auto& mesh = meshes[i];
						const std::string& meshName = mesh.m_Name;
						const char* displayName = meshName.empty() ? "Unnamed" : meshName.c_str();

						// Filter check
						if (meshFilter[0] != '\0' && meshName.find(meshFilter) == std::string::npos)
						{
							continue;
						}

						ImGui::PushID(static_cast<int>(i));

						bool isSelected = (static_cast<int>(i) == selectedMeshIdx);
						ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
						if (isSelected)
						{
							nodeFlags |= ImGuiTreeNodeFlags_Selected;
						}

						bool nodeOpen = ImGui::TreeNodeEx(displayName, nodeFlags);

						if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
						{
							m_World->SelectMesh(static_cast<int>(i));
						}

						if (nodeOpen)
						{
							std::string matKey = meshName.empty() ? ("Mesh " + std::to_string(i)) : meshName;
							MeshMaterial& material = object->GetOrCreateMeshMaterial(matKey);

							if (ImGui::TreeNode("Transform Offset"))
							{
								ImGui::DragFloat3("Position", glm::value_ptr(material.positionOffset), 0.1f);
								ImGui::DragFloat3("Rotation", glm::value_ptr(material.rotationOffset), 1.0f);
								ImGui::DragFloat("Scale", &material.scaleMultiplier, 0.01f, 0.01f, 10.0f);
								if (ImGui::Button("Reset Transform"))
								{
									material.positionOffset = glm::vec3(0.0f);
									material.rotationOffset = glm::vec3(0.0f);
									material.scaleMultiplier = 1.0f;
								}
								ImGui::TreePop();
							}

							RenderMaterialSelector("Material##mesh", material.materialId);
							if (!material.materialId.empty() && m_Viewport)
							{
								if (auto* mat = Onyx::Application::GetInstance().GetAssetManager().GetMaterial(material.materialId))
								{
									ImGui::TextDisabled("  Albedo: %s", mat->albedoPath.empty() ? "(none)" : mat->albedoPath.c_str());
									ImGui::TextDisabled("  Normal: %s", mat->normalPath.empty() ? "(none)" : mat->normalPath.c_str());
								}
							}

							ImGui::TreePop();
						}

						ImGui::PopID();
					}
					ImGui::EndChild();
					ImGui::TreePop();
				}
			}
		}

		if (!modelPath.empty() && m_Viewport && m_Viewport->IsAnimatedModel(modelPath))
		{
			ImGui::Separator();
			if (ImGui::TreeNodeEx("Animation", ImGuiTreeNodeFlags_DefaultOpen))
			{
				Onyx::Animator* animator = m_Viewport->GetAnimator(object->GetGuid());
				Onyx::AnimatedModel* animModel = m_Viewport->GetAnimatedModel(modelPath);

				if (animator && animModel)
				{
					std::vector<std::string> animNames = animModel->GetAnimationNames();
					const Onyx::Animation* currentAnim = animator->GetCurrentAnimation();
					std::string currentName = currentAnim ? currentAnim->GetName() : "None";

					if (ImGui::BeginCombo("Animation", currentName.c_str()))
					{
						for (int i = 0; i < static_cast<int>(animNames.size()); i++)
						{
							bool isSelected = (currentAnim && animNames[i] == currentName);
							if (ImGui::Selectable(animNames[i].c_str(), isSelected))
							{
								animator->Play(animNames[i], object->GetAnimationLoop());
								object->SetCurrentAnimation(animNames[i]);
							}
							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					bool loop = object->GetAnimationLoop();
					if (ImGui::Checkbox("Loop", &loop))
					{
						object->SetAnimationLoop(loop);
						animator->SetLoop(loop);
					}

					bool isPlaying = animator->IsPlaying();
					bool isPaused = animator->IsPaused();

					ImGui::SameLine();
					if (isPaused)
					{
						if (ImGui::Button("Resume"))
						{
							animator->Resume();
						}
					}
					else if (isPlaying)
					{
						if (ImGui::Button("Pause"))
						{
							animator->Pause();
						}
					}
					else
					{
						if (ImGui::Button("Play"))
						{
							if (animModel->GetAnimationCount() > 0)
							{
								const std::string& savedAnim = object->GetCurrentAnimation();
								if (!savedAnim.empty() && animModel->GetAnimation(savedAnim))
								{
									animator->Play(savedAnim, loop);
								}
								else
								{
									animator->Play(0, loop);
								}
							}
						}
					}
					ImGui::SameLine();
					if (ImGui::Button("Stop"))
					{
						animator->Stop();
					}

					float speed = object->GetAnimationSpeed();
					if (ImGui::SliderFloat("Speed", &speed, 0.0f, 3.0f, "%.2fx"))
					{
						object->SetAnimationSpeed(speed);
						animator->SetSpeed(speed);
					}

					if (currentAnim)
					{
						float durationSec = currentAnim->GetDuration() / currentAnim->GetTicksPerSecond();
						if (durationSec < 0.1f)
						{
							ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Static pose (%.3fs)", durationSec);
						}
						else
						{
							float progress = animator->GetNormalizedTime();
							ImGui::ProgressBar(progress, ImVec2(-1, 0), nullptr);
							ImGui::Text("Duration: %.2fs", durationSec);
						}
					}

					ImGui::TextDisabled("%d animations, %d bones",
										animModel->GetAnimationCount(),
										static_cast<int>(animModel->GetSkeleton().GetBoneCount()));

					ImGui::Spacing();
					ImGui::Separator();

					if (ImGui::TreeNode("Animation Files"))
					{
						ImGui::TextDisabled("Load additional animations (e.g., Mixamo \"Without Skin\")");

						const auto& animPaths = object->GetAnimationPaths();
						int removeIndex = -1;

						for (size_t i = 0; i < animPaths.size(); i++)
						{
							ImGui::PushID(static_cast<int>(i));

							std::string displayName = animPaths[i];
							size_t lastSlash = displayName.find_last_of("/\\");
							if (lastSlash != std::string::npos)
							{
								displayName = displayName.substr(lastSlash + 1);
							}

							ImGui::BulletText("%s", displayName.c_str());
							ImGui::SameLine();
							if (ImGui::SmallButton("X"))
							{
								removeIndex = static_cast<int>(i);
							}
							if (ImGui::IsItemHovered())
							{
								ImGui::SetTooltip("%s", animPaths[i].c_str());
							}

							ImGui::PopID();
						}

						if (removeIndex >= 0)
						{
							object->RemoveAnimationPath(static_cast<size_t>(removeIndex));
							m_Viewport->InvalidateAnimator(object->GetGuid());
						}

						if (ImGui::Button("+ Add Animation File"))
						{
							m_ShowFileBrowser = true;
							m_BrowseFilter = ".fbx,.FBX,.dae,.DAE,.glb,.gltf";
							m_BrowseCurrentDir = "MMOGame/assets/models";
							m_BrowseCallback = [this, object](const std::string& path) {
								object->AddAnimationPath(path);
								// Reload animations for this object
								m_Viewport->ReloadAnimations(object->GetGuid(), object);
							};
						}

						ImGui::TreePop();
					}
				}
				else
				{
					ImGui::TextDisabled("Loading animation data...");
				}

				ImGui::TreePop();
			}
		}

		// Object-level material
		{
			std::string matId = object->GetMaterialId();
			if (RenderMaterialSelector("Material", matId))
			{
				object->SetMaterialId(matId);
			}
			if (!matId.empty() && m_Viewport)
			{
				if (auto* mat = Onyx::Application::GetInstance().GetAssetManager().GetMaterial(matId))
				{
					ImGui::TextDisabled("  Albedo: %s", mat->albedoPath.empty() ? "(none)" : mat->albedoPath.c_str());
					ImGui::TextDisabled("  Normal: %s", mat->normalPath.empty() ? "(none)" : mat->normalPath.c_str());
				}
			}
		}

		ImGui::Spacing();

		ImGui::Text("Rendering");
		ImGui::Indent();

		bool castsShadow = object->CastsShadow();
		if (ImGui::Checkbox("Casts Shadow", &castsShadow))
		{
			object->SetCastsShadow(castsShadow);
		}

		bool receivesLightmap = object->ReceivesLightmap();
		if (ImGui::Checkbox("Receives Lightmap", &receivesLightmap))
		{
			object->SetReceivesLightmap(receivesLightmap);
		}

		ImGui::Unindent();
	}

	void InspectorPanel::RenderSpawnPointProperties(SpawnPoint* object)
	{
		uint32_t templateId = object->GetCreatureTemplateId();
		if (ImGui::InputScalar("Creature Template ID", ImGuiDataType_U32, &templateId))
		{
			object->SetCreatureTemplateId(templateId);
		}

		ImGui::Spacing();

		if (ImGui::TreeNode("Preview Model"))
		{
			RenderPathInput("Model", object->GetModelPath(), [object](const std::string& path) { object->SetModelPath(path); }, ".obj,.fbx,.gltf,.glb");

			{
				std::string matId = object->GetMaterialId();
				if (RenderMaterialSelector("Material", matId))
				{
					object->SetMaterialId(matId);
				}
				if (!matId.empty() && m_Viewport)
				{
					if (auto* mat = Onyx::Application::GetInstance().GetAssetManager().GetMaterial(matId))
					{
						ImGui::TextDisabled("  Albedo: %s", mat->albedoPath.empty() ? "(none)" : mat->albedoPath.c_str());
						ImGui::TextDisabled("  Normal: %s", mat->normalPath.empty() ? "(none)" : mat->normalPath.c_str());
					}
				}
			}

			ImGui::TreePop();
		}

		ImGui::Spacing();

		ImGui::Text("Spawn Behavior");
		ImGui::Indent();

		float respawnTime = object->GetRespawnTime();
		if (ImGui::DragFloat("Respawn Time (s)", &respawnTime, 1.0f, 0.0f, 3600.0f))
		{
			object->SetRespawnTime(respawnTime);
		}

		float wanderRadius = object->GetWanderRadius();
		if (ImGui::DragFloat("Wander Radius", &wanderRadius, 0.5f, 0.0f, 100.0f))
		{
			object->SetWanderRadius(wanderRadius);
		}

		uint32_t maxCount = object->GetMaxCount();
		if (ImGui::InputScalar("Max Count", ImGuiDataType_U32, &maxCount))
		{
			object->SetMaxCount(maxCount);
		}

		ImGui::Unindent();
	}

	void InspectorPanel::RenderLightProperties(Light* object)
	{
		const char* lightTypes[] = {"Point", "Spot", "Directional"};
		int currentType = static_cast<int>(object->GetLightType());
		if (ImGui::Combo("Light Type", &currentType, lightTypes, 3))
		{
			object->SetLightType(static_cast<LightType>(currentType));
		}

		glm::vec3 color = object->GetColor();
		if (ImGui::ColorEdit3("Color", glm::value_ptr(color)))
		{
			object->SetColor(color);
		}

		float intensity = object->GetIntensity();
		if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
		{
			object->SetIntensity(intensity);
		}

		LightType type = object->GetLightType();
		if (type == LightType::POINT || type == LightType::SPOT)
		{
			float radius = object->GetRadius();
			if (ImGui::DragFloat("Radius", &radius, 0.5f, 0.0f, 1000.0f))
			{
				object->SetRadius(radius);
			}
		}

		if (type == LightType::SPOT)
		{
			float innerAngle = object->GetInnerAngle();
			float outerAngle = object->GetOuterAngle();

			if (ImGui::DragFloat("Inner Angle", &innerAngle, 1.0f, 0.0f, outerAngle))
			{
				object->SetInnerAngle(innerAngle);
			}
			if (ImGui::DragFloat("Outer Angle", &outerAngle, 1.0f, innerAngle, 180.0f))
			{
				object->SetOuterAngle(outerAngle);
			}
		}

		bool castsShadows = object->CastsShadows();
		if (ImGui::Checkbox("Casts Shadows", &castsShadows))
		{
			object->SetCastsShadows(castsShadows);
		}

		bool bakedOnly = object->IsBakedOnly();
		if (ImGui::Checkbox("Baked Only", &bakedOnly))
		{
			object->SetBakedOnly(bakedOnly);
		}
	}

	void InspectorPanel::RenderParticleEmitterProperties(ParticleEmitter* object)
	{
		uint32_t effectId = object->GetEffectId();
		if (ImGui::InputScalar("Effect ID", ImGuiDataType_U32, &effectId))
		{
			object->SetEffectId(effectId);
		}

		char effectPath[256];
		strncpy(effectPath, object->GetEffectPath().c_str(), sizeof(effectPath) - 1);
		effectPath[sizeof(effectPath) - 1] = '\0';

		if (ImGui::InputText("Effect Path", effectPath, sizeof(effectPath)))
		{
			object->SetEffectPath(effectPath);
		}

		bool autoPlay = object->IsAutoPlay();
		if (ImGui::Checkbox("Auto Play", &autoPlay))
		{
			object->SetAutoPlay(autoPlay);
		}

		bool loop = object->IsLooping();
		if (ImGui::Checkbox("Loop", &loop))
		{
			object->SetLooping(loop);
		}
	}

	void InspectorPanel::RenderTriggerVolumeProperties(TriggerVolume* object)
	{
		const char* shapeTypes[] = {"Box", "Sphere", "Capsule"};
		int currentShape = static_cast<int>(object->GetShape());
		if (ImGui::Combo("Shape", &currentShape, shapeTypes, 3))
		{
			object->SetShape(static_cast<TriggerShape>(currentShape));
		}

		if (object->GetShape() == TriggerShape::BOX)
		{
			glm::vec3 halfExtents = object->GetHalfExtents();
			if (ImGui::DragFloat3("Half Extents", glm::value_ptr(halfExtents), 0.1f, 0.1f, 100.0f))
			{
				object->SetHalfExtents(halfExtents);
			}
		}

		if (object->GetShape() == TriggerShape::SPHERE || object->GetShape() == TriggerShape::CAPSULE)
		{
			float radius = object->GetRadius();
			if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 100.0f))
			{
				object->SetRadius(radius);
			}
		}

		const char* eventTypes[] = {"On Enter", "On Exit", "On Stay"};
		int currentEvent = static_cast<int>(object->GetTriggerEvent());
		if (ImGui::Combo("Event Type", &currentEvent, eventTypes, 3))
		{
			object->SetTriggerEvent(static_cast<TriggerEvent>(currentEvent));
		}

		char scriptName[256];
		strncpy(scriptName, object->GetScriptName().c_str(), sizeof(scriptName) - 1);
		scriptName[sizeof(scriptName) - 1] = '\0';

		if (ImGui::InputText("Script Name", scriptName, sizeof(scriptName)))
		{
			object->SetScriptName(scriptName);
		}

		bool triggerOnce = object->IsTriggerOnce();
		if (ImGui::Checkbox("Trigger Once", &triggerOnce))
		{
			object->SetTriggerOnce(triggerOnce);
		}

		ImGui::Text("Filters:");
		bool filterPlayers = object->TriggerPlayers();
		if (ImGui::Checkbox("Players", &filterPlayers))
		{
			object->SetTriggerPlayers(filterPlayers);
		}
		ImGui::SameLine();
		bool filterCreatures = object->TriggerCreatures();
		if (ImGui::Checkbox("Creatures", &filterCreatures))
		{
			object->SetTriggerCreatures(filterCreatures);
		}
	}

	void InspectorPanel::RenderInstancePortalProperties(InstancePortal* object)
	{
		uint32_t dungeonId = object->GetDungeonId();
		if (ImGui::InputScalar("Dungeon ID", ImGuiDataType_U32, &dungeonId))
		{
			object->SetDungeonId(dungeonId);
		}

		char dungeonName[256];
		strncpy(dungeonName, object->GetDungeonName().c_str(), sizeof(dungeonName) - 1);
		dungeonName[sizeof(dungeonName) - 1] = '\0';

		if (ImGui::InputText("Dungeon Name", dungeonName, sizeof(dungeonName)))
		{
			object->SetDungeonName(dungeonName);
		}

		ImGui::Separator();
		ImGui::Text("Exit Location:");

		uint32_t exitMapId = object->GetExitMapId();
		if (ImGui::InputScalar("Exit Map ID", ImGuiDataType_U32, &exitMapId))
		{
			object->SetExitMapId(exitMapId);
		}

		glm::vec3 exitPos = object->GetExitPosition();
		if (ImGui::DragFloat3("Exit Position", glm::value_ptr(exitPos), 0.1f))
		{
			object->SetExitPosition(exitPos);
		}

		float interactRadius = object->GetInteractionRadius();
		if (ImGui::DragFloat("Interaction Radius", &interactRadius, 0.1f, 0.1f, 20.0f))
		{
			object->SetInteractionRadius(interactRadius);
		}

		ImGui::Separator();
		ImGui::Text("Requirements:");

		uint32_t minLevel = object->GetMinLevel();
		if (ImGui::InputScalar("Min Level", ImGuiDataType_U32, &minLevel))
		{
			object->SetMinLevel(minLevel);
		}

		uint32_t maxLevel = object->GetMaxLevel();
		if (ImGui::InputScalar("Max Level", ImGuiDataType_U32, &maxLevel))
		{
			object->SetMaxLevel(maxLevel);
		}

		uint32_t minPlayers = object->GetMinPlayers();
		if (ImGui::InputScalar("Min Players", ImGuiDataType_U32, &minPlayers))
		{
			object->SetMinPlayers(minPlayers);
		}

		uint32_t maxPlayers = object->GetMaxPlayers();
		if (ImGui::InputScalar("Max Players", ImGuiDataType_U32, &maxPlayers))
		{
			object->SetMaxPlayers(maxPlayers);
		}
	}

	void InspectorPanel::RenderPlayerSpawnProperties(PlayerSpawn* object)
	{
		const auto& combos = RaceClassRegistry::Combos();

		// Determine the currently-selected combo (single per spawn in v1).
		const auto& pairs = object->GetRaceClassPairs();
		int currentIdx = -1;
		if (!pairs.empty())
		{
			for (size_t i = 0; i < combos.size(); ++i)
			{
				if (combos[i].raceId == pairs.front().first &&
					combos[i].classId == pairs.front().second)
				{
					currentIdx = static_cast<int>(i);
					break;
				}
			}
		}

		const char* preview = (currentIdx >= 0)
								  ? combos[currentIdx].DisplayName().c_str()
								  : "(none — will not export)";

		if (ImGui::BeginCombo("Race / Class", preview))
		{
			for (size_t i = 0; i < combos.size(); ++i)
			{
				const std::string label = combos[i].DisplayName();
				const bool selected = (currentIdx == static_cast<int>(i));
				if (ImGui::Selectable(label.c_str(), selected))
				{
					// Replace any existing pairs with the chosen single combo.
					while (!object->GetRaceClassPairs().empty())
					{
						const auto& p = object->GetRaceClassPairs().front();
						object->RemoveRaceClass(p.first, p.second);
					}
					object->AddRaceClass(combos[i].raceId, combos[i].classId);
				}
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// Conflict warning: another PlayerSpawn covers the same combo.
		if (currentIdx >= 0 && m_World)
		{
			const auto& chosen = combos[currentIdx];
			int conflicts = 0;
			for (const auto& other : m_World->GetPlayerSpawns())
			{
				if (other.get() == object)
					continue;
				for (const auto& p : other->GetRaceClassPairs())
				{
					if (p.first == chosen.raceId && p.second == chosen.classId)
					{
						++conflicts;
						break;
					}
				}
			}
			if (conflicts > 0)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
								   "⚠ Conflict: another PlayerSpawn covers this combo. "
								   "Re-export will pick one arbitrarily.");
			}
		}

		if (currentIdx < 0)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
							   "✗ No race/class assigned; will not export.");
		}

		ImGui::Spacing();
		ImGui::Separator();

		float orientation = object->GetOrientation();
		if (ImGui::SliderAngle("Orientation", &orientation, 0.0f, 360.0f))
		{
			object->SetOrientation(orientation);
		}
	}

	bool InspectorPanel::RenderMaterialSelector(const char* label, std::string& materialId)
	{
		if (!m_Viewport)
			return false;

		auto& lib = Onyx::Application::GetInstance().GetAssetManager();
		auto ids = lib.GetAllMaterialIds();
		std::sort(ids.begin(), ids.end());

		bool changed = false;
		const char* preview = materialId.empty() ? "(None)" : materialId.c_str();

		ImGui::PushID(label);
		if (ImGui::BeginCombo(label, preview))
		{
			// (None) option to clear
			if (ImGui::Selectable("(None)", materialId.empty()))
			{
				materialId.clear();
				changed = true;
			}

			for (const auto& id : ids)
			{
				bool isSelected = (id == materialId);
				const Onyx::Material* mat = lib.GetMaterial(id);
				std::string displayStr = mat ? (mat->name.empty() ? id : mat->name + " (" + id + ")") : id;

				if (ImGui::Selectable(displayStr.c_str(), isSelected))
				{
					materialId = id;
					changed = true;
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();

		return changed;
	}

	bool InspectorPanel::RenderPathInput(const char* label, const std::string& currentPath,
										 const std::function<void(const std::string&)>& onPathChanged,
										 const char* filter)
	{
		bool changed = false;

		ImGui::PushID(label);

		float totalWidth = ImGui::GetContentRegionAvail().x;
		float buttonWidth = 60.0f;
		float clearButtonWidth = 20.0f;
		float inputWidth = totalWidth - buttonWidth - clearButtonWidth - 8.0f;

		char pathBuf[512];
		strncpy(pathBuf, currentPath.c_str(), sizeof(pathBuf) - 1);
		pathBuf[sizeof(pathBuf) - 1] = '\0';

		ImGui::SetNextItemWidth(inputWidth);
		if (ImGui::InputText(label, pathBuf, sizeof(pathBuf)))
		{
			onPathChanged(pathBuf);
			changed = true;
		}

		if (ImGui::IsItemHovered() && !currentPath.empty())
		{
			ImGui::BeginTooltip();
			ImGui::Text("%s", currentPath.c_str());
			ImGui::EndTooltip();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
			{
				const char* droppedPath = static_cast<const char*>(payload->Data);

				bool accepted = true;
				if (filter && strlen(filter) > 0)
				{
					std::string ext = std::filesystem::path(droppedPath).extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					std::string filterStr = filter;
					accepted = false;
					size_t pos = 0;
					while (pos < filterStr.size())
					{
						size_t next = filterStr.find(',', pos);
						if (next == std::string::npos)
							next = filterStr.size();

						std::string allowedExt = filterStr.substr(pos, next - pos);
						std::transform(allowedExt.begin(), allowedExt.end(), allowedExt.begin(), ::tolower);

						if (ext == allowedExt)
						{
							accepted = true;
							break;
						}
						pos = next + 1;
					}
				}

				if (accepted)
				{
					onPathChanged(droppedPath);
					changed = true;
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		if (ImGui::Button("Browse", ImVec2(buttonWidth, 0)))
		{
			m_ShowFileBrowser = true;
			m_BrowseFilter = filter ? filter : "";
			m_BrowseCallback = onPathChanged;
		}

		ImGui::SameLine();

		if (ImGui::Button("X", ImVec2(clearButtonWidth, 0)))
		{
			onPathChanged("");
			changed = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Clear");
		}

		ImGui::PopID();

		return changed;
	}

	void InspectorPanel::RenderFileBrowserPopup()
	{
		if (!m_ShowFileBrowser)
			return;

		if (m_BrowseCurrentDir.empty())
		{
			m_BrowseCurrentDir = "MMOGame/assets";
		}

		ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Select File", &m_ShowFileBrowser))
		{
			if (ImGui::Button("^"))
			{
				std::filesystem::path current(m_BrowseCurrentDir);
				if (current.has_parent_path() && current.parent_path() != current)
				{
					m_BrowseCurrentDir = current.parent_path().string();
				}
			}
			ImGui::SameLine();
			ImGui::Text("%s", m_BrowseCurrentDir.c_str());

			ImGui::Separator();

			if (!m_BrowseFilter.empty())
			{
				ImGui::TextDisabled("Filter: %s", m_BrowseFilter.c_str());
			}

			ImGui::Separator();

			ImGui::BeginChild("FileList", ImVec2(0, -30), true);

			namespace fs = std::filesystem;

			if (fs::exists(m_BrowseCurrentDir))
			{
				std::vector<fs::directory_entry> entries;
				try
				{
					for (const auto& entry : fs::directory_iterator(m_BrowseCurrentDir))
					{
						entries.push_back(entry);
					}
				}
				catch (...)
				{}

				std::sort(entries.begin(), entries.end(),
						  [](const fs::directory_entry& a, const fs::directory_entry& b) {
							  if (a.is_directory() != b.is_directory())
							  {
								  return a.is_directory() > b.is_directory();
							  }
							  return a.path().filename() < b.path().filename();
						  });

				for (const auto& entry : entries)
				{
					std::string name = entry.path().filename().string();

					// Skip hidden files
					if (!name.empty() && name[0] == '.')
						continue;

					if (entry.is_directory())
					{
						if (ImGui::Selectable(("[DIR] " + name).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
						{
							if (ImGui::IsMouseDoubleClicked(0))
							{
								m_BrowseCurrentDir = entry.path().string();
							}
						}
					}
					else
					{
						std::string ext = entry.path().extension().string();
						std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

						bool matchesFilter = m_BrowseFilter.empty();
						if (!matchesFilter)
						{
							std::string filterLower = m_BrowseFilter;
							std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

							size_t pos = 0;
							while (pos < filterLower.size())
							{
								size_t next = filterLower.find(',', pos);
								if (next == std::string::npos)
									next = filterLower.size();

								std::string allowedExt = filterLower.substr(pos, next - pos);
								if (ext == allowedExt)
								{
									matchesFilter = true;
									break;
								}
								pos = next + 1;
							}
						}

						if (matchesFilter)
						{
							if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
							{
								if (ImGui::IsMouseDoubleClicked(0))
								{
									if (m_BrowseCallback)
									{
										m_BrowseCallback(entry.path().string());
									}
									m_ShowFileBrowser = false;
								}
							}
						}
						else
						{
							ImGui::TextDisabled("%s", name.c_str());
						}
					}
				}
			}

			ImGui::EndChild();

			if (ImGui::Button("Cancel"))
			{
				m_ShowFileBrowser = false;
			}
		}
		ImGui::End();

		if (!m_ShowFileBrowser)
		{
			m_BrowseCallback = nullptr;
		}
	}

} // namespace MMO
