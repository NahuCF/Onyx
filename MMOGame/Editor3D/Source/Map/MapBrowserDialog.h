#pragma once

#include "EditorMapRegistry.h"
#include <string>

namespace Editor3D {

	class MapBrowserDialog
	{
	public:
		void SetRegistry(EditorMapRegistry* registry) { m_Registry = registry; }

		// Returns true when user has made a selection (open or create)
		bool Render();

		bool HasSelection() const { return m_HasSelection; }
		uint32_t GetSelectedMapId() const { return m_SelectedMapId; }

		void Reset();

	private:
		void RenderMapList();
		void RenderNewMapDialog();

		EditorMapRegistry* m_Registry = nullptr;
		uint32_t m_SelectedMapId = 0;
		bool m_HasSelection = false;

		// New map creation state
		bool m_ShowNewMapDialog = false;
		char m_NewMapName[128] = {};
		MapInstanceType m_NewMapType = MapInstanceType::OpenWorld;
		int m_NewMapMaxPlayers = 0;

		// Delete confirmation
		bool m_ShowDeleteConfirm = false;
		uint32_t m_DeleteTargetId = 0;
	};

} // namespace Editor3D
