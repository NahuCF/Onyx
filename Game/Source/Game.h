#pragma once

#include <SE.h>
#include <memory>
#include "MapRenderer.h"
#include "EntityManager.h"

class Game
{
public:
	Game();
	~Game();

    void OnInput();

	void Update();
private:
	std::unique_ptr<se::Window> m_Window;
	std::shared_ptr<se::Renderer2D> m_Renderer;
	std::shared_ptr<se::Camera> m_Camera;
    std::unique_ptr<MapRenderer> m_MapRenderer;
    std::unique_ptr<EntityManager> m_EntityManager;
    std::shared_ptr<Map> m_Map;
    
	std::shared_ptr<Player> m_Player;

	std::shared_ptr<Entity> unit;

	ImGuiIO& Init();
};