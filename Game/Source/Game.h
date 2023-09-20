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

	se::ParticleSystem m_ParticleSystem;
	se::ParticleProperties m_ParticleProps;
};