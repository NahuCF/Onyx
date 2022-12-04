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
	se::Shader* m_BaseShader;
	se::Renderer2D* m_Renderer;
	GameObject m_Player;

	se::ParticleSystem m_ParticleSystem;
	se::ParticleProperties m_ParticleProps;
};