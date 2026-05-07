#pragma once

#include "EditorPanel.h"
#include <Graphics/Framebuffer.h>
#include <UI/Color.h>
#include <UI/Render/UIRenderer.h>
#include <UI/Widget.h>
#include <UI/WidgetTree.h>
#include <glm/glm.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct ImVec2;

namespace Onyx::UI {
	class FontAtlas;
}

namespace MMO {

	// First-cut UIEditor workspace per docs/ui-library-design.md M12.
	//
	// Hosts an isolated Onyx::UI::WidgetTree inside an ImGui-backed framebuffer
	// with its own UIRenderer instance — separate from the main
	// Onyx::UI::Manager renderer so the editor preview is insulated from any
	// GL state churn caused by the rest of the editor's rendering. Mouse
	// hover / click are forwarded from ImGui to the tree when the canvas image
	// is hovered.
	//
	// Designed as a stage simulator (Q16). Primary navigation is the stage
	// dropdown in the top toolbar; each stage builds its own test tree.
	// Loader-dependent M12 features (drag-from-palette-to-canvas, theme cascade
	// with real values, live binding mocks, runtime locale switch, MMOClient
	// hot-reload) are stubbed until M5/M6/M7/M10 ship; the stage simulator
	// chrome itself is fully wired so the UX can be evaluated now.
	class UIEditorPanel : public EditorPanel
	{
	public:
		UIEditorPanel();
		~UIEditorPanel() override;

		void OnInit() override;
		void OnImGuiRender() override;

	private:
		// ----- registration tables -----

		struct StageEntry
		{
			std::string name;
			std::string description;
			std::function<std::unique_ptr<Onyx::UI::Widget>()> build;
		};
		struct ResolutionEntry
		{
			std::string name;
			int width;
			int height;
			int safeAreaInset;
		};
		struct MockProfile
		{
			std::string name;
			std::string description;
		};
		struct ThemePreset
		{
			std::string name;
			Onyx::UI::Color background;
			Onyx::UI::Color panel;
			Onyx::UI::Color accent;
			Onyx::UI::Color text;
		};
		struct LocaleEntry
		{
			std::string code;
			std::string name;
		};

		void RegisterStages();
		void RegisterResolutions();
		void RegisterMockProfiles();
		void RegisterThemes();
		void RegisterLocales();

		// ----- stage builders -----
		// Each builder returns a fully-constructed widget tree root that
		// demonstrates the visual composition expected at that stage. All
		// builders run against the editor preview's own font atlas + shared
		// UIRenderer; they don't touch the live game's UI.
		std::unique_ptr<Onyx::UI::Widget> BuildLoginStage();
		std::unique_ptr<Onyx::UI::Widget> BuildCharacterSelectStage();
		std::unique_ptr<Onyx::UI::Widget> BuildLoadingStage();
		std::unique_ptr<Onyx::UI::Widget> BuildInGameHudStage();
		std::unique_ptr<Onyx::UI::Widget> BuildInGameInventoryStage();
		std::unique_ptr<Onyx::UI::Widget> BuildDeathScreenStage();
		std::unique_ptr<Onyx::UI::Widget> BuildCombatLowHpStage();

		void RebuildCurrentStage();

		// ----- rendering -----
		void EnsureFramebuffer(int width, int height);
		void RenderCanvasToFramebuffer(int canvasWidth, int canvasHeight, float deltaTime);
		void DrawSafeAreaOverlay(int canvasWidth, int canvasHeight, int safeAreaInset);
		void DrawSelectionHandles();

		// ----- ImGui sub-panels -----
		void DrawTopToolbar();
		void DrawHierarchyColumn();
		void DrawCanvasColumn(float availableWidth, float availableHeight);
		void DrawInspectorColumn();
		void DrawPaletteStrip();
		void DrawBottomTabs();

		void DrawHierarchyNode(Onyx::UI::Widget* widget);
		void DrawWidgetInspector(Onyx::UI::Widget* widget);
		void DrawStyleCascadePlaceholder(Onyx::UI::Widget* widget);

		// ----- input forwarding -----
		void DispatchInputFromImGui(const ImVec2& imageOrigin, const ImVec2& imageSize);

		// ----- runtime state -----
		std::unique_ptr<Onyx::Framebuffer>    m_Framebuffer;
		std::unique_ptr<Onyx::UI::UIRenderer> m_Renderer;
		std::unique_ptr<Onyx::UI::WidgetTree> m_Tree;
		bool m_RendererInitialized = false;

		int m_FbWidth = 0;
		int m_FbHeight = 0;

		// Stage simulator state
		std::vector<StageEntry>      m_Stages;
		std::vector<ResolutionEntry> m_Resolutions;
		std::vector<MockProfile>     m_Mocks;
		std::vector<ThemePreset>     m_Themes;
		std::vector<LocaleEntry>     m_Locales;

		int m_CurrentStage  = 0;
		int m_CurrentRes    = 0;
		int m_CurrentMock   = 0;
		int m_CurrentTheme  = 0;
		int m_CurrentLocale = 0;

		// Diagnostic toggles
		bool m_ShowSafeArea = true;
		bool m_ShowZXRay    = false;
		bool m_ShowOverflow = false;
		bool m_ShowGrid     = false;

		// Live-client indicator. Placeholder until M5 wires efsw to a running
		// MMOClient process; for now the dot is always grey.
		bool m_LiveClientConnected = false;

		// Selection (the currently inspected widget)
		Onyx::UI::Widget* m_SelectedWidget = nullptr;

		// Mouse-edge tracking — translates ImGui's polling state into the
		// edge events our WidgetTree expects.
		bool      m_PrevMouseValid = false;
		glm::vec2 m_PrevMouse{0.0f};
		bool      m_PrevButtons[3] = {false, false, false};

		// Per-stage time accumulator (drives loading bar etc.)
		float m_StageTime = 0.0f;

		// Active bottom-strip tab: 0=Files, 1=Themes, 2=Locales, 3=Mocks, 4=Sockets
		int m_BottomTab = 0;
	};

} // namespace MMO
