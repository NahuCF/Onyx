#pragma once

#include "Source/Core/Application.h"

extern Onyx::Application* Onyx::CreateApplication();

int main()
{
    Onyx::Application* app = Onyx::CreateApplication(); 
    app->Run();
    delete app;
}