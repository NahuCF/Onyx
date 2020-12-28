#include <iostream>

#include "graphics/window.h"
#include "graphics/shader.h"
#include "src/graphics/texture.h"

#include "GLFW/glfw3.h"

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

int main()
{
	using namespace se;
	using namespace graphics;

	Window window("SE", 1600, 900);
	glClearColor(0.3f, 0.3f, 1.0f, 1.0f);

	Shader GeneralTextureShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Texture Mario("./assets/textures/Mario.png", 1.0f, 1.9f);

	while (!window.Closed())
	{
		window.Vsync("Enable");
		window.Clear();

		//CODE HERE

		if (window.IsKeyPressed(GLFW_KEY_E))
			std::cout << "E key presssed PAPU" << std::endl;

		GeneralTextureShader.UseProgramShader();
		Mario.UseTexture();


		//

		window.Update();
	}
}