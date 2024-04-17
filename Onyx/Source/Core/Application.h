#pragma once

#include "Source/Core/Layer.h"
#include "Source/Graphics/Window.h"
#include "Source/Core/Base.h"

#include <vector>
#include <string>

int main();

namespace Onyx {

    struct ApplicationSpec
    {
        uint32_t windowWidth = 1280;
        uint32_t windowHeight = 720;
        std::string applicationName = "Application";
    };

    class Application
    {
    public:
        Application(ApplicationSpec& spec);
        virtual ~Application();

        void PushLayer(Layer* layer);
    private:
        void Run();

    private:
        bool m_IsRunning = true;
        std::vector<Layer*> m_LayerStack;
        Ref<Onyx::Window> m_Window;

        friend int ::main();
    };

    Application* CreateApplication();

}