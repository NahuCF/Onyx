#include <SE.h>

#include "GameObject.h"

class Game
{
public:
	Game();
	~Game();

	void Update();
private:
	se::Window* m_Window;
	se::Texture* m_TilesTexture;
	se::Shader* m_TextureShader;
	se::Shader* m_BaseShader;
	se::Renderer2D* m_Renderer;
	GameObject m_Player;

	int* m_World = nullptr;
	lptm::Vector2D m_WorldSize;
	lptm::Vector2D m_Origin;

	se::ParticleSystem m_ParticleSystem;
	se::ParticleProperties m_ParticleProps;

	lptm::Vector2D m_InitialMousePosition;
	lptm::Vector2D m_WorldOffset;
	lptm::Vector2D m_LastOffset;

	float m_Zoom = 1.0f;

	ImGuiIO& Init();
};