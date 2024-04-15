#pragma once

#include "Source/Core/Layer.h"

#include <vector>

int main();

namespace Onyx {

    class Application
    {
    public:
        Application();
        virtual ~Application();

        void PushLayer(Layer* layer);
    private:
        void Run();

        bool m_IsRunning = true;
        std::vector<Layer*> m_LayerStack;

        friend int ::main();
    };

    Application* CreateApplication();

}