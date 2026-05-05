#pragma once

#include "Map/EditorMapRegistry.h"
#include "Map/MapBrowserDialog.h"
#include "Panels/PanelManager.h"
#include "Runtime/LocalRunSession.h"
#include "World/EditorWorld.h"
#include <Core/Layer.h>
#include <Graphics/SceneRenderer.h>
#include <chrono>
#include <memory>

#ifdef HAS_DATABASE
#include "../../Shared/Source/Database/Database.h"
#endif

namespace MMO {

	class ViewportPanel;
	class InspectorPanel;
	class StatisticsPanel;
	class LightingPanel;
	class MaterialEditorPanel;

	class Editor3DLayer : public Onyx::Layer
	{
	public:
		Editor3DLayer();
		~Editor3DLayer() override;

		void OnAttach() override;
		void OnUpdate() override;
		void OnImGui() override;

		EditorWorld& GetWorld() { return m_World; }

	private:
		void SetupDockspace();
		void RenderMenuBar();
		void HandleGlobalShortcuts();
		void LoadMap(uint32_t mapId);
		void UnloadMap();

		// Editor ↔ DB round-trip for non-chunk objects (PlayerSpawn, SpawnPoint).
		// Static objects continue to live in chunk binaries; this is the locked
		// DB-only path for spawn-style entities.
		bool EnsureDatabaseConnected();
		void LoadWorldFromDatabase(uint32_t mapId);
		void SyncWorldToDatabase(); // Builds + applies spawn migration on Ctrl+S.
		void HandleSave();			// Ctrl+S / menu / autosave entry point.

		void ExportRuntimeFiles();
		void RenderRuntimeExportDialog();
		bool m_ShowRuntimeExportDialog = false;
		std::vector<std::string> m_RuntimeExportLog;

		// Run Locally — apply migration.sql + spawn LoginServer/WorldServer/MMOClient.
		void StartRunLocally();
		std::unique_ptr<LocalRunSession> m_RunSession;

		// Auto-save — interval driven by EditorPreferences.
		std::chrono::steady_clock::time_point m_LastSaveTime;

		EditorWorld m_World;
		PanelManager m_PanelManager;
		ViewportPanel* m_ViewportPanel = nullptr;
		std::unique_ptr<Onyx::SceneRenderer> m_SceneRenderer;

#ifdef HAS_DATABASE
		Database m_Database;
		bool m_DatabaseConnectAttempted = false;
#endif

		// Map management
		Editor3D::EditorMapRegistry m_MapRegistry;
		Editor3D::MapBrowserDialog m_MapBrowser;
		uint32_t m_CurrentMapId = 0;
		bool m_MapLoaded = false;
		bool m_ShowMapBrowser = true;
	};

} // namespace MMO
