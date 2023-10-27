#include "Game.h"
#include <cmath>
#include "Mechanics/Unit.h"
#include "Actions/ActionMove.h"
#include "Functions.h"
#include "Enums.h"

Game::Game()
{
	//lptm::Vector2D windowSize = { 1800, 900 };
	lptm::Vector2D windowSize = { 1366, 768 };
    lptm::Vector2D mapSize {(float)MapSize::Tiny, (float)MapSize::Tiny};

	m_Window = std::make_unique<se::Window>("Application", windowSize.x, windowSize.y, windowSize.x / windowSize.y, false);
    m_Camera = std::make_shared<se::Camera>();
	m_Renderer = std::make_shared<se::Renderer2D>(*m_Window, m_Camera);
    m_Map = std::make_shared<Map>(mapSize.x, mapSize.y);
	m_MapRenderer = std::make_unique<MapRenderer>(mapSize.x, mapSize.y, lptm::Vector2D(0.0f, 0.0f), lptm::Vector2D(40.0f, 20.0f), m_Map, m_Renderer);
	m_EntityManager = std::make_unique<EntityManager>(400, m_Renderer, m_Map);

    //m_Window->MakeFullScreen(); 
	CommodityMap startCommodities;

    // Create player
	m_Player = std::make_shared<Player>(1, startCommodities);

    // Create unit
	unit = std::make_shared<Unit>();
    unit.get()->SetPlayer(m_Player);
    unit.get()->SetPosition({ 0.2f, 0.2f});

    EntityPtr entity1 = std::make_shared<Unit>();
    entity1->SetPlayer(m_Player);
    entity1->SetPosition({ 0.2f, 0.4f});

    EntityPtr entity2 = std::make_shared<Unit>();
    entity2->SetPlayer(m_Player);
    entity2->SetPosition({ 0.4f, 0.2f});

    EntityPtr entity3 = std::make_shared<Unit>();
    entity3->SetPlayer(m_Player);
    entity3->SetPosition({ 0.4f, 0.4f});
    // Add unit to manager 
    //m_EntityManager->Add(unit);
    //m_EntityManager->Add(entity1);
    //m_EntityManager->Add(entity2);
    //m_EntityManager->Add(entity3);

    for (int i = 0; i < 1000 * 30; i++) {
        //EntityPtr asd = std::make_shared<Unit>();
        //asd->SetPlayer(m_Player);
        ////asd->SetPosition({ 0.3f + 0.01f * i, 0.4f });
        //m_EntityManager->Add(asd);
    }

    m_EntityManager->Add(unit);

	m_Window->SetVSync(1);
	m_Window->SetWindowColor({ .0f, .0f, .0f, 1.0f });
}

Game::~Game()
{
}

static lptm::Vector2D BeginPos;

void Game::Update()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(m_Window->WindowGUI(), true);
	ImGui_ImplOpenGL3_Init("#version 330");

	while (!m_Window->ShouldClose())
	{
		m_Window->Clear();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Debug");
        ImGui::Text("Framerate: %i Aspect Ratio: %f", m_Window->GetFramerate(), m_Window->GetAspectRatio());
        
        lptm::Vector2D normalizedMousePos = m_Renderer->GetWindow().GetMousePos();
        ImGui::Text("Start Mouse pos: %f, %f", BeginPos.x, BeginPos.y);
        ImGui::Text("Mouse pos: %f, %f", normalizedMousePos.x, normalizedMousePos.y);

        OnInput();

		m_MapRenderer->Update();
        m_EntityManager->Update();

		m_MapRenderer->Render();
		ImGui::End();
		m_Renderer->Flush();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		m_Window->Update(); 
	}
}

void Game::OnInput()
{
    if(m_Window->IsButtomJustPressed(GLFW_MOUSE_BUTTON_1))
    {
        BeginPos = m_Renderer->GetWindow().GetMousePos();
    }

    if(m_Window->IsButtomJustPressed(GLFW_MOUSE_BUTTON_2))
    {
        lptm::Vector2D offsetInPixels = NormalizedToPixel({m_Camera->GetPosition().x, m_Camera->GetPosition().y}, m_Renderer->GetWindow().GetWindowSize());
        m_EntityManager->OnRightClick(
            PixelToNormalized(m_Window->GetMousePosInPixels() - offsetInPixels, m_Window->GetWindowSize())
        );
    }

    if(m_Window->IsButtomPressed(GLFW_MOUSE_BUTTON_1))
    {
        lptm::Vector2D mousePos = m_Renderer->GetWindow().GetMousePos();
        float sizeX = mousePos.x - BeginPos.x;
        float sizeY = (mousePos.y - BeginPos.y)/2;
        // Top
        m_Renderer->RenderRotatedQuad({sizeX, 0.001f}, {BeginPos.x + sizeX / 2, BeginPos.y, 0.0f}, { 1.0f, 1.0f, 1.0f, 1.0f}, 0.0f);
        // Bottom
        m_Renderer->RenderRotatedQuad({sizeX, 0.001f}, {BeginPos.x + sizeX / 2, mousePos.y, 0.0f}, { 1.0f, 1.0f, 1.0f, 1.0f}, 0.0f);
        // Left
        float offsetY = (mousePos.y - BeginPos.y) / 2;
        m_Renderer->RenderRotatedQuad({sizeY, 0.001f}, {BeginPos.x, mousePos.y - offsetY, 0.0f}, { 1.0f, 1.0f, 1.0f, 1.0f}, 90.0f);
        // Right
        m_Renderer->RenderRotatedQuad({sizeY, 0.001f}, {BeginPos.x + sizeX, mousePos.y - offsetY, 0.0f}, { 1.0f, 1.0f, 1.0f, 1.0f}, 90.0f);

        ImGui::Text("SizeY: %f", sizeY);
    }

    if(m_Window->IsButtomReleased(GLFW_MOUSE_BUTTON_1))
    {
        lptm::Vector2D mousePos = m_Renderer->GetWindow().GetMousePos();
        m_EntityManager->SetSelected(BeginPos, mousePos, unit);
    }
}