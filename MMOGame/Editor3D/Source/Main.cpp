#include "Editor3DLayer.h"
#include <Onyx.h>
#include <Source/Core/EntryPoint.h>

class MMOEditor3DApp : public Onyx::Application {
public:
    MMOEditor3DApp(Onyx::ApplicationSpec& spec) : Application(spec) {
        PushLayer(new MMO::Editor3DLayer());
    }
};

static Onyx::ApplicationSpec s_AppSpec = { 1600, 900, "MMO Editor 3D" };

Onyx::Application* Onyx::CreateApplication() {
    return new MMOEditor3DApp(s_AppSpec);
}
