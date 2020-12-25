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

	Shader MarioShader("./shaders/textureShader.vs", "./shaders/textureShader.fs");
	Texture Mario("./assets/textures/Mario.png", 0.1f, 0.17f);

	while(!window.Closed())
	{
		window.Vsync("Disable");
		window.Clear();

		//CODE HERE

		MarioShader.UseProgramShader();
		Mario.UseTexture();

		//

		window.Update();
	}
}