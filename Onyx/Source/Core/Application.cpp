#include "pch.h"

#include "Source/Core/Application.h"
#include "Source/Core/Base.h"
#include "Source/Core/ImGuiLayer.h"

namespace Onyx {

    Application* Application::s_Instance = nullptr;

    Application::Application(ApplicationSpec& spec)
    {
        m_Window = Onyx::MakeRef<Onyx::Window>(spec.applicationName.c_str(), spec.windowWidth, spec.windowHeight);

        s_Instance = this;

        m_ImGuiLayer = new ImGuiLayer();
        PushLayer(m_ImGuiLayer);
    }

    Application::~Application()
    {
        for (Layer* layer : m_LayerStack)
            delete layer;
    }

    void Application::Run()
    {
        while (!m_Window->ShouldClose())
        {
            m_Window->Clear();

            for (Layer* layer : m_LayerStack)
                layer->OnUpdate();

            m_ImGuiLayer->Begin();
            for (Layer* layer : m_LayerStack)
                layer->OnImGui();
            m_ImGuiLayer->End();

            m_Window->Update();

        }
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.push_back(layer);
        layer->OnAttach();
    }

}