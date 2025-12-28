// Temporary working editor application using Onyx engine
// Replace Main.cpp with this when building with CMake

#include "Onyx.h"
#include "Source/Core/EntryPoint.h"

class EditorLayer : public Onyx::Layer
{
public:
    EditorLayer() {}
    ~EditorLayer() {}

    virtual void OnUpdate() override
    {
        // Editor update logic here
    }

    virtual void OnImGui() override
    {
        // ImGui UI rendering
        ImGui::Begin("Onyx Editor");
        ImGui::Text("Welcome to Onyx Editor");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                    1000.0f / ImGui::GetIO().Framerate, 
                    ImGui::GetIO().Framerate);
        ImGui::End();

        // Viewport window
        ImGui::Begin("Viewport");
        ImGui::Text("3D Viewport will be rendered here");
        ImGui::End();

        // Properties window
        ImGui::Begin("Properties");
        ImGui::Text("Object properties");
        ImGui::End();

        // File browser
        ImGui::Begin("Assets");
        ImGui::Text("Asset browser");
        ImGui::End();
    }
};

class EditorApplication : public Onyx::Application
{
public:
    EditorApplication() : Onyx::Application(m_Spec)
    {
        m_Spec.applicationName = "Onyx Editor";
        m_Spec.windowWidth = 1280;
        m_Spec.windowHeight = 720;
        
        PushLayer(new EditorLayer());
    }

    ~EditorApplication()
    {
    }

private:
    Onyx::ApplicationSpec m_Spec;
};

// Entry point - required by Onyx engine
Onyx::Application* Onyx::CreateApplication()
{
    return new EditorApplication();
}