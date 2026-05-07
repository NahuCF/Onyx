#pragma once

#include "EditorPanel.h"
#include <Graphics/Framebuffer.h>
#include <UI/Render/UIRenderer.h>
#include <UI/WidgetTree.h>
#include <glm/glm.hpp>
#include <memory>

namespace Onyx::UI {
	class FontAtlas;
}

namespace MMO {

	// Hosts an Onyx::UI WidgetTree inside an ImGui window. Renders the tree
	// into its own framebuffer with its own UIRenderer instance (separate
	// from the main Onyx::UI::Manager renderer) so the editor preview is
	// isolated from any GL state churn caused by the rest of the editor's
	// rendering. Mouse hover / click forwarded from ImGui to the tree when
	// the image is hovered.
	class UIEditorPanel : public EditorPanel
	{
	public:
		UIEditorPanel();
		~UIEditorPanel() override;

		void OnInit() override;
		void OnImGuiRender() override;

	private:
		void BuildDefaultTestTree();
		void EnsureFramebuffer(int width, int height);
		void RenderUIToFramebuffer(int width, int height);
		void DispatchInputFromImGui(const glm::vec2& imageOrigin, const glm::vec2& imageSize);

		std::unique_ptr<Onyx::Framebuffer>   m_Framebuffer;
		std::unique_ptr<Onyx::UI::UIRenderer> m_Renderer;
		std::unique_ptr<Onyx::UI::WidgetTree> m_Tree;

		int m_FbWidth = 0;
		int m_FbHeight = 0;

		// Mouse-edge tracking — translates ImGui's polling state into the
		// edge events our WidgetTree expects.
		bool m_PrevMouseValid = false;
		glm::vec2 m_PrevMouse{0.0f};
		bool m_PrevButtons[3] = {false, false, false};
		double m_PrevTime = 0.0;
	};

} // namespace MMO
