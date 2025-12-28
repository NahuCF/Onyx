#include "Onyx.h"
#include "Source/Core/EntryPoint.h"

class GameLayer : public Onyx::Layer
{
public:
    GameLayer() 
    {
        // Initialize renderer
        auto& app = Onyx::Application::GetInstance();
        m_Camera = std::make_shared<Onyx::Camera>(
            app.GetWindow()->Width(), 
            app.GetWindow()->Height(),
            glm::vec3(0.0f, 0.0f, 3.0f)  // Add default position
        );
        
        m_Renderer = std::make_unique<Onyx::Renderer2D>(
            *app.GetWindow(), 
            m_Camera
        );
        
        // Load texture and shader for textured rendering
        m_FloorTexture = std::make_unique<Onyx::Texture>("assets/textures/Floor_Corner_01.png");
        m_TextureShader = std::make_unique<Onyx::Shader>("assets/shaders/basic.vert", "assets/shaders/basic.frag");
    }
    
    ~GameLayer() 
    {
    }

    virtual void OnUpdate() override
    {
        // Clear with dark blue background
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render a colored quad offset to the right
        // Position: right side, Size: 0.5x0.5, Color: cyan
        m_Renderer->RenderQuad(
            Onyx::Vector2D(0.5f, 0.5f),      // Size
            Onyx::Vector3(0.6f, 0.0f, 0.0f),  // Position (right)
            Onyx::Vector4D(0.2f, 0.8f, 0.8f, 1.0f)  // Color (cyan)
        );
        
        // Render textured quad in the center
        m_Renderer->RenderQuad(
            Onyx::Vector2D(0.3f, 0.3f),      // Size
            Onyx::Vector3(0.0f, 0.0f, 0.0f),  // Position (centered)
            m_FloorTexture.get(),             // Texture
            m_TextureShader.get(),            // Shader
            Onyx::Vector2D(0.0f, 0.0f),       // Sprite coord (first tile)
            Onyx::Vector2D(1636.0f/4.0f, 249.0f)  // Sprite size (exactly 1636/4 x 249)
        );
        
        // Render a quad to the left
        m_Renderer->RenderQuad(
            Onyx::Vector2D(0.3f, 0.3f),      // Size
            Onyx::Vector3(-0.6f, 0.0f, 0.0f), // Position
            Onyx::Vector4D(0.8f, 0.8f, 0.2f, 1.0f)  // Color (yellow)
        );
        
        // Flush the renderer to draw all quads
        m_Renderer->Flush();
    }

    virtual void OnImGui() override
    {
        // Create a simple debug window
        ImGui::Begin("MMO Game Debug");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::Text("- Close window to exit");
        
        // Color picker for background
        static float bgColor[3] = { 0.1f, 0.1f, 0.2f };
        if (ImGui::ColorEdit3("Background Color", bgColor))
        {
            glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
        }
        
        ImGui::End();
    }

private:
    std::unique_ptr<Onyx::Renderer2D> m_Renderer;
    std::shared_ptr<Onyx::Camera> m_Camera;
    std::unique_ptr<Onyx::Texture> m_FloorTexture;
    std::unique_ptr<Onyx::Shader> m_TextureShader;
};

class MMOGameApplication : public Onyx::Application
{
public:
    MMOGameApplication() : Onyx::Application(GetSpec())
    {
        // Add our game layer
        PushLayer(new GameLayer());
    }

    ~MMOGameApplication()
    {
    }

private:
    static Onyx::ApplicationSpec& GetSpec()
    {
        static Onyx::ApplicationSpec spec;
        spec.windowWidth = 1024;
        spec.windowHeight = 768;
        spec.applicationName = "MMO Game";
        return spec;
    }
};

// Entry point required by Onyx engine
Onyx::Application* Onyx::CreateApplication()
{
    return new MMOGameApplication();
}