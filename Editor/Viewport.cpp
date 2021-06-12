#include <SE.h>

#include "Viewport.h"

lptm::Vector2D PixelToNDC(lptm::Vector2D pixels, lptm::Vector2D windowSize)
{
    return lptm::Vector2D(pixels.x / (windowSize.x / 2), pixels.y / (windowSize.y / 2));
}

Viewport::Viewport()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
}

ImGuiIO& Viewport::Init(se::Window* window)
{
    m_Window = window;
    this->map = new se::OrtogonalTilemap(8, 8, 100, 100);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    /*io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;*/

    ImGui_ImplGlfw_InitForOpenGL(window->WindowGUI(), true);
    ImGui_ImplOpenGL3_Init();
    
    return io;
}

Viewport::~Viewport()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete map;
}

void Viewport::Loop(ImGuiIO& io) 
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

    /*ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if(dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;*/

    /*ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport", (bool*)true, window_flags);

    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);*/

    /*if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("Viewport");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }*/

    // --- Actual Code --------------------

    map->SetData(m_Window->GetMousePos(), m_Window->GetOffsets(), m_Window->GetWindowSize());
    static lptm::Vector2D initialMousePos;


    if(m_Window->IsButtomJustPressed(GLFW_MOUSE_BUTTON_MIDDLE))
    {
        initialMousePos = m_Window->GetMousePos();
    }
    else if(m_Window->IsButtomPressed(GLFW_MOUSE_BUTTON_MIDDLE) && m_Window->IsMouseMoving())
    {
        m_Window->AddToOffsets(lptm::Vector2D(m_Window->GetMouseX() - initialMousePos.x, m_Window->GetMouseY() - initialMousePos.y));

        lptm::Vector2D moveBy = PixelToNDC(
            lptm::Vector2D(m_Window->GetMouseX() - initialMousePos.x, m_Window->GetMouseY() - initialMousePos.y),
            m_Window->GetWindowSize());

        for(uint32_t i = 0; i < m_Shaders.size(); i++)
        {
            m_Shaders[i]->SetPos(glm::vec3(m_Shaders[i]->GetRealPosX() + moveBy.x, m_Shaders[i]->GetRealPosY() - moveBy.y, 0.0f));
        }

        initialMousePos = m_Window->GetMousePos();
    }

    if(m_Window->IsButtomPressed(GLFW_MOUSE_BUTTON_LEFT) 
        && map->GetCurrentTile().x >= 0
        && map->GetCurrentTile().y >= 0
        && map->GetCurrentTile().x < map->GetTilemapSize().x
        && map->GetCurrentTile().y < map->GetTilemapSize().y
        && map->GetTileValue() == 0)
    {
        m_Shaders.push_back(new se::Shader("Assets/Shaders/TextureShader.vs", "Assets/Shaders/TextureShader.fs"));
        m_Shaders.back()->SetPos(glm::vec3(map->PositionTile().x, map->PositionTile().y, 0.0f));
        map->SetTile(2);
        std::cout << "Tiled Placed" << std::endl;
    }
    std::cout << "X: " << map->GetCurrentTile().x << " Y: " << map->GetCurrentTile().y << std::endl;
    static se::Texture texture("Assets/Textures/grass.png", 0.2f, 0.2f);

    for(uint32_t i = 0; i < m_Shaders.size(); i++)
    {
        m_Shaders[i]->UseProgramShader();
        texture.UseTexture();
    }

    //std::cout << "X: " << map->PositionTile().x << " Y: " << map->PositionTile().y << std::endl;

    //std::cout << "X: " << map->GetCurrentTile().x << " Y: " << map->GetCurrentTile().y << std::endl;

    /*if(ImGui::BeginMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();

    ImGui::Begin("Files");
    ImGui::Text("Framerate; %d", (int)(1000 / m_Window->GetMilliseconds()));
    ImGui::End();

    ImGui::Begin("asdasd");
    ImGui::Text("Siasdasd");
    ImGui::End();*/

    // --- End actual code ---------------

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    /*if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }*/
}