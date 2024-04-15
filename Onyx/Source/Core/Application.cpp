#include "pch.h"

#include "Source/Core/Application.h"

namespace Onyx {

    Application::Application()
    {

    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        while (m_IsRunning)
        {
            for (Layer* layer : m_LayerStack)
            {
                layer->OnUpdate();
                layer->OnImGui();
            }
        }
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.push_back(layer);
    }

}