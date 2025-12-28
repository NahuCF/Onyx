#include "ImGuiLayer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Source/Core/Application.h"

namespace Onyx {

	void ImGuiLayer::OnAttach()
	{
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      
        // Note: Docking and Viewports require ImGui docking branch
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        // Viewport specific styling disabled for now
        // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        // {
        //     style.WindowRounding = 0.0f;
        //     style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        // }

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
        // Viewport rendering disabled for now (requires docking branch)
        // GLFWwindow* backup_current_context = glfwGetCurrentContext();
        // ImGui::UpdatePlatformWindows();
        // ImGui::RenderPlatformWindowsDefault();
        // glfwMakeContextCurrent(backup_current_context);
    }

}

