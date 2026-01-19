#include "EditorLayer.h"
#include <Onyx.h>
#include <Source/Core/EntryPoint.h>

class MMOEditorApp : public Onyx::Application {
public:
    MMOEditorApp(Onyx::ApplicationSpec& spec) : Application(spec) {
        PushLayer(new MMO::EditorLayer());
    }
};

static Onyx::ApplicationSpec s_AppSpec = { 1600, 900, "MMO Editor" };

Onyx::Application* Onyx::CreateApplication() {
    return new MMOEditorApp(s_AppSpec);
}
