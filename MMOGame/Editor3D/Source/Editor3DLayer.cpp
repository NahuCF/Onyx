#include "Editor3DLayer.h"
#include "Commands/EditorCommand.h"
#include "Export/MigrationSqlWriter.h"
#include "Panels/AssetBrowserPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/LightingPanel.h"
#include "Panels/MaterialEditorPanel.h"
#include "Panels/StatisticsPanel.h"
#include "Panels/TerrainPanel.h"
#include "Panels/ViewportPanel.h"
#include "Settings/EditorPreferences.h"
#include "Terrain/MaterialSerializer.h"
#include "World/EditorWorldSystem.h"
#include <Core/Application.h>
#include <Core/Platform.h>
#include <World/PlayerSpawn.h>
#include <World/SpawnPoint.h>
#include <cstdlib>
#include <filesystem>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <map>
#include <sstream>

#ifdef HAS_DATABASE
#include "../../Shared/Source/Database/MigrationRunner.h"
#endif

namespace MMO {

	Editor3DLayer::Editor3DLayer() = default;

	Editor3DLayer::~Editor3DLayer()
	{
		// Tear down in a controlled order so we don't crash on shutdown:
		//  1. Stop any Run-Locally subprocesses (LoginServer, WorldServer, Client)
		//     before tearing anything else down — they may briefly outlive the
		//     editor process otherwise, and pqxx's connection close can hang if
		//     the server it's talking to is in the middle of being killed.
		if (m_RunSession)
		{
			m_RunSession.reset();
		}

		// 2. Close the pqxx connection now while the rest of the editor's state
		//    is still alive. Wrapped in try/catch so a malformed network shutdown
		//    can never bubble out of the destructor.
#ifdef HAS_DATABASE
		try
		{
			if (m_Database.IsConnected())
				m_Database.Disconnect();
		}
		catch (...)
		{
			// Best-effort; nothing to do.
		}
#endif
	}

	void Editor3DLayer::OnAttach()
	{
		auto& window = *Onyx::Application::GetInstance().GetWindow();
		window.SetVSync(false);

		EditorPreferences::Instance().Load();
		m_LastSaveTime = std::chrono::steady_clock::now();

		// Load map registry
		m_MapRegistry.Load("maps");
		m_MapBrowser.SetRegistry(&m_MapRegistry);
		m_ShowMapBrowser = true;
		m_MapLoaded = false;

		// Create SceneRenderer
		m_SceneRenderer = std::make_unique<Onyx::SceneRenderer>();

		m_SceneRenderer->Init(&Onyx::Application::GetInstance().GetAssetManager());
		// Setup panels (created once, rendered only when map is loaded)
		m_ViewportPanel = m_PanelManager.AddPanel<ViewportPanel>("Viewport", true);
		m_ViewportPanel->SetSceneRenderer(m_SceneRenderer.get());

		m_PanelManager.AddPanel<HierarchyPanel>("Hierarchy", true);

		auto* inspector = m_PanelManager.AddPanel<InspectorPanel>("Inspector", true);
		inspector->SetViewport(m_ViewportPanel);

		m_PanelManager.AddPanel<AssetBrowserPanel>("Asset Browser", true);

		auto* stats = m_PanelManager.AddPanel<StatisticsPanel>("Statistics", false);
		stats->SetViewport(m_ViewportPanel);

		auto* lighting = m_PanelManager.AddPanel<LightingPanel>("Lighting", true);
		lighting->SetViewport(m_ViewportPanel);

		auto* terrain = m_PanelManager.AddPanel<TerrainPanel>("Terrain", true);
		terrain->SetViewport(m_ViewportPanel);

		auto* materialEditor = m_PanelManager.AddPanel<MaterialEditorPanel>("Material Editor", false);
		materialEditor->SetMaterialLibrary(&m_ViewportPanel->GetTerrainMaterialLibrary());

		m_PanelManager.SetWorld(&m_World);
		m_PanelManager.OnInit();

		terrain->SetMaterialLibrary(&m_ViewportPanel->GetTerrainMaterialLibrary());

		auto* assetBrowser = m_PanelManager.GetPanel<AssetBrowserPanel>("Asset Browser");
		if (assetBrowser)
		{
			assetBrowser->SetMaterialOpenCallback([materialEditor, this](const std::string& filePath) {
				std::filesystem::path p(filePath);
				std::string matId = p.stem().string();

				auto& lib = m_ViewportPanel->GetTerrainMaterialLibrary();
				Onyx::Material mat;
				if (Editor3D::LoadMaterial(filePath, mat))
				{
					if (mat.id.empty())
						mat.id = matId;
					lib.UpdateMaterial(mat.id, mat);
					materialEditor->OpenMaterial(mat.id);
				}
			});

			assetBrowser->SetMaterialCreateCallback([this, materialEditor](const std::string& currentDir) {
				auto& lib = m_ViewportPanel->GetTerrainMaterialLibrary();
				std::string newId = lib.CreateMaterial("New Material", currentDir);
				materialEditor->OpenMaterial(newId);
			});
		}
	}

