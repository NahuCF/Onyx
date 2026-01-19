#include "ImGuiLayer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include "Source/Core/Application.h"

namespace Onyx {

    // Hazel-style theme colors
    namespace Theme {
        constexpr auto accent               = IM_COL32(236, 158, 36, 255);
        constexpr auto highlight            = IM_COL32(39, 185, 242, 255);
        constexpr auto background           = IM_COL32(36, 36, 36, 255);
        constexpr auto backgroundDark       = IM_COL32(26, 26, 26, 255);
        constexpr auto titlebar             = IM_COL32(21, 21, 21, 255);
        constexpr auto propertyField        = IM_COL32(15, 15, 15, 255);
        constexpr auto text                 = IM_COL32(192, 192, 192, 255);
        constexpr auto groupHeader          = IM_COL32(47, 47, 47, 255);
        constexpr auto backgroundPopup      = IM_COL32(50, 50, 50, 255);
    }

    static void SetDarkTheme()
    {
        auto& style = ImGui::GetStyle();
        auto& colors = ImGui::GetStyle().Colors;

        // Headers
        colors[ImGuiCol_Header]             = ImGui::ColorConvertU32ToFloat4(Theme::groupHeader);
        colors[ImGuiCol_HeaderHovered]      = ImGui::ColorConvertU32ToFloat4(Theme::groupHeader);
        colors[ImGuiCol_HeaderActive]       = ImGui::ColorConvertU32ToFloat4(Theme::groupHeader);

        // Buttons
        colors[ImGuiCol_Button]             = ImColor(56, 56, 56, 200);
        colors[ImGuiCol_ButtonHovered]      = ImColor(70, 70, 70, 255);
        colors[ImGuiCol_ButtonActive]       = ImColor(56, 56, 56, 150);

        // Frame BG
        colors[ImGuiCol_FrameBg]            = ImGui::ColorConvertU32ToFloat4(Theme::propertyField);
        colors[ImGuiCol_FrameBgHovered]     = ImGui::ColorConvertU32ToFloat4(Theme::propertyField);
        colors[ImGuiCol_FrameBgActive]      = ImGui::ColorConvertU32ToFloat4(Theme::propertyField);

        // Tabs
        colors[ImGuiCol_Tab]                = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
        colors[ImGuiCol_TabHovered]         = ImColor(255, 225, 135, 30);
        colors[ImGuiCol_TabActive]          = ImColor(255, 225, 135, 60);
        colors[ImGuiCol_TabUnfocused]       = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
        colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

        // Title
        colors[ImGuiCol_TitleBg]            = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
        colors[ImGuiCol_TitleBgActive]      = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
        colors[ImGuiCol_TitleBgCollapsed]   = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Resize Grip
        colors[ImGuiCol_ResizeGrip]         = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered]  = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]   = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

        // Check Mark
        colors[ImGuiCol_CheckMark]          = ImGui::ColorConvertU32ToFloat4(Theme::text);

        // Slider
        colors[ImGuiCol_SliderGrab]         = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
        colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

        // Text
        colors[ImGuiCol_Text]               = ImGui::ColorConvertU32ToFloat4(Theme::text);

        // Separator
        colors[ImGuiCol_Separator]          = ImGui::ColorConvertU32ToFloat4(Theme::backgroundDark);
        colors[ImGuiCol_SeparatorActive]    = ImGui::ColorConvertU32ToFloat4(Theme::highlight);
        colors[ImGuiCol_SeparatorHovered]   = ImColor(39, 185, 242, 150);

        // Window Background
        colors[ImGuiCol_WindowBg]           = ImGui::ColorConvertU32ToFloat4(Theme::titlebar);
        colors[ImGuiCol_ChildBg]            = ImGui::ColorConvertU32ToFloat4(Theme::background);
        colors[ImGuiCol_PopupBg]            = ImGui::ColorConvertU32ToFloat4(Theme::backgroundPopup);
        colors[ImGuiCol_Border]             = ImGui::ColorConvertU32ToFloat4(Theme::backgroundDark);

        // Tables
        colors[ImGuiCol_TableHeaderBg]      = ImGui::ColorConvertU32ToFloat4(Theme::groupHeader);
        colors[ImGuiCol_TableBorderLight]   = ImGui::ColorConvertU32ToFloat4(Theme::backgroundDark);

        // Menubar
        colors[ImGuiCol_MenuBarBg]          = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };

        // Docking
        colors[ImGuiCol_DockingPreview]     = ImGui::ColorConvertU32ToFloat4(Theme::accent);
        colors[ImGuiCol_DockingEmptyBg]     = ImGui::ColorConvertU32ToFloat4(Theme::backgroundDark);

        // Style
        style.FrameRounding = 2.5f;
        style.FrameBorderSize = 1.0f;
        style.IndentSpacing = 11.0f;
    }

	void ImGuiLayer::OnAttach()
	{
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable multi-viewport

        // Load custom font (with fallback to default if file not found)
        float fontSize = 15.0f;
        ImFont* font = io.Fonts->AddFontFromFileTTF("Resources/Fonts/Roboto-Medium.ttf", fontSize);
        if (font) {
            io.FontDefault = font;
        } else {
            io.Fonts->AddFontDefault();
        }

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        SetDarkTheme();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        ImGui_ImplGlfw_InitForOpenGL(Application::GetInstance().GetWindow()->GetWindow(), true);
        ImGui_ImplOpenGL3_Init("#version 450");
	}

    void ImGuiLayer::OnImGui()
    {      
    }

    void ImGuiLayer::Begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::End()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

}

