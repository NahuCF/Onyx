#include "pch.h"

#include "Source/Core/Application.h"
#include "Source/Core/Base.h"

namespace Onyx {

    Application::Application(ApplicationSpec& spec)
    {
        m_Window = Onyx::MakeRef<Onyx::Window>(spec.applicationName.c_str(), spec.windowWidth, spec.windowHeight);

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
            {
                layer->OnUpdate();
                layer->OnImGui();
            }
            m_Window->Update();
        }
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.push_back(layer);
        layer->OnAttach();
    }

}