	void Editor3DLayer::OnUpdate()
	{
		if (m_RunSession)
			m_RunSession->Tick();
		if (!m_MapLoaded)
			return;

		// Auto-save: if enabled and the configured interval has elapsed, persist
		// dirty chunks. Reuses the same Ctrl+S code path so manual saves and
		// auto-saves go through one path.
		auto& prefs = EditorPreferences::Instance();
		if (prefs.AutosaveEnabled() && m_ViewportPanel)
		{
			const auto now = std::chrono::steady_clock::now();
			const float elapsed = std::chrono::duration<float>(now - m_LastSaveTime).count();
			if (elapsed >= prefs.AutosaveIntervalSecs())
			{
				HandleSave();
			}
		}

		HandleGlobalShortcuts();
	}

	void Editor3DLayer::OnImGui()
	{
		SetupDockspace();
	}

	void Editor3DLayer::SetupDockspace()
	{
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
		windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		if (ImGui::BeginMenuBar())
		{
			RenderMenuBar();
			ImGui::EndMenuBar();
		}

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

			static bool firstTime = true;
			if (firstTime)
			{
				firstTime = false;

				ImGui::DockBuilderRemoveNode(dockspaceId);
				ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
				ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

				ImGuiID dockLeft, dockRight, dockBottom;
				ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.2f, &dockLeft, &dockspaceId);
				ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.25f, &dockRight, &dockspaceId);
				ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.25f, &dockBottom, &dockspaceId);

				ImGuiID dockRightTop, dockRightBottom;
				ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Down, 0.4f, &dockRightBottom, &dockRightTop);

				ImGuiID dockLeftTop, dockLeftBottom;
				ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.4f, &dockLeftBottom, &dockLeftTop);

				ImGui::DockBuilderDockWindow("Hierarchy", dockLeftTop);
				ImGui::DockBuilderDockWindow("Lighting", dockLeftBottom);
				ImGui::DockBuilderDockWindow("Inspector", dockRightTop);
				ImGui::DockBuilderDockWindow("Terrain", dockRightBottom);
				ImGui::DockBuilderDockWindow("Asset Browser", dockBottom);
				ImGui::DockBuilderDockWindow("Viewport", dockspaceId);

				ImGui::DockBuilderFinish(dockspaceId);
			}
		}

		ImGui::End();

		RenderRuntimeExportDialog();

		// Show map browser if no map is loaded
		if (m_ShowMapBrowser)
		{
			if (m_MapBrowser.Render())
			{
				uint32_t selectedId = m_MapBrowser.GetSelectedMapId();
				if (selectedId > 0)
				{
					LoadMap(selectedId);
					m_ShowMapBrowser = false;
				}
			}
		}

		// Only render panels when a map is loaded
		if (m_MapLoaded)
		{
			m_PanelManager.OnImGuiRender();
		}
	}

	void Editor3DLayer::RenderMenuBar()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Map", "Ctrl+O"))
			{
				m_MapBrowser.Reset();
				m_ShowMapBrowser = true;
			}
			const auto& recentMaps = EditorPreferences::Instance().RecentMaps();
			if (ImGui::BeginMenu("Recent Maps", !recentMaps.empty()))
			{
				for (uint32_t id : recentMaps)
				{
					if (const MapEntry* entry = m_MapRegistry.GetMapById(id))
					{
						std::string label = entry->displayName + " (id " + std::to_string(id) + ")";
						if (ImGui::MenuItem(label.c_str()))
						{
							LoadMap(id);
						}
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Save", "Ctrl+S", false, m_MapLoaded))
			{
				HandleSave();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Export Runtime Data...", nullptr, false, m_MapLoaded))
			{
				m_ShowRuntimeExportDialog = true;
			}
			ImGui::Separator();
			const bool sessionRunning = m_RunSession && m_RunSession->GetStatus() == LocalRunSession::Status::Running;
			if (ImGui::MenuItem("Run Locally", "Ctrl+R", false, m_MapLoaded && !sessionRunning))
			{
				StartRunLocally();
			}
			if (ImGui::MenuItem("Stop", nullptr, false, sessionRunning))
			{
				if (m_RunSession)
					m_RunSession->Stop();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit"))
			{
				Onyx::Application::GetInstance().GetWindow()->CloseWindow();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			auto& history = CommandHistory::Get();

			std::string undoLabel = "Undo";
			if (history.CanUndo())
			{
				undoLabel += " " + history.GetUndoDescription();
			}
			if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, history.CanUndo()))
			{
				history.Undo();
			}

			std::string redoLabel = "Redo";
			if (history.CanRedo())
			{
				redoLabel += " " + history.GetRedoDescription();
			}
			if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, history.CanRedo()))
			{
				history.Redo();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Delete", "Delete", false, m_World.HasSelection()))
			{
				m_World.DeleteSelected();
			}
			if (ImGui::MenuItem("Select All", "Ctrl+A"))
			{
				m_World.SelectAll();
			}
			if (ImGui::MenuItem("Deselect All", "Escape"))
			{
				m_World.DeselectAll();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Create"))
		{
			if (ImGui::MenuItem("Static Object"))
			{
				auto* obj = m_World.CreateStaticObject();
				m_World.Select(obj);
			}
			if (ImGui::MenuItem("Spawn Point"))
			{
				auto* obj = m_World.CreateSpawnPoint();
				m_World.Select(obj);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Point Light"))
			{
				auto* obj = m_World.CreateLight(LightType::POINT);
				m_World.Select(obj);
			}
			if (ImGui::MenuItem("Spot Light"))
			{
				auto* obj = m_World.CreateLight(LightType::SPOT);
				m_World.Select(obj);
			}
			if (ImGui::MenuItem("Directional Light"))
			{
				auto* obj = m_World.CreateLight(LightType::DIRECTIONAL);
				m_World.Select(obj);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Particle Emitter"))
			{
				auto* obj = m_World.CreateParticleEmitter();
				m_World.Select(obj);
			}
			if (ImGui::MenuItem("Trigger Volume"))
			{
				auto* obj = m_World.CreateTriggerVolume();
				m_World.Select(obj);
			}
			if (ImGui::MenuItem("Instance Portal"))
			{
				auto* obj = m_World.CreateInstancePortal();
				m_World.Select(obj);
			}
			if (ImGui::MenuItem("Player Spawn"))
			{
				auto* obj = m_World.CreatePlayerSpawn();
				m_World.Select(obj);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			ImGui::SeparatorText("Panels");
			m_PanelManager.RenderPanelToggles();

			ImGui::SeparatorText("Object Visibility");

			bool staticVisible = m_World.IsTypeVisible(WorldObjectType::STATIC_OBJECT);
			bool spawnVisible = m_World.IsTypeVisible(WorldObjectType::SPAWN_POINT);
			bool lightVisible = m_World.IsTypeVisible(WorldObjectType::LIGHT);
			bool particleVisible = m_World.IsTypeVisible(WorldObjectType::PARTICLE_EMITTER);
			bool triggerVisible = m_World.IsTypeVisible(WorldObjectType::TRIGGER_VOLUME);
			bool portalVisible = m_World.IsTypeVisible(WorldObjectType::INSTANCE_PORTAL);
			bool playerSpawnVisible = m_World.IsTypeVisible(WorldObjectType::PLAYER_SPAWN);

			if (ImGui::MenuItem("Static Objects", nullptr, &staticVisible))
			{
				m_World.SetTypeVisible(WorldObjectType::STATIC_OBJECT, staticVisible);
			}
			if (ImGui::MenuItem("Spawn Points", nullptr, &spawnVisible))
			{
				m_World.SetTypeVisible(WorldObjectType::SPAWN_POINT, spawnVisible);
			}
			if (ImGui::MenuItem("Lights", nullptr, &lightVisible))
			{
				m_World.SetTypeVisible(WorldObjectType::LIGHT, lightVisible);
			}
			if (ImGui::MenuItem("Particle Emitters", nullptr, &particleVisible))
			{
				m_World.SetTypeVisible(WorldObjectType::PARTICLE_EMITTER, particleVisible);
			}
			if (ImGui::MenuItem("Trigger Volumes", nullptr, &triggerVisible))
			{
				m_World.SetTypeVisible(WorldObjectType::TRIGGER_VOLUME, triggerVisible);
			}
			if (ImGui::MenuItem("Instance Portals", nullptr, &portalVisible))
			{
				m_World.SetTypeVisible(WorldObjectType::INSTANCE_PORTAL, portalVisible);
			}
			if (ImGui::MenuItem("Player Spawns", nullptr, &playerSpawnVisible))
			{
				m_World.SetTypeVisible(WorldObjectType::PLAYER_SPAWN, playerSpawnVisible);
			}
			ImGui::EndMenu();
		}

		// Show current map name in the menu bar
		if (m_MapLoaded)
		{
			const MapEntry* entry = m_MapRegistry.GetMapById(m_CurrentMapId);
			if (entry)
			{
				float availWidth = ImGui::GetContentRegionAvail().x;
				std::string mapInfo = entry->displayName + " (" + MapInstanceTypeName(entry->instanceType) + ")";
				float textWidth = ImGui::CalcTextSize(mapInfo.c_str()).x;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - textWidth);
				ImGui::TextDisabled("%s", mapInfo.c_str());
			}
		}
	}

	void Editor3DLayer::HandleGlobalShortcuts()
	{
		ImGuiIO& io = ImGui::GetIO();

		if (io.WantTextInput)
			return;

		if (ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			m_World.DeleteSelected();
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			m_World.DeselectAll();
		}

		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
		{
			m_World.SelectAll();
		}

		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
		{
			CommandHistory::Get().Undo();
		}

		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))
		{
			CommandHistory::Get().Redo();
		}

		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
		{
			HandleSave();
		}
	}

	void Editor3DLayer::LoadMap(uint32_t mapId)
	{
		const MapEntry* entry = m_MapRegistry.GetMapById(mapId);
		if (!entry)
			return;

		// Unload previous map if any
		if (m_MapLoaded)
		{
			UnloadMap();
		}

		m_CurrentMapId = mapId;
		m_MapLoaded = true;

		EditorPreferences::Instance().PushRecentMap(mapId);

		m_World.NewMap(entry->displayName);

		// Initialize terrain system with map-scoped chunks directory
		std::string chunksDir = m_MapRegistry.GetChunksDirectory(mapId);
		m_ViewportPanel->GetWorldSystem().SetEditorWorld(&m_World);
		m_ViewportPanel->GetWorldSystem().Init(chunksDir);

		// Pull authored DB-bound entities (creature spawns, player spawns)
		// back from Postgres. Static objects come from chunk binaries via Init().
		LoadWorldFromDatabase(mapId);
		m_World.SetDirty(false);
	}

	void Editor3DLayer::UnloadMap()
	{
		if (!m_MapLoaded)
			return;

		// Save dirty terrain before unloading
		if (m_ViewportPanel)
		{
			m_ViewportPanel->GetWorldSystem().SaveDirtyChunks();
			m_ViewportPanel->GetWorldSystem().Shutdown();
		}

		m_World.NewMap("Untitled Map");
		m_CurrentMapId = 0;
		m_MapLoaded = false;
	}

	void Editor3DLayer::RenderRuntimeExportDialog()
	{
		if (m_ShowRuntimeExportDialog)
		{
			ImGui::OpenPopup("Export Runtime Data");
			m_ShowRuntimeExportDialog = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 350), ImGuiCond_Appearing);

		if (ImGui::BeginPopupModal("Export Runtime Data", nullptr, ImGuiWindowFlags_NoResize))
		{
			ImGui::TextWrapped("Export map data to Data/ folder for client runtime use.");
			ImGui::TextWrapped("Output: Data/maps/, Data/models/, Data/textures/");
			ImGui::Spacing();

			bool canExport = m_MapLoaded;
			if (!canExport)
				ImGui::BeginDisabled();
			if (ImGui::Button("Export", ImVec2(-1, 30)))
			{
				ExportRuntimeFiles();
			}
			if (!canExport)
				ImGui::EndDisabled();

			ImGui::Spacing();
			ImGui::SeparatorText("Log");
			ImGui::BeginChild("RuntimeExportLog", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Borders);
			for (const auto& line : m_RuntimeExportLog)
			{
				ImGui::TextWrapped("%s", line.c_str());
			}
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			{
				ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();

			if (ImGui::Button("Close", ImVec2(-1, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void Editor3DLayer::ExportRuntimeFiles()
	{
		if (!m_MapLoaded || !m_ViewportPanel)
			return;

		m_RuntimeExportLog.clear();
		m_RuntimeExportLog.push_back("Starting runtime export for map " + std::to_string(m_CurrentMapId) + "...");

		auto result = m_ViewportPanel->GetWorldSystem().ExportForRuntime("Data", m_CurrentMapId);

		m_RuntimeExportLog.push_back("Models exported: " + std::to_string(result.modelsExported));
		m_RuntimeExportLog.push_back("Materials exported: " + std::to_string(result.materialsExported));
		m_RuntimeExportLog.push_back("Chunks exported: " + std::to_string(result.chunksExported));
		m_RuntimeExportLog.push_back("Textures copied: " + std::to_string(result.texturesCopied));

		for (const auto& err : result.errors)
		{
			m_RuntimeExportLog.push_back("ERROR: " + err);
		}

		if (result.success)
		{
			m_RuntimeExportLog.push_back("Export completed successfully.");
		}
		else
		{
			m_RuntimeExportLog.push_back("Export completed with errors.");
		}
	}

	void Editor3DLayer::StartRunLocally()
	{
		if (!m_MapLoaded || !m_ViewportPanel)
			return;

		// Run the export first so migration.sql exists. Reuses the same Data/
		// output dir as the standalone Export Runtime Data flow.
		auto result = m_ViewportPanel->GetWorldSystem().ExportForRuntime("Data", m_CurrentMapId);
		if (!result.success)
		{
			std::cerr << "[Run Locally] export failed; not starting servers" << std::endl;
			for (const auto& err : result.errors)
				std::cerr << "  " << err << std::endl;
			return;
		}

		// Sibling binaries live in the same dir as the editor exe.
		// ONYX_BIN_DIR overrides for unusual layouts.
		std::filesystem::path binDir;
		if (const char* override_dir = std::getenv("ONYX_BIN_DIR"))
		{
			binDir = std::filesystem::path(override_dir);
		}
		else
		{
			binDir = Onyx::Platform::GetExecutableDir();
		}

		m_RunSession.reset();
		m_RunSession = std::make_unique<LocalRunSession>();
		if (!m_RunSession->Start(binDir, std::filesystem::absolute("Data"), m_CurrentMapId))
		{
			std::cerr << "[Run Locally] failed: " << m_RunSession->LastError() << std::endl;
		}
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// SAVE / DB ROUND-TRIP
	// ─────────────────────────────────────────────────────────────────────────────

	void Editor3DLayer::HandleSave()
	{
		if (!m_MapLoaded || !m_ViewportPanel)
			return;
		m_ViewportPanel->GetWorldSystem().SaveDirtyChunks();
		SyncWorldToDatabase();
		m_LastSaveTime = std::chrono::steady_clock::now();
	}

#ifdef HAS_DATABASE

	namespace {

		std::string BuildEditorDbConnString()
		{
			auto get = [](const char* name, const char* fallback) -> std::string {
				const char* v = std::getenv(name);
				return v ? std::string(v) : std::string(fallback);
			};
			std::ostringstream s;
			s << "host=" << get("DB_HOST", "localhost")
			  << " port=" << get("DB_PORT", "5432")
			  << " user=" << get("DB_USER", "root")
			  << " password=" << get("DB_PASS", "root")
			  << " dbname=" << get("DB_NAME", "mmogame");
			return s.str();
		}

	} // namespace

	bool Editor3DLayer::EnsureDatabaseConnected()
	{
		if (m_Database.IsConnected())
			return true;
		if (m_DatabaseConnectAttempted)
			return false;
		m_DatabaseConnectAttempted = true;
		if (!m_Database.Connect(BuildEditorDbConnString()))
		{
			std::cerr << "[Editor] Could not connect to Postgres — spawn entities "
						 "will not persist across editor restarts. Check DB_HOST/"
						 "DB_USER/DB_PASS/DB_NAME env vars."
					  << std::endl;
			return false;
		}
		// Schema migrations are owned by the LoginServer/WorldServer at boot, but
		// run them here too so a freshly-cloned editor can edit a freshly-created
		// database without needing the servers to have ever run.
		if (!MigrationRunner::ApplyAll(m_Database))
		{
			std::cerr << "[Editor] Schema migrations failed; spawn load/save will be skipped." << std::endl;
			m_Database.Disconnect();
			return false;
		}
		std::cout << "[Editor] Connected to Postgres for spawn round-trip." << std::endl;
		return true;
	}

	void Editor3DLayer::LoadWorldFromDatabase(uint32_t mapId)
	{
		if (!EnsureDatabaseConnected())
			return;

		// ── Creature spawns → SpawnPoints. DB stores Z-up; editor uses Y-up.
		auto creatures = m_Database.LoadCreatureSpawns(mapId);
		for (const auto& cs : creatures)
		{
			std::string name = "Creature " + std::to_string(cs.creatureTemplateId);
			SpawnPoint* sp = m_World.CreateSpawnPoint(name);
			// Axis swap: DB(x, y_ground, z_vertical) → Editor(x, y_vertical, z_ground)
			sp->SetPosition(glm::vec3(cs.positionX, cs.positionZ, cs.positionY));
			sp->SetRotation(glm::angleAxis(cs.orientation, glm::vec3(0.0f, 1.0f, 0.0f)));
			sp->SetCreatureTemplateId(cs.creatureTemplateId);
			sp->SetRespawnTime(cs.respawnTime);
			sp->SetWanderRadius(cs.wanderRadius);
			sp->SetMaxCount(cs.maxCount);
		}

		// ── Player spawns → group player_create_info rows by exact (pos, ori) tuple
		// so a single editor PlayerSpawn can carry multiple (race, class) pairs.
		auto rows = m_Database.LoadPlayerCreateInfo();
		struct Key
		{
			uint32_t mapId;
			float x, y, z, ori;
			bool operator<(const Key& o) const
			{
				return std::tie(mapId, x, y, z, ori) < std::tie(o.mapId, o.x, o.y, o.z, o.ori);
			}
		};
		std::map<Key, std::vector<std::pair<uint8_t, uint8_t>>> grouped;
		for (const auto& r : rows)
		{
			if (r.info.mapId != mapId)
				continue;
			grouped[{r.info.mapId, r.info.positionX, r.info.positionY, r.info.positionZ, r.info.orientation}]
				.push_back({r.race, r.cls});
		}
		for (const auto& [k, pairs] : grouped)
		{
			PlayerSpawn* ps = m_World.CreatePlayerSpawn();
			ps->SetPosition(glm::vec3(k.x, k.z, k.y)); // axis swap — see above
			ps->SetOrientation(k.ori);
			ps->SetRotation(glm::angleAxis(k.ori, glm::vec3(0.0f, 1.0f, 0.0f)));
			for (auto [race, cls] : pairs)
			{
				ps->AddRaceClass(static_cast<CharacterRace>(race),
								 static_cast<CharacterClass>(cls));
			}
		}

		if (!creatures.empty() || !grouped.empty())
		{
			std::cout << "[Editor] Loaded " << creatures.size()
					  << " creature spawns and " << grouped.size()
					  << " player spawns from DB for map " << mapId << "." << std::endl;
		}
	}

	void Editor3DLayer::SyncWorldToDatabase()
	{
		if (!m_MapLoaded || !EnsureDatabaseConnected())
			return;

		// Build the same SQL the migration.sql writer emits, but apply it directly
		// to the live connection instead of writing to disk. The writer methods
		// emit their own BEGIN/COMMIT, so each entity-type block is self-contained.
		std::ostringstream sql;

		std::vector<const SpawnPoint*> spawns;
		spawns.reserve(m_World.GetSpawnPoints().size());
		for (const auto& sp : m_World.GetSpawnPoints())
			spawns.push_back(sp.get());
		MigrationSqlWriter::EmitCreatureSpawnsForMap(m_CurrentMapId, spawns, sql);

		std::vector<const PlayerSpawn*> players;
		players.reserve(m_World.GetPlayerSpawns().size());
		for (const auto& ps : m_World.GetPlayerSpawns())
			players.push_back(ps.get());
		MigrationSqlWriter::EmitPlayerCreateInfo(players, m_CurrentMapId, sql);

		const std::string sqlText = sql.str();
		if (sqlText.empty())
			return;

		try
		{
			pqxx::nontransaction txn(m_Database.GetRawConnection());
			txn.exec(sqlText);
		}
		catch (const std::exception& e)
		{
			std::cerr << "[Editor] Failed to sync spawns to DB: " << e.what() << std::endl;
		}
	}

#else // !HAS_DATABASE

	bool Editor3DLayer::EnsureDatabaseConnected() { return false; }
	void Editor3DLayer::LoadWorldFromDatabase(uint32_t /*mapId*/) {}
	void Editor3DLayer::SyncWorldToDatabase() {}

#endif // HAS_DATABASE

} // namespace MMO